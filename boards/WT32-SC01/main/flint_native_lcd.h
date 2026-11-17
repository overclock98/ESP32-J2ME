#ifndef __FLINT_NATIVE_LCD_H
#define __FLINT_NATIVE_LCD_H

#include "flint_native.h"

/* Native backing for the app-side `Lcd` class (default package).
 *   public final class Lcd {
 *       public static native void init();
 *       public static native int  width();
 *       public static native int  height();
 *       public static native void present(byte[] fb);
 *   }
 * Registered in flint_native.cpp as NATIVE_CLASS("Lcd", lcdMethods). */

jvoid NativeLcd_Init(FNIEnv *env);
jint  NativeLcd_Width(FNIEnv *env);
jint  NativeLcd_Height(FNIEnv *env);
jvoid NativeLcd_Present(FNIEnv *env, jobject fb);
jint  NativeLcd_ReadKey(FNIEnv *env);

inline constexpr NativeMethod lcdMethods[] = {
    NATIVE_METHOD("init",    "()V",   NativeLcd_Init),
    NATIVE_METHOD("width",   "()I",   NativeLcd_Width),
    NATIVE_METHOD("height",  "()I",   NativeLcd_Height),
    NATIVE_METHOD("present", "([B)V", NativeLcd_Present),
    NATIVE_METHOD("readKey", "()I",   NativeLcd_ReadKey),
};

#endif /* __FLINT_NATIVE_LCD_H */
