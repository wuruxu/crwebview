// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_crwebview.webkit;

//import org.chromium.android_webview.common.Lifetime;
import android.view.Surface;

import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

@JNINamespace("draw_fn")
public class MyDrawFunctor {
    private Surface mCurrentSurface;
    private final long mDrawFunctor;

    public static long getDrawFnFunctionTable() {
        return MyDrawFunctorJni.get().getDrawFnFunctionTable(true);
    }

    public MyDrawFunctor() {
        mDrawFunctor = MyDrawFunctorJni.get().init(true);
    }

    public void setSurface(Surface surface, int width, int height) {
        if (mCurrentSurface == surface) {
            if (surface != null) {
                MyDrawFunctorJni.get().resizeSurface(mDrawFunctor, width, height);
            }
            return;
        }
        mCurrentSurface = surface;
        MyDrawFunctorJni.get().setSurface(mDrawFunctor, surface, width, height);
    }

    public void setOverlaysSurface(Surface surface) {
        MyDrawFunctorJni.get().setOverlaysSurface(mDrawFunctor, surface);
    }

    public void sync(int functor, boolean applyForceDark) {
        MyDrawFunctorJni.get().sync(mDrawFunctor, functor, applyForceDark);
    }

    // Uses functor from last sync.
    public int[] draw(int width, int height, int scrollX, int scrollY, boolean readbackQuadrants) {
        return MyDrawFunctorJni.get()
                .draw(mDrawFunctor, width, height, scrollX, scrollY, readbackQuadrants);
    }

    @NativeMethods
    interface Natives {
        long getDrawFnFunctionTable(boolean useVulkan);

        long init(boolean useVulkan);

        void setSurface(long nativeDrawFunctor, Surface surface, int width, int height);

        void resizeSurface(long nativeDrawFunctor, int width, int height);

        void setOverlaysSurface(long nativeDrawFunctor, Surface surface);

        void sync(long nativeDrawFunctor, int functor, boolean applyForceDark);

        int[] draw(long nativeDrawFunctor, int width, int height, int scrollX, int scrollY, boolean readbackQuadrants);
    }
}
