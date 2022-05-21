package io.mask.crwebview;

import android.app.Application;
import android.content.Context;

import org.chromium.android_webview.AwLocaleConfig;
import org.chromium.base.CommandLine;
import org.chromium.base.ContextUtils;
import org.chromium.base.PathUtils;
import org.chromium.ui.base.ResourceBundle;


public class AwShellApplication extends Application {
    // Called by the framework for ALL processes. Runs before ContentProviders are created.
    // Quirk: context.getApplicationContext() returns null during this method.
    @Override
    protected void attachBaseContext(Context context) {
        super.attachBaseContext(context);
        ContextUtils.initApplicationContext(this);
        PathUtils.setPrivateDataDirectorySuffix("webview", "WebView");
        CommandLine.initFromFile("/data/local/tmp/android-webview-command-line");
        ResourceBundle.setAvailablePakLocales(AwLocaleConfig.getWebViewSupportedPakLocales());
    }
}
