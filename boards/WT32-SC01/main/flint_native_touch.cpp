#include "flint_native_touch.h"
#include "touch.h"
#include "lcd.h"
#include "flint_system_api.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

/* consoleWrite is the SAME visible channel the boot lines use (usb_serial_jtag direct). IDF's
 * ESP_LOGx / esp_rom_printf go to a console that is silent here, so all touch diagnostics use
 * this. No snprintf — integers are formatted by hand (newlib vfprintf can fault on a task stack). */
static void cwrite(const char *s) { FlintAPI::System::consoleWrite((uint8_t *)s, (uint32_t)strlen(s)); }

static int put_int(char *out, int v) {
    char tmp[12]; int n = 0, p = 0; unsigned uv;
    if (v < 0) { out[p++] = '-'; uv = (unsigned)(-v); } else uv = (unsigned)v;
    if (uv == 0) tmp[n++] = '0';
    while (uv) { tmp[n++] = (char)('0' + uv % 10); uv /= 10; }
    while (n) out[p++] = tmp[--n];
    return p;
}

/* C-linkage logging hooks called from the native touch task (touch.c). */
extern "C" void touch_log(const char *s) { cwrite(s); }
extern "C" void touch_log_xy(int x, int y, int idx) {
    char b[48]; int p = 0;
    const char *pfx = "[TOUCH x=";  while (*pfx) b[p++] = *pfx++;
    p += put_int(b + p, x);
    b[p++] = ' '; b[p++] = 'y'; b[p++] = '=';                 p += put_int(b + p, y);
    b[p++] = ' '; b[p++] = 'i'; b[p++] = 'd'; b[p++] = 'x'; b[p++] = '='; p += put_int(b + p, idx);
    b[p++] = ']'; b[p++] = '\n';
    FlintAPI::System::consoleWrite((uint8_t *)b, (uint32_t)p);
}

/* Bring up the I2C touch controller (+ its polling task) and paint the on-screen keypad. */
jvoid NativeTouch_Init(FNIEnv *env) {
    bool ok = bsp_touch_init();
    cwrite(ok ? "[touch init OK]\n" : "[touch init FAILED - touch disabled]\n");
    lcd_keypad_init();
}

/* Raise the calling (Java input) thread above the game thread (prio 1) so it can't be starved
 * by the game's flat-out render loop — that starvation is why keys never reached the game. */
jvoid NativeTouch_Boost(FNIEnv *env) {
    vTaskPrioritySet(NULL, 2);
    cwrite("[input thread boosted]\n");
}

/* Cheap shared read — the native task does the I2C work, so this never blocks. Logs each change
 * so we can confirm the Java input thread (Main$1) is actually polling and receiving keys: if
 * [TOUCH ...] appears (task) but [deliver ...] does not, the Java thread is starved. */
jint NativeTouch_Poll(FNIEnv *env) {
    int k = touch_poll_key();
    static int prevk = -1;
    if (k != prevk) {
        char b[28]; int p = 0;
        const char *pfx = "[deliver k=";  while (*pfx) b[p++] = *pfx++;
        p += put_int(b + p, k);
        b[p++] = ']'; b[p++] = '\n';
        FlintAPI::System::consoleWrite((uint8_t *)b, (uint32_t)p);
        prevk = k;
    }
    return k;
}
