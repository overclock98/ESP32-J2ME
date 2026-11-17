
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/idf_additions.h"
#include "esp_heap_caps.h"
#include "flint_system_api.h"

using namespace FlintAPI::Thread;

ThreadHandle FlintAPI::Thread::create(void (*task)(void *), void *param, uint32_t stackSize) {
    TaskHandle_t xHandle = NULL;
#if CONFIG_IDF_TARGET_ESP32
    /* On classic ESP32 (LX6), flash operations disable the PSRAM cache, requiring the task
     * stack to reside in internal DRAM (esp_ptr_in_dram). 32KB internal SRAM is plenty for
     * the interpreter and deep call chains while leaving ~170KB+ internal RAM for DMA. */
    uint32_t nativeStack = (stackSize > 32768) ? stackSize : 32768;   /* 32KB in internal DRAM */
    if(xTaskCreateWithCaps(task, "FlintJavaThread", nativeStack, param, tskIDLE_PRIORITY + 1, &xHandle, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT) != pdPASS)
        return NULL;
#else
    /* On ESP32-S3, PSRAM has independent caching and can host the large 96KB stack */
    uint32_t nativeStack = (stackSize > 98304) ? stackSize : 98304;   /* 96KB in PSRAM */
    if(xTaskCreateWithCaps(task, "FlintJavaThread", nativeStack, param, tskIDLE_PRIORITY + 1, &xHandle, MALLOC_CAP_SPIRAM) != pdPASS)
        return NULL;
#endif
    return (void *)xHandle;
}

ThreadHandle FlintAPI::Thread::getCurrentThread(void) {
    return (void *)xTaskGetCurrentTaskHandle();
}

void FlintAPI::Thread::terminate(ThreadHandle handle) {
    vTaskDeleteWithCaps((TaskHandle_t)handle);
}

void FlintAPI::Thread::sleep(uint32_t ms) {
    TickType_t tick = pdMS_TO_TICKS(ms);
    vTaskDelay((tick < 1) ? 1 : tick);
}

void FlintAPI::Thread::yield(void) {
    vTaskDelay(1);
}

bool FlintAPI::Thread::wait(uint32_t ms, uint32_t *notifyValue) {
    if(ms > 0)
        return xTaskNotifyWait(0, ULONG_MAX, notifyValue, pdMS_TO_TICKS(ms));
    else
        return xTaskNotifyWait(0, ULONG_MAX, notifyValue, portMAX_DELAY);
}

void FlintAPI::Thread::notify(ThreadHandle handle, uint32_t notifyValue) {
    xTaskNotify((TaskHandle_t)handle, notifyValue, eSetValueWithOverwrite);
}
