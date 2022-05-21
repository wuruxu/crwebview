package io.mask.crwebview;
import android.content.Context;

import org.chromium.android_webview.common.AwResource;


public class AwShellResourceProvider {
    private static boolean sInitialized;

    public static void registerResources(Context context) {
        if (sInitialized) {
            return;
        }

        AwResource.setResources(context.getResources());

        AwResource.setConfigKeySystemUuidMapping(R.array.config_key_system_uuid_mapping);

        sInitialized = true;
    }
}
