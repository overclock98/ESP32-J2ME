
#include <stdio.h>
#include "flint.h"
#include "esp_system.h"
#include "esp_vfs_fat.h"
#include "esp_board.h"
#include "esp_debugger.h"
#include "esp_heap_caps.h"
#include "flint_system_api.h"

static Flint flint;

extern "C" void app_main() {
    static const char *defaultApp = "FlintApp.jar";
    static const esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 16,
        .allocation_unit_size = CONFIG_WL_SECTOR_SIZE,
        .disk_status_check_enable = false,
        .use_one_fat = false
    };
    static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;

    Board_Init();

    ESP_ERROR_CHECK(esp_vfs_fat_spiflash_mount_rw_wl("", "storage", &mount_config, &s_wl_handle));

    EspDbg::getInstance()->setTarget(&flint);

    flint.println();
    flint.setCwd("/");

    {   /* Firmware-identity + baseline heap marker. If you see "PSRAM-malloc" the new build
         * (ALWAYSINTERNAL=0) is live; internal should now read ~200KB, not ~32KB. */
        char b[96];
        int n = snprintf(b, sizeof(b), "[boot v2 PSRAM-malloc] internal=%u psram=%u\n",
            (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
            (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
        FlintAPI::System::consoleWrite((uint8_t *)b, n);
    }

    if(esp_reset_reason() != ESP_RST_PANIC) {
        if(FlintAPI::IO::finfo(defaultApp, NULL) == FlintAPI::IO::FILE_RESULT_OK) {
            flint.setProgram(defaultApp);
            flint.start();
        }
    }

    vTaskPrioritySet(NULL, 2);
    EspDbg::getInstance()->receiveTask();
}
