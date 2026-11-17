#ifndef __FLINT_NATIVE_TOUCH_H
#define __FLINT_NATIVE_TOUCH_H

#include "flint_native.h"

/* Native backing for the MIDP `board.Touch` class:
 *   public final class Touch {
 *       public static native void init();   // I2C touch + draw the on-screen keypad
 *       public static native int  poll();   // ASCII key under the finger, or -1
 *   }
 * Registered in flint_native.cpp as NATIVE_CLASS("board/Touch", touchMethods). */

jvoid NativeTouch_Init(FNIEnv *env);
jint  NativeTouch_Poll(FNIEnv *env);
jvoid NativeTouch_Boost(FNIEnv *env);

inline constexpr NativeMethod touchMethods[] = {
    NATIVE_METHOD("init",  "()V", NativeTouch_Init),
    NATIVE_METHOD("poll",  "()I", NativeTouch_Poll),
    NATIVE_METHOD("boost", "()V", NativeTouch_Boost),
};

#endif /* __FLINT_NATIVE_TOUCH_H */
