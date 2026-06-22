#include "touch.h"
#include "lcd.h"

#include "esp_err.h"
#include "driver/i2c_master.h"     /* NEW i2c_master driver — what the QR build uses to read this
                                    * touch. flint.io links the legacy driver; the boot conflict is
                                    * disabled via CONFIG_I2C_SKIP_LEGACY_CONFLICT_CHECK. */
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_axs15231b.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* ── Touch pins (from the QR project's board.h) ──────────────────────────── */
#define TOUCH_I2C_PORT   I2C_NUM_0
#define PIN_TOUCH_SDA     4
#define PIN_TOUCH_SCL     8
#define TOUCH_I2C_HZ     (400 * 1000)

/* Visible logging (consoleWrite) lives in flint_native_touch.cpp — IDF's ESP_LOG / esp_rom_printf
 * are silent on this firmware's console. */
extern void touch_log(const char *s);
extern void touch_log_xy(int x, int y, int idx);

static esp_lcd_touch_handle_t s_tp = NULL;
static volatile int s_touch_key = -1;   /* ASCII key under the finger, or -1 — read by the Java poll */

bool touch_get_point(int *x, int *y)
{
    if (!s_tp) return false;
    esp_lcd_touch_point_data_t point = {0};
    uint8_t cnt = 0;
    esp_lcd_touch_read_data(s_tp);
    esp_lcd_touch_get_data(s_tp, &point, &cnt, 1);
    if (cnt > 0) {
        if (x) *x = point.x;
        if (y) *y = point.y;
        return true;
    }
    return false;
}

/* Dedicated polling task (own core + priority) so a slow/stuck I2C read never starves the game
 * or the Java input thread. Updates the highlight and the shared key; logs liveness + taps. */
static void touch_task(void *arg)
{
    touch_log("[touch task started]\n");
    int prevIdx = -1;
    uint32_t calls = 0;
    while (1) {
        int x = -1, y = -1, idx = -1;
        if (touch_get_point(&x, &y)) idx = lcd_keypad_hit(x, y);
        if (idx != prevIdx) {
            if (idx >= 0) touch_log_xy(x, y, idx);   /* log presses only — releases were just spam */
            lcd_keypad_set_active(idx);
            s_touch_key = (idx >= 0) ? lcd_keypad_key(idx) : -1;
            prevIdx = idx;
        }
        if ((++calls % 150) == 0) touch_log("[touch task alive]\n");   /* ~every 3s */
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

bool touch_init(void)
{
    if (s_tp) return true;
    esp_err_t err;

    i2c_master_bus_handle_t bus = NULL;
    const i2c_master_bus_config_t i2c_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port   = TOUCH_I2C_PORT,
        .sda_io_num = PIN_TOUCH_SDA,
        .scl_io_num = PIN_TOUCH_SCL,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    err = i2c_new_master_bus(&i2c_cfg, &bus);
    if (err != ESP_OK) { touch_log("[touch] i2c_new_master_bus FAIL\n"); return false; }

    esp_lcd_panel_io_handle_t tp_io = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_cfg = ESP_LCD_TOUCH_IO_I2C_AXS15231B_CONFIG();
    tp_io_cfg.scl_speed_hz = TOUCH_I2C_HZ;
    err = esp_lcd_new_panel_io_i2c(bus, &tp_io_cfg, &tp_io);   /* bus handle -> v2 (new driver) */
    if (err != ESP_OK) { touch_log("[touch] panel_io_i2c FAIL\n"); return false; }

    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = LCD_W,
        .y_max = LCD_H,
        .rst_gpio_num = -1,
        .int_gpio_num = -1,
        .levels = { .reset = 0, .interrupt = 0 },
        .flags  = { .swap_xy = 0, .mirror_x = 0, .mirror_y = 0 },
    };
    err = esp_lcd_touch_new_i2c_axs15231b(tp_io, &tp_cfg, &s_tp);
    if (err != ESP_OK) { touch_log("[touch] touch_new FAIL\n"); s_tp = NULL; return false; }

    /* Poll on APP_CPU (core 1), priority 5 — above the game thread so it runs reliably. */
    xTaskCreatePinnedToCore(touch_task, "touch", 4096, NULL, 5, NULL, 1);
    return true;
}

int touch_poll_key(void) { return s_touch_key; }
