#include "touch.h"
#include "lcd.h"

#include "esp_err.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_touch_ft5x06.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* ── WT32-SC01 V3.4 Touch Pins ──────────────────────────────────────────── */
#define TOUCH_I2C_PORT   I2C_NUM_0
#define PIN_TOUCH_SDA     18
#define PIN_TOUCH_SCL     19
#define PIN_TOUCH_INT     39     /* SENSOR_VN, input-only */
#define TOUCH_I2C_HZ     (400 * 1000)

extern void touch_log(const char *s);
extern void touch_log_xy(int x, int y, int idx);

static esp_lcd_touch_handle_t s_tp = NULL;
static volatile int s_touch_key = -1;   /* ASCII key under finger, or -1 */

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

/* Dedicated polling task on core 1 so I2C reads never starve the game thread */
static void touch_task(void *arg)
{
    touch_log("[touch task started]\n");
    int prevIdx = -1;
    uint32_t calls = 0;
    while (1) {
        int x = -1, y = -1, idx = -1;
        if (touch_get_point(&x, &y)) idx = lcd_keypad_hit(x, y);
        if (idx != prevIdx) {
            if (idx >= 0) touch_log_xy(x, y, idx);
            lcd_keypad_set_active(idx);
            s_touch_key = (idx >= 0) ? lcd_keypad_key(idx) : -1;
            prevIdx = idx;
        }
        if ((++calls % 150) == 0) touch_log("[touch task alive]\n");
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

bool bsp_touch_init(void)
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
        .flags.enable_internal_pullup = false,  /* Board has onboard 10k pull-ups R13, R14 */
    };
    err = i2c_new_master_bus(&i2c_cfg, &bus);
    if (err != ESP_OK) { touch_log("[touch] i2c_new_master_bus FAIL\n"); return false; }

    esp_lcd_panel_io_handle_t tp_io = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_cfg = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();
    tp_io_cfg.scl_speed_hz = TOUCH_I2C_HZ;
    err = esp_lcd_new_panel_io_i2c(bus, &tp_io_cfg, &tp_io);
    if (err != ESP_OK) { touch_log("[touch] panel_io_i2c FAIL\n"); return false; }

    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = LCD_W,
        .y_max = LCD_H,
        .rst_gpio_num = -1,
        .int_gpio_num = PIN_TOUCH_INT,
        .levels = { .reset = 0, .interrupt = 0 },
        .flags  = { .swap_xy = 0, .mirror_x = 0, .mirror_y = 0 },
    };
    err = esp_lcd_touch_new_i2c_ft5x06(tp_io, &tp_cfg, &s_tp);
    if (err != ESP_OK) { touch_log("[touch] touch_new_i2c_ft5x06 FAIL\n"); s_tp = NULL; return false; }

    xTaskCreatePinnedToCore(touch_task, "touch", 4096, NULL, 5, NULL, 1);
    return true;
}

int touch_poll_key(void) { return s_touch_key; }
