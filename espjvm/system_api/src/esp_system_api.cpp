
#include <stdio.h>
#include "sdkconfig.h"
#include "esp_timer.h"
#include "flint_utf8.h"
#include "esp_heap_caps.h"
#include "../heap_private.h"
#include "flint_system_api.h"
#include "freertos/FreeRTOS.h"
#include "driver/usb_serial_jtag.h"
#include "esp_rom_sys.h"
#include <string.h>

void *FlintAPI::System::malloc(uint32_t size) {
    return ::malloc(size);
}

void *FlintAPI::System::realloc(void *p, uint32_t size) {
    return ::realloc(p, size);
}

void FlintAPI::System::free(void *p) {
    ::free(p);
}

void FlintAPI::System::consoleWrite(uint8_t *utf8, uint32_t length) {
    /* NON-BLOCKING write to USB-Serial-JTAG (timeout 0). The blocking version (50ms timeout)
     * stalled the game thread on every log line once the host couldn't drain the TX buffer
     * fast enough under render load — which LOOKED like a freeze. Dropping output when the
     * buffer is full keeps the game running at full speed. */
    usb_serial_jtag_write_bytes((const char *)utf8, length, 0);
}

/* SPSC ring buffer for typed console input (producer: debugger RX task; consumer: the VM
 * input thread via board.Lcd.readKey()). Guarded by a portMUX so it is safe across cores. */
static portMUX_TYPE keyInMux = portMUX_INITIALIZER_UNLOCKED;
#define KEY_IN_BUF_SIZE 64
static uint8_t keyInBuf[KEY_IN_BUF_SIZE];
static volatile uint32_t keyInHead = 0, keyInTail = 0;

void FlintAPI::System::consoleInputPush(const uint8_t *data, uint32_t length) {
    portENTER_CRITICAL(&keyInMux);
    for(uint32_t i = 0; i < length; i++) {
        uint32_t next = (keyInHead + 1) % KEY_IN_BUF_SIZE;
        if(next == keyInTail) break;   /* full: drop the rest */
        keyInBuf[keyInHead] = data[i];
        keyInHead = next;
    }
    portEXIT_CRITICAL(&keyInMux);
}

int FlintAPI::System::consoleInputPop(void) {
    int ret = -1;
    portENTER_CRITICAL(&keyInMux);
    if(keyInTail != keyInHead) {
        ret = keyInBuf[keyInTail];
        keyInTail = (keyInTail + 1) % KEY_IN_BUF_SIZE;
    }
    portEXIT_CRITICAL(&keyInMux);
    return ret;
}

int64_t FlintAPI::System::getTimeNanos(void) {
    return esp_timer_get_time() * 1000;
}

int64_t FlintAPI::System::getTimeMillis(void) {
    return esp_timer_get_time() / 1000;
}

const char *FlintAPI::System::getClassPath(uint32_t index) {
    static const char *jars[] = {
        "/lib/java.base.jar",
        "/lib/flint.io.jar",
        "/lib/flint.net.jar",
        "/lib/flint.drawing.jar",
        "/lib/midp.jar",        /* MIDP/CLDC shim (javax.microedition.*, board.Lcd) */
    };
    if(index < LENGTH(jars))
        return jars[index];
    return NULL;        
}
