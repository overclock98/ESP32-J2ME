#include "flint_native_lcd.h"
#include "flint_array_object.h"
#include "flint_system_api.h"
#include "lcd.h"

jvoid NativeLcd_Init(FNIEnv *env) {
    lcd_init();
}

jint NativeLcd_Width(FNIEnv *env) {
    return lcd_width();
}

jint NativeLcd_Height(FNIEnv *env) {
    return lcd_height();
}

jvoid NativeLcd_Present(FNIEnv *env, jobject fb) {
    if(fb == NULL) {
        env->throwNew(env->findClass("java/lang/NullPointerException"), "fb cannot be null");
        return;
    }
    jbyteArray arr = (jbyteArray)fb;
    lcd_present((const uint8_t *)arr->getData(), arr->getLength());
}

/* Pop the next typed console byte (game key), or -1 if none are queued. Non-blocking. */
jint NativeLcd_ReadKey(FNIEnv *env) {
    return FlintAPI::System::consoleInputPop();
}
