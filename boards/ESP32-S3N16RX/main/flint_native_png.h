#ifndef __FLINT_NATIVE_PNG_H
#define __FLINT_NATIVE_PNG_H

#include "flint_native.h"

/* Native PNG decoder for the MIDP image path. Backs:
 *   package board;
 *   public final class Png {
 *       // Decodes a PNG into Flint's RGB565 / ARGB565 byte layout (BGR-packed big-endian,
 *       // ARGB565 = RGB plane + appended 4-bit alpha plane). Writes {w,h,format} into wh[0..2]
 *       // (format 2=RGB565, 3=ARGB565) and returns the pixel buffer, or null on failure
 *       // (caller falls back to a placeholder).
 *       public static native byte[] decode(byte[] data, int off, int len, int[] wh);
 *   }
 * Inflate uses the ESP32-S3 ROM miniz (tinfl_decompress_mem_to_mem). Non-interlaced only. */

jobject NativePng_Decode(FNIEnv *env, jobject data, jint off, jint len, jobject wh);

inline constexpr NativeMethod pngMethods[] = {
    NATIVE_METHOD("decode", "([BII[I)[B", NativePng_Decode),
};

#endif /* __FLINT_NATIVE_PNG_H */
