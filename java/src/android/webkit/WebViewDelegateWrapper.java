/*
 * Copyright (C) 2014 The Android Open Source Project
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

package android.webkit;

import android.app.Application;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.res.Resources;
import android.graphics.Canvas;
import android.graphics.RecordingCanvas;
import android.os.RemoteException;
import android.util.SparseArray;
import android.util.Log;
import android.view.View;
import android.graphics.Paint;
import android.graphics.Color;
import java.lang.reflect.Method;
import java.lang.reflect.Constructor;

/**
 * Delegate used by the WebView provider implementation to access
 * the required framework functionality needed to implement a {@link WebView}.
 *
 * @hide
 */
public final class WebViewDelegateWrapper {

    private Paint paint;

    public WebViewDelegateWrapper() {
        paint = new Paint();
        paint.setColor(Color.RED);
        paint.setStrokeWidth(3.0f);
        paint.setStyle(Paint.Style.STROKE);
    }

    /**
     * Listener that gets notified whenever tracing has been enabled/disabled.
     */
    public interface OnTraceEnabledChangeListener {
        void onTraceEnabledChange(boolean enabled);
    }

    /**
     * Register a callback to be invoked when tracing for the WebView component has been
     * enabled/disabled.
     */
    public void setOnTraceEnabledChangeListener(final OnTraceEnabledChangeListener listener) {
		Log.i("crWebView", "setOnTraceEnabledChangeListener called");
    }

    /**
     * Returns {@code true} if the WebView trace tag is enabled and {@code false} otherwise.
     */
    public boolean isTraceTagEnabled() {
        return false;
    }

    /**
     * Throws {@link UnsupportedOperationException}
     * @deprecated Use {@link #drawWebViewFunctor(Canvas, int)}
     */
    @Deprecated
    public boolean canInvokeDrawGlFunctor(View containerView) {
        throw new UnsupportedOperationException();
    }

    /**
     * Throws {@link UnsupportedOperationException}
     * @deprecated Use {@link #drawWebViewFunctor(Canvas, int)}
     */
    @Deprecated
    public void invokeDrawGlFunctor(View containerView, long nativeDrawGLFunctor,
            boolean waitForCompletion) {
        throw new UnsupportedOperationException();
    }

    /**
     * Throws {@link UnsupportedOperationException}
     * @deprecated Use {@link #drawWebViewFunctor(Canvas, int)}
     */
    @Deprecated
    public void callDrawGlFunction(Canvas canvas, long nativeDrawGLFunctor) {
        throw new UnsupportedOperationException();
    }

    /**
     * Throws {@link UnsupportedOperationException}
     * @deprecated Use {@link #drawWebViewFunctor(Canvas, int)}
     */
    @Deprecated
    public void callDrawGlFunction(Canvas canvas, long nativeDrawGLFunctor,
            Runnable releasedRunnable) {
        throw new UnsupportedOperationException();
    }

    /**
     * Call webview draw functor. See API in draw_fn.h.
     * @param canvas a {@link RecordingCanvas}.
     * @param functor created by AwDrawFn_CreateFunctor in draw_fn.h.
     */
    public void drawWebViewFunctor(Canvas canvas, int functor) {
        if (!(canvas instanceof RecordingCanvas)) {
            // Canvas#isHardwareAccelerated() is only true for subclasses of RecordingCanvas.
            throw new IllegalArgumentException(canvas.getClass().getName()
                    + " is not a RecordingCanvas canvas");
        }
        //webkitDelegate.drawWebViewFunctor(canvas, functor);
        Log.i("crWebView", " WebViewDelegateWrapper drawWebViewFunctor called");
        //canvas.drawRect(20, 20, 300, 300, paint);
        //webkitDelegate.drawWebViewFunctor(canvas, functor);
        try {
            ////Class webViewDelegateClass = Class.forName("android.webkit.WebViewDelegate");
            //Class webViewDelegateClass = Class.forName("android.graphics.RecordingCanvas");
            ////Log.i("crWebView", "drawWebViewFunctor STEP 000");
            ////Object delegate = webViewDelegateClass.newInstance();
            ////Constructor newDelegate = webViewDelegateClass.getConstructor();
            //Log.i("crWebView", "drawWebViewFunctor STEP 001 ");
            ////Constructor newDelegate = webViewDelegateClass.getConstructor();
            //Method drawWebViewFunctor = webViewDelegateClass.getMethod("drawWebViewFunctor", int.class);
            //Log.i("crWebView", "drawWebViewFunctor STEP 002 " );
            ////newDelegate.setAccessible(true);
            //Log.i("crWebView", "drawWebViewFunctor STEP 003 " );
            //Log.i("crWebView", "drawWebViewFunctor STEP 004");
            ////Method drawWebViewFunctor = webViewDelegateClass.getMethod("drawWebViewFunctor");
            //Log.i("crWebView", "drawWebViewFunctor STEP 005");
            //drawWebViewFunctor.invoke(canvas, functor);
            //Log.i("crWebView", "drawWebViewFunctor STEP 006");
            //RecordingCanvas recordingCanvas = (RecordingCanvas) canvas;
            //Log.i("crWebView", "drawWebViewFunctor STEP 001" + canvas + "  status " + (canvas instanceof RecordingCanvas));
            //Class recordingCanvasClass = Class.forName("android.graphics.RecordingCanvas");
            //Log.i("crWebView", "drawWebViewFunctor STEP 002");
            //Method method = recordingCanvasClass.getMethod("drawWebViewFunctor", int.class);
            //Method method = recordingCanvasClass.getMethod("callDrawGLFunction2", long.class);
            //Log.i("crWebView", "drawWebViewFunctor STEP 003" + method);
            //method.invoke(recordingCanvas, functor, null);
            //Log.i("crWebView", "drawWebViewFunctor STEP 004");
            //drawWebViewFunctor.invoke(rc, functor);
            //Object mNativeCanvasWrapper = canvas.getClass().getMethod("getNativeCanvasWrapper").invoke(canvas);
            //Log.i("crWebView", "WebViewDelegateWrapper drawWebViewFunctor mNativeCanvasWrapper " + mNativeCanvasWrapper);
        } catch (Exception e) {
            e.printStackTrace();
            Log.i("crWebView", " WebViewDelegateWrapper drawWebViewFunctor invoke failed ");
        }
		//Log.i("crWebView", " *** WebViewDelegateWrapper drawWebViewFunctor called");
        //((RecordingCanvas) canvas).drawWebViewFunctor(functor);
    }

    /**
     * Detaches the draw GL functor.
     *
     * @param nativeDrawGLFunctor the pointer to the native functor that implements
     *        system/core/include/utils/Functor.h
     * @deprecated Use {@link #drawWebViewFunctor(Canvas, int)}
     */
    @Deprecated
    public void detachDrawGlFunctor(View containerView, long nativeDrawGLFunctor) {
		Log.i("crWebView", "WebViewDelegateWrapper detachDrawGlFunctor called");
    }

    /**
     * Returns the package id of the given {@code packageName}.
     */
    public int getPackageId(Resources resources, String packageName) {
		Log.i("crWebView", "WebViewDelegateWrapper getPackageId called, packageName " + packageName);
        return 0;
    }

    /**
     * Returns the application which is embedding the WebView.
     */
    public Application getApplication() {
        try {
            Class activityThreadClass = Class.forName("android.app.ActivityThread");
            Object currentApplication = activityThreadClass.getMethod("currentApplication").invoke(null);
		    Log.i("crWebView", "WebViewDelegateWrapper getApplication called" + currentApplication);
            return (Application)currentApplication;
        } catch (Exception e) {
            return null;
        }
    }

    /**
     * Returns the error string for the given {@code errorCode}.
     */
    public String getErrorString(Context context, int errorCode) {
        Log.i("crWebView", " getErrorString called ");
        return "crError=" + errorCode ;//LegacyErrorStrings.getString(errorCode, context);
    }

    /**
     * Adds the WebView asset path to {@link android.content.res.AssetManager}.
     * If {@link android.content.res.Flags#FLAG_REGISTER_RESOURCE_PATHS} is enabled, this function
     * will be a no-op because the asset paths appending work will only be handled by
     * {@link android.content.res.Resources#registerResourcePaths(String, ApplicationInfo)},
     * otherwise it behaves the old way.
     */
    public void addWebViewAssetPath(Context context) {
		Log.i("crWebView", "addWebViewAssetPath called");
    }

    /**
     * Returns whether WebView should run in multiprocess mode.
     */
    public boolean isMultiProcessEnabled() {
        return false;
    }

    ///**
    // * Returns the data directory suffix to use, or null for none.
    // */
    public String getDataDirectorySuffix() {
        Log.i("crWebView", " getDataDirectorySuffix called");
        return "";
        //return WebViewFactory.getDataDirectorySuffix();
    }

    /**
     * Get the timestamps at which various WebView startup events occurred in this process.
     * This method must be called on the same thread where the
     * WebViewChromiumFactoryProvider#create method was invoked.
     */
    //public WebViewFactory.StartupTimestamps getStartupTimestamps() {
    //    return WebViewFactory.getStartupTimestamps();
    //}
}
