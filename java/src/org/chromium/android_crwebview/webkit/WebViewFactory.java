/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.chromium.android_crwebview.webkit;

import android.app.ActivityManager;
import android.app.Application;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.Signature;
import android.content.res.Resources;
import android.os.Build;
import android.os.RemoteException;
import android.os.SystemClock;
import android.text.TextUtils;
import android.util.AndroidRuntimeException;
import android.util.ArraySet;
import android.util.Log;
import android.webkit.WebViewDelegateWrapper;
//import com.android.webview.chromium.WebViewChromiumFactoryProviderForT;

import java.io.File;
import java.lang.reflect.Method;

/**
 * Top level factory, used creating all the main WebView implementation classes.
 *
 * @hide
 */
public final class WebViewFactory {

    /** @hide */
    private static final String CHROMIUM_WEBVIEW_FACTORY =
            "com.android.webview.chromium.WebViewChromiumFactoryProviderForT";

    private static final String CHROMIUM_WEBVIEW_FACTORY_METHOD = "create";

    private static final String LOGTAG = "WebViewFactory";

    private static final boolean DEBUG = false;

    // Cache the factory both for efficiency, and ensure any one process gets all webviews from the
    // same provider.
    private static WebViewFactoryProvider sProviderInstance;
    private static final Object sProviderLock = new Object();
    private static PackageInfo sPackageInfo;
    private static Boolean sWebViewSupported = true;
    private static boolean sWebViewDisabled;
    private static String sDataDirectorySuffix; // stored here so it can be set without loading WV

    // Error codes for loadWebViewNativeLibraryFromPackage
    public static final int LIBLOAD_SUCCESS = 0;
    public static final int LIBLOAD_WRONG_PACKAGE_NAME = 1;
    public static final int LIBLOAD_ADDRESS_SPACE_NOT_RESERVED = 2;

    // error codes for waiting for WebView preparation
    public static final int LIBLOAD_FAILED_WAITING_FOR_RELRO = 3;
    public static final int LIBLOAD_FAILED_LISTING_WEBVIEW_PACKAGES = 4;

    // native relro loading error codes
    public static final int LIBLOAD_FAILED_TO_OPEN_RELRO_FILE = 5;
    public static final int LIBLOAD_FAILED_TO_LOAD_LIBRARY = 6;
    public static final int LIBLOAD_FAILED_JNI_CALL = 7;

    // more error codes for waiting for WebView preparation
    public static final int LIBLOAD_FAILED_WAITING_FOR_WEBVIEW_REASON_UNKNOWN = 8;

    // error for namespace lookup
    public static final int LIBLOAD_FAILED_TO_FIND_NAMESPACE = 10;

    // generic error for future use
    static final int LIBLOAD_FAILED_OTHER = 11;

    /**
     * Stores the timestamps at which various WebView startup events occurred in this process.
     */
    public static class StartupTimestamps {
        long mWebViewLoadStart;
        long mCreateContextStart;
        long mCreateContextEnd;
        long mAddAssetsStart;
        long mAddAssetsEnd;
        long mGetClassLoaderStart;
        long mGetClassLoaderEnd;
        long mNativeLoadStart;
        long mNativeLoadEnd;
        long mProviderClassForNameStart;
        long mProviderClassForNameEnd;

        StartupTimestamps() {}

        /** When the overall WebView provider load began. */
        public long getWebViewLoadStart() {
            return mWebViewLoadStart;
        }

        /** Before creating the WebView APK Context. */
        public long getCreateContextStart() {
            return mCreateContextStart;
        }

        /** After creating the WebView APK Context. */
        public long getCreateContextEnd() {
            return mCreateContextEnd;
        }

        /** Before adding WebView assets to AssetManager. */
        public long getAddAssetsStart() {
            return mAddAssetsStart;
        }

        /** After adding WebView assets to AssetManager. */
        public long getAddAssetsEnd() {
            return mAddAssetsEnd;
        }

        /** Before creating the WebView ClassLoader. */
        public long getGetClassLoaderStart() {
            return mGetClassLoaderStart;
        }

        /** After creating the WebView ClassLoader. */
        public long getGetClassLoaderEnd() {
            return mGetClassLoaderEnd;
        }

        /** Before preloading the WebView native library. */
        public long getNativeLoadStart() {
            return mNativeLoadStart;
        }

        /** After preloading the WebView native library. */
        public long getNativeLoadEnd() {
            return mNativeLoadEnd;
        }

        /** Before looking up the WebView provider class. */
        public long getProviderClassForNameStart() {
            return mProviderClassForNameStart;
        }

        /** After looking up the WebView provider class. */
        public long getProviderClassForNameEnd() {
            return mProviderClassForNameEnd;
        }
    }

    static final StartupTimestamps sTimestamps = new StartupTimestamps();

    static StartupTimestamps getStartupTimestamps() {
        return sTimestamps;
    }

    private static String getWebViewPreparationErrorReason(int error) {
        switch (error) {
            case LIBLOAD_FAILED_WAITING_FOR_RELRO:
                return "Time out waiting for Relro files being created";
            case LIBLOAD_FAILED_LISTING_WEBVIEW_PACKAGES:
                return "No WebView installed";
            case LIBLOAD_FAILED_WAITING_FOR_WEBVIEW_REASON_UNKNOWN:
                return "Crashed for unknown reason";
        }
        return "Unknown";
    }

    static class MissingWebViewPackageException extends Exception {
        public MissingWebViewPackageException(String message) { super(message); }
        public MissingWebViewPackageException(Exception e) { super(e); }
    }

    private static boolean isWebViewSupported() {
        return sWebViewSupported;
    }

    /**
     * @hide
     */
    static void disableWebView() {
        synchronized (sProviderLock) {
            if (sProviderInstance != null) {
                throw new IllegalStateException(
                        "Can't disable WebView: WebView already initialized");
            }
            sWebViewDisabled = true;
        }
    }

    /**
     * @hide
     */
    static void setDataDirectorySuffix(String suffix) {
        synchronized (sProviderLock) {
            if (sProviderInstance != null) {
                throw new IllegalStateException(
                        "Can't set data directory suffix: WebView already initialized");
            }
            if (suffix.indexOf(File.separatorChar) >= 0) {
                throw new IllegalArgumentException("Suffix " + suffix
                                                   + " contains a path separator");
            }
            sDataDirectorySuffix = suffix;
        }
    }

    /**
     * @hide
     */
    public static String getDataDirectorySuffix() {
        synchronized (sProviderLock) {
            return sDataDirectorySuffix;
        }
    }

    /**
     * @hide
     */
    public static String getWebViewLibrary(ApplicationInfo ai) {
        if (ai.metaData != null)
            return ai.metaData.getString("com.android.webview.WebViewLibrary");
        return null;
    }

    public static PackageInfo getLoadedPackageInfo() {
        synchronized (sProviderLock) {
            return sPackageInfo;
        }
    }

    /**
     * Load the native library for the given package name if that package
     * name is the same as the one providing the webview.
     */
    public static int loadWebViewNativeLibraryFromPackage(String packageName,
                                                          ClassLoader clazzLoader) {
		Log.i(LOGTAG, packageName + " loadWebViewNativeLibraryFromPackage CALLED");
        return 0;
    }

    static WebViewFactoryProvider getProvider() {
        synchronized (sProviderLock) {
            // For now the main purpose of this function (and the factory abstraction) is to keep
            // us honest and minimize usage of WebView internals when binding the proxy.
            if (sProviderInstance != null) return sProviderInstance;

            sTimestamps.mWebViewLoadStart = SystemClock.uptimeMillis();
            //final int uid = android.os.Process.myUid();
            //if (uid == android.os.Process.ROOT_UID || uid == android.os.Process.SYSTEM_UID
            //        || uid == android.os.Process.PHONE_UID || uid == android.os.Process.NFC_UID
            //        || uid == android.os.Process.BLUETOOTH_UID) {
            //    throw new UnsupportedOperationException(
            //            "For security reasons, WebView is not allowed in privileged processes");
            //}

            if (!isWebViewSupported()) {
                // Device doesn't support WebView; don't try to load it, just throw.
                throw new UnsupportedOperationException();
            }

            if (sWebViewDisabled) {
                throw new IllegalStateException(
                        "WebView.disableWebView() was called: WebView is disabled");
            }

            //sProviderInstance = WebViewChromiumFactoryProviderForT.create(new WebViewDelegateWrapper());
            //return sProviderInstance;
            try {
                Class<WebViewFactoryProvider> providerClass = getProviderClass();
                //Method staticFactory = providerClass.getMethod(
                Method staticFactory = providerClass.getDeclaredMethod(
                        CHROMIUM_WEBVIEW_FACTORY_METHOD, WebViewDelegateWrapper.class);
                try {
                    sProviderInstance = (WebViewFactoryProvider)
                            staticFactory.invoke(providerClass, new WebViewDelegateWrapper());
                    return sProviderInstance;
                } finally {
                }
            } catch (Exception e) {
                Log.e(LOGTAG, "error instantiating provider", e);
                throw new AndroidRuntimeException(e);
            } finally {
            }
        }
    }

    /**
     * Returns {@code true} if the signatures match, {@code false} otherwise
     */
    private static boolean signaturesEquals(Signature[] s1, Signature[] s2) {
        if (s1 == null) {
            return s2 == null;
        }
        if (s2 == null) return false;

        ArraySet<Signature> set1 = new ArraySet<>();
        for(Signature signature : s1) {
            set1.add(signature);
        }
        ArraySet<Signature> set2 = new ArraySet<>();
        for(Signature signature : s2) {
            set2.add(signature);
        }
        return set1.equals(set2);
    }

    private static Class<WebViewFactoryProvider> getProviderClass() {
        try {
            return (Class<WebViewFactoryProvider>) Class.forName(CHROMIUM_WEBVIEW_FACTORY);
        } catch (ClassNotFoundException e) {
            Log.i("crWebView", "Failed to load class: " + CHROMIUM_WEBVIEW_FACTORY, e);
        }

        return null;
    }


    /**
     * Perform any WebView loading preparations that must happen in the zygote.
     * Currently, this means allocating address space to load the real JNI library later.
     */
    public static void prepareWebViewInZygote() {
		Log.i(LOGTAG, "** prepareWebViewInZygote called ");
    }

    /**
     * @hide
     */
    public static int onWebViewProviderChanged(PackageInfo packageInfo) {
        int startedRelroProcesses = 0;
		Log.i(LOGTAG, "** onWebViewProviderChanged called");
        return startedRelroProcesses;
    }
}
