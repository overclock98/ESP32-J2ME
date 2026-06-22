/* Thin Java handle to the board LCD. Native methods are backed in firmware by
 * boards/ESP32-S3N16RX/main/flint_native_lcd.cpp, registered as NATIVE_CLASS("Lcd", ...). */
public final class Lcd {
    public static native void init();
    public static native int  width();
    public static native int  height();
    /** Push a full big-endian RGB565 framebuffer (width*height*2 bytes) to the panel. */
    public static native void present(byte[] fb);

    private Lcd() {}
}
