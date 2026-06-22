#include "lcd.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_heap_caps.h"
#include "esp_rom_sys.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_axs15231b.h"

/* ── Board pin map (from the QR project's board.h) ─────────────────────────── */
#define LCD_HOST          SPI2_HOST
#define LCD_BIT_PER_PIXEL 16
#define PIN_LCD_CS        45
#define PIN_LCD_PCLK      47
#define PIN_LCD_D0        21
#define PIN_LCD_D1        48
#define PIN_LCD_D2        40
#define PIN_LCD_D3        39
#define PIN_LCD_RST       -1
#define PIN_LCD_BL         1     /* backlight, active high */
#define BOUNCE_LINES      40     /* lines per DMA strip through internal SRAM */

static esp_lcd_panel_handle_t    s_panel    = NULL;
static esp_lcd_panel_io_handle_t s_io       = NULL;
static uint16_t                 *s_bounce   = NULL;   /* DMA-capable internal SRAM */
static SemaphoreHandle_t         s_flush_done = NULL;
static bool                      s_inited   = false;

/* On-screen keypad: a LCD_W x KEYPAD_H big-endian RGB565 buffer (same encoding as the
 * game framebuffer) composited into the bottom of every frame by lcd_present(). */
static uint8_t *s_keypad     = NULL;
static uint8_t *s_softkey    = NULL;
static int      s_kp_active  = -1;     /* 0-11 digit, 12 CALL, 13 END, or -1 none */
static void     lcd_keypad_draw(void);
static void     lcd_softkey_draw(void);

/* ── Vendor init sequence (Guition AXS15231B) ─────────────────────────────── */
static const axs15231b_lcd_init_cmd_t lcd_init_cmds[] = {
    {0xBB, (uint8_t[]){0x00,0x00,0x00,0x00,0x00,0x00,0x5A,0xA5}, 8, 0},
    {0xA0, (uint8_t[]){0xC0,0x10,0x00,0x02,0x00,0x00,0x04,0x3F,0x20,0x05,0x3F,0x3F,0x00,0x00,0x00,0x00,0x00}, 17, 0},
    {0xA2, (uint8_t[]){0x30,0x3C,0x24,0x14,0xD0,0x20,0xFF,0xE0,0x40,0x19,0x80,0x80,0x80,0x20,0xF9,0x10,0x02,0xFF,0xFF,0xF0,0x90,0x01,0x32,0xA0,0x91,0xE0,0x20,0x7F,0xFF,0x00,0x5A}, 31, 0},
    {0xD0, (uint8_t[]){0xE0,0x40,0x51,0x24,0x08,0x05,0x10,0x01,0x20,0x15,0x42,0xC2,0x22,0x22,0xAA,0x03,0x10,0x12,0x60,0x14,0x1E,0x51,0x15,0x00,0x8A,0x20,0x00,0x03,0x3A,0x12}, 30, 0},
    {0xA3, (uint8_t[]){0xA0,0x06,0xAA,0x00,0x08,0x02,0x0A,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x00,0x55,0x55}, 22, 0},
    {0xC1, (uint8_t[]){0x31,0x04,0x02,0x02,0x71,0x05,0x24,0x55,0x02,0x00,0x41,0x00,0x53,0xFF,0xFF,0xFF,0x4F,0x52,0x00,0x4F,0x52,0x00,0x45,0x3B,0x0B,0x02,0x0D,0x00,0xFF,0x40}, 30, 0},
    {0xC3, (uint8_t[]){0x00,0x00,0x00,0x50,0x03,0x00,0x00,0x00,0x01,0x80,0x01}, 11, 0},
    {0xC4, (uint8_t[]){0x00,0x24,0x33,0x80,0x00,0xEA,0x64,0x32,0xC8,0x64,0xC8,0x32,0x90,0x90,0x11,0x06,0xDC,0xFA,0x00,0x00,0x80,0xFE,0x10,0x10,0x00,0x0A,0x0A,0x44,0x50}, 29, 0},
    {0xC5, (uint8_t[]){0x18,0x00,0x00,0x03,0xFE,0x3A,0x4A,0x20,0x30,0x10,0x88,0xDE,0x0D,0x08,0x0F,0x0F,0x01,0x3A,0x4A,0x20,0x10,0x10,0x00}, 23, 0},
    {0xC6, (uint8_t[]){0x05,0x0A,0x05,0x0A,0x00,0xE0,0x2E,0x0B,0x12,0x22,0x12,0x22,0x01,0x03,0x00,0x3F,0x6A,0x18,0xC8,0x22}, 20, 0},
    {0xC7, (uint8_t[]){0x50,0x32,0x28,0x00,0xA2,0x80,0x8F,0x00,0x80,0xFF,0x07,0x11,0x9C,0x67,0xFF,0x24,0x0C,0x0D,0x0E,0x0F}, 20, 0},
    {0xC9, (uint8_t[]){0x33,0x44,0x44,0x01}, 4, 0},
    {0xCF, (uint8_t[]){0x2C,0x1E,0x88,0x58,0x13,0x18,0x56,0x18,0x1E,0x68,0x88,0x00,0x65,0x09,0x22,0xC4,0x0C,0x77,0x22,0x44,0xAA,0x55,0x08,0x08,0x12,0xA0,0x08}, 27, 0},
    {0xD5, (uint8_t[]){0x40,0x8E,0x8D,0x01,0x35,0x04,0x92,0x74,0x04,0x92,0x74,0x04,0x08,0x6A,0x04,0x46,0x03,0x03,0x03,0x03,0x82,0x01,0x03,0x00,0xE0,0x51,0xA1,0x00,0x00,0x00}, 30, 0},
    {0xD6, (uint8_t[]){0x10,0x32,0x54,0x76,0x98,0xBA,0xDC,0xFE,0x93,0x00,0x01,0x83,0x07,0x07,0x00,0x07,0x07,0x00,0x03,0x03,0x03,0x03,0x03,0x03,0x00,0x84,0x00,0x20,0x01,0x00}, 30, 0},
    {0xD7, (uint8_t[]){0x03,0x01,0x0B,0x09,0x0F,0x0D,0x1E,0x1F,0x18,0x1D,0x1F,0x19,0x40,0x8E,0x04,0x00,0x20,0xA0,0x1F}, 19, 0},
    {0xD8, (uint8_t[]){0x02,0x00,0x0A,0x08,0x0E,0x0C,0x1E,0x1F,0x18,0x1D,0x1F,0x19}, 12, 0},
    {0xD9, (uint8_t[]){0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F}, 12, 0},
    {0xDD, (uint8_t[]){0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F}, 12, 0},
    {0xDF, (uint8_t[]){0x44,0x73,0x4B,0x69,0x00,0x0A,0x02,0x90}, 8, 0},
    {0xE0, (uint8_t[]){0x3B,0x28,0x10,0x16,0x0C,0x06,0x11,0x28,0x5C,0x21,0x0D,0x35,0x13,0x2C,0x33,0x28,0x0D}, 17, 0},
    {0xE1, (uint8_t[]){0x37,0x28,0x10,0x16,0x0B,0x06,0x11,0x28,0x5C,0x21,0x0D,0x35,0x14,0x2C,0x33,0x28,0x0F}, 17, 0},
    {0xE2, (uint8_t[]){0x3B,0x07,0x12,0x18,0x0E,0x0D,0x17,0x35,0x44,0x32,0x0C,0x14,0x14,0x36,0x3A,0x2F,0x0D}, 17, 0},
    {0xE3, (uint8_t[]){0x37,0x07,0x12,0x18,0x0E,0x0D,0x17,0x35,0x44,0x32,0x0C,0x14,0x14,0x36,0x32,0x2F,0x0F}, 17, 0},
    {0xE4, (uint8_t[]){0x3B,0x07,0x12,0x18,0x0E,0x0D,0x17,0x39,0x44,0x2E,0x0C,0x14,0x14,0x36,0x3A,0x2F,0x0D}, 17, 0},
    {0xE5, (uint8_t[]){0x37,0x07,0x12,0x18,0x0E,0x0D,0x17,0x39,0x44,0x2E,0x0C,0x14,0x14,0x36,0x3A,0x2F,0x0F}, 17, 0},
    {0xA4, (uint8_t[]){0x85,0x85,0x95,0x82,0xAF,0xAA,0xAA,0x80,0x10,0x30,0x40,0x40,0x20,0xFF,0x60,0x30}, 16, 0},
    {0xA4, (uint8_t[]){0x85,0x85,0x95,0x85}, 4, 0},
    {0xBB, (uint8_t[]){0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, 8, 0},
    {0x13, (uint8_t[]){0x00}, 0, 0},
    {0x11, (uint8_t[]){0x00}, 0, 120},   /* sleep out, 120 ms */
    {0x29, (uint8_t[]){0x00}, 0, 10},    /* display on */
};

/* Fires from ISR when one DMA strip finishes. */
static bool IRAM_ATTR on_color_trans_done(esp_lcd_panel_io_handle_t io,
                                          esp_lcd_panel_io_event_data_t *e,
                                          void *user_ctx)
{
    BaseType_t hpw = pdFALSE;
    xSemaphoreGiveFromISR(s_flush_done, &hpw);
    return hpw == pdTRUE;
}

static void backlight_init(void)
{
    gpio_config_t bk = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << PIN_LCD_BL,
    };
    gpio_config(&bk);
    gpio_set_level(PIN_LCD_BL, 1);
}

void lcd_init(void)
{
    if (s_inited) return;

    const spi_bus_config_t bus_cfg = AXS15231B_PANEL_BUS_QSPI_CONFIG(
        PIN_LCD_PCLK,
        PIN_LCD_D0, PIN_LCD_D1, PIN_LCD_D2, PIN_LCD_D3,
        LCD_W * BOUNCE_LINES * sizeof(uint16_t) + 8);
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_spi_config_t io_cfg =
        AXS15231B_PANEL_IO_QSPI_CONFIG(PIN_LCD_CS, NULL, NULL);
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(
        (esp_lcd_spi_bus_handle_t)LCD_HOST, &io_cfg, &s_io));

    const axs15231b_vendor_config_t vendor_cfg = {
        .init_cmds      = lcd_init_cmds,
        .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0]),
        .flags = { .use_qspi_interface = 1 },
    };
    const esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = PIN_LCD_RST,
        /* BGR (not RGB like the LVGL/QR driver): flint.drawing.ColorRgb565 packs
         * pixels as BGR565, so the panel must swap R/B back at the hardware level. */
        .rgb_ele_order  = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = LCD_BIT_PER_PIXEL,
        .vendor_config  = (void *)&vendor_cfg,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_axs15231b(s_io, &panel_cfg, &s_panel));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));
    /* Do NOT call esp_lcd_panel_disp_on_off(): vendor init already did 0x11/0x29
     * and re-issuing 0x29 can wedge this panel. */

    const size_t bounce_bytes = LCD_W * BOUNCE_LINES * sizeof(uint16_t);
    s_bounce = heap_caps_malloc(bounce_bytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    assert(s_bounce);

    /* Keypad pixel buffer (PSRAM is fine — present memcpy's it into the internal bounce
     * buffer before DMA, just like the game framebuffer). Starts black until drawn. */
    s_keypad = heap_caps_malloc((size_t)LCD_W * KEYPAD_H * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (s_keypad) memset(s_keypad, 0, (size_t)LCD_W * KEYPAD_H * sizeof(uint16_t));

    s_flush_done = xSemaphoreCreateBinary();
    const esp_lcd_panel_io_callbacks_t cbs = { .on_color_trans_done = on_color_trans_done };
    ESP_ERROR_CHECK(esp_lcd_panel_io_register_event_callbacks(s_io, &cbs, NULL));

    backlight_init();
    s_inited = true;
}

int lcd_width(void)  { return LCD_W; }
int lcd_height(void) { return GAME_H; }   /* the game owns only the top region */

/* ── On-screen touch keypad ───────────────────────────────────────────────
 * Numpad (3 cols x 4 rows, phone order) drawn into s_keypad (bottom strip). The number rows
 * (1-9) are tall (frequently used); the * 0 # row is a normal height. Two Nokia-style soft keys
 * (CALL/END) are a half-width vertical stack (s_softkey) overlaid on the game's bottom-right.
 *      1 2 3                          [CALL]  idx 12 -> -6 (left soft / OK)
 *      4 5 6                          [END ]  idx 13 -> -7 (right soft / Back)
 *      7 8 9
 *      * 0 #     ('*','#' are real key codes 42/35)
 * For digit and star/hash buttons the label IS the injected key code. */
static const char KP_KEYS[12] = { '1','2','3', '4','5','6', '7','8','9', '*','0','#' };

#define KP_DIGIT_ROW_H 42                        /* tall number rows (1-9) */
#define KP_BOTTOM_Y    (3 * KP_DIGIT_ROW_H)      /* 126 */
#define KP_BOTTOM_H    (KEYPAD_H - KP_BOTTOM_Y)  /* 34 — normal-height * 0 # row */

#define SK_W      56                             /* ~half a numpad key wide */
#define SK_H      30
#define SK_TOTAL  (2 * SK_H)                     /* two stacked buttons */
#define SK_X0     (LCD_W - SK_W)                 /* right edge */
#define SK_Y0     (GAME_H - SK_TOTAL)            /* just above the keypad strip */

/* Minimal 5x7 glyphs (row-major, MSB = leftmost column). */
static const uint8_t *glyph5x7(char c) {
    static const uint8_t G[10][7] = {
        {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}, /*0*/ {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}, /*1*/
        {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}, /*2*/ {0x1F,0x02,0x04,0x02,0x01,0x11,0x0E}, /*3*/
        {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}, /*4*/ {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}, /*5*/
        {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}, /*6*/ {0x1F,0x01,0x02,0x04,0x08,0x08,0x08}, /*7*/
        {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}, /*8*/ {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}, /*9*/
    };
    static const uint8_t STAR[7] = {0x00,0x15,0x0E,0x1F,0x0E,0x15,0x00};
    static const uint8_t HASH[7] = {0x0A,0x0A,0x1F,0x0A,0x1F,0x0A,0x0A};
    static const uint8_t O[7]    = {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E};
    static const uint8_t K[7]    = {0x11,0x12,0x14,0x18,0x14,0x12,0x11};
    static const uint8_t B[7]    = {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E};
    if (c >= '0' && c <= '9') return G[c - '0'];
    if (c == '*') return STAR;
    if (c == '#') return HASH;
    if (c == 'O') return O;
    if (c == 'K') return K;
    if (c == 'B') return B;
    return NULL;
}

static inline uint16_t kp_rgb(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint16_t)(b >> 3) << 11) | ((uint16_t)(g >> 2) << 5) | (r >> 3);  /* BGR565 */
}
/* Generic draw into any big-endian RGB565 buffer (keypad or soft-key). */
static inline void blit_px(uint8_t *buf, int bw, int bh, int x, int y, uint16_t v) {
    if ((unsigned)x >= (unsigned)bw || (unsigned)y >= (unsigned)bh) return;
    uint8_t *p = &buf[((size_t)y * bw + x) * 2];
    p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)(v & 0xFF);   /* big-endian, like the framebuffer */
}
static void blit_fill(uint8_t *buf, int bw, int bh, int x0, int y0, int w, int h, uint16_t v) {
    for (int y = y0; y < y0 + h; y++) for (int x = x0; x < x0 + w; x++) blit_px(buf, bw, bh, x, y, v);
}
static void blit_border(uint8_t *buf, int bw, int bh, int x0, int y0, int w, int h, uint16_t v) {
    blit_fill(buf,bw,bh, x0,y0, w,1, v); blit_fill(buf,bw,bh, x0,y0+h-1, w,1, v);
    blit_fill(buf,bw,bh, x0,y0, 1,h, v); blit_fill(buf,bw,bh, x0+w-1,y0, 1,h, v);
}
static void blit_glyph(uint8_t *buf, int bw, int bh, char ch, int x0, int y0, int scale, uint16_t v) {
    const uint8_t *g = glyph5x7(ch); if (!g) return;
    for (int row = 0; row < 7; row++) for (int col = 0; col < 5; col++)
        if (g[row] & (1 << (4 - col))) blit_fill(buf, bw, bh, x0 + col*scale, y0 + row*scale, scale, scale, v);
}

static void lcd_keypad_draw(void) {
    if (!s_keypad) return;
    const uint16_t bg=kp_rgb(16,16,24), face=kp_rgb(56,56,76), faceA=kp_rgb(120,170,255),
                   border=kp_rgb(95,95,125), label=kp_rgb(240,240,240), labelA=kp_rgb(10,12,30);
    blit_fill(s_keypad, LCD_W, KEYPAD_H, 0,0, LCD_W, KEYPAD_H, bg);
    const int cellW = LCD_W / 3, m = 2;
    for (int idx = 0; idx < 12; idx++) {
        int r = idx / 3, c = idx % 3;
        int y0, h, sc;
        if (r < 3) { y0 = r * KP_DIGIT_ROW_H; h = KP_DIGIT_ROW_H; sc = 4; }  /* big number keys */
        else       { y0 = KP_BOTTOM_Y;        h = KP_BOTTOM_H;    sc = 3; }  /* normal * 0 # */
        int x0 = c * cellW, w = cellW, on = (idx == s_kp_active);
        blit_fill(s_keypad, LCD_W, KEYPAD_H, x0+m, y0+m, w-2*m, h-2*m, on?faceA:face);
        blit_border(s_keypad, LCD_W, KEYPAD_H, x0+m, y0+m, w-2*m, h-2*m, border);
        blit_glyph(s_keypad, LCD_W, KEYPAD_H, KP_KEYS[idx], x0+(w-5*sc)/2, y0+(h-7*sc)/2, sc, on?labelA:label);
    }
}

static void lcd_softkey_draw(void) {
    if (!s_softkey) return;
    const uint16_t white = kp_rgb(255,255,255);
    for (int b = 0; b < 2; b++) {                  /* b=0 CALL (green), b=1 END (red) */
        int y0 = b * SK_H, on = (s_kp_active == 12 + b), m = 2, sc = 2;
        uint16_t face = (b == 0) ? kp_rgb(0, on?235:150, 40)
                                 : kp_rgb(on?255:190, 35, 35);
        blit_fill(s_softkey, SK_W, SK_TOTAL, m, y0+m, SK_W-2*m, SK_H-2*m, face);
        blit_border(s_softkey, SK_W, SK_TOTAL, m, y0+m, SK_W-2*m, SK_H-2*m, white);
        const char *lbl = (b == 0) ? "OK" : "BK";
        int cw = 5*sc, gap = sc, tw = 2*cw + gap, gh = 7*sc;
        int gx = (SK_W - tw)/2, gy = y0 + (SK_H - gh)/2;
        blit_glyph(s_softkey, SK_W, SK_TOTAL, lbl[0], gx, gy, sc, white);
        blit_glyph(s_softkey, SK_W, SK_TOTAL, lbl[1], gx+cw+gap, gy, sc, white);
    }
}

void lcd_keypad_init(void) {
    if (!s_softkey) {
        s_softkey = heap_caps_malloc((size_t)SK_W * SK_TOTAL * 2, MALLOC_CAP_SPIRAM);
        if (s_softkey) memset(s_softkey, 0, (size_t)SK_W * SK_TOTAL * 2);
    }
    lcd_keypad_draw();
    lcd_softkey_draw();
}

int lcd_keypad_hit(int x, int y) {
    /* soft-key overlay (in the game area, bottom-right corner) */
    if (x >= SK_X0 && x < LCD_W && y >= SK_Y0 && y < GAME_H) {
        int bsel = (y - SK_Y0) / SK_H; if (bsel < 0) bsel = 0; if (bsel > 1) bsel = 1;
        return 12 + bsel;
    }
    if (y < GAME_H) return -1;
    int ly = y - GAME_H;
    if (ly >= KEYPAD_H || x < 0 || x >= LCD_W) return -1;
    int c = x * 3 / LCD_W; if (c < 0 || c > 2) return -1;
    int r = (ly < KP_BOTTOM_Y) ? (ly / KP_DIGIT_ROW_H) : 3;
    return r * 3 + c;
}
int lcd_keypad_key(int idx) {
    if (idx == 12) return -6;   /* CALL -> LEFT soft key  (OK / select) */
    if (idx == 13) return -7;   /* END  -> RIGHT soft key (Back)        */
    if (idx < 0 || idx >= 12) return -1;
    return (unsigned char)KP_KEYS[idx];   /* digits, '*' (42), '#' (35) as real key codes */
}
void lcd_keypad_set_active(int idx) {
    if (idx == s_kp_active) return;
    s_kp_active = idx;
    lcd_keypad_draw();
    lcd_softkey_draw();
}

/* Push a full big-endian RGB565 framebuffer to the panel, strip by strip via the
 * internal-SRAM bounce buffer (GP-SPI can't DMA straight from PSRAM). No byte
 * swap: flint.drawing already stores RGB565 big-endian, which is what the panel
 * wants. The panel ignores CASET/RASET, so we always push the whole frame. */
void lcd_present(const uint8_t *fb, uint32_t len)
{
    if (!s_inited || fb == NULL) return;
    /* fb only holds the game region (LCD_W x GAME_H); the keypad strip comes from s_keypad. */
    const size_t game_need = (size_t)LCD_W * GAME_H * sizeof(uint16_t);
    if (len < game_need) return;

    for (int y = 0; y < LCD_H; y += BOUNCE_LINES) {
        int y_end = y + BOUNCE_LINES;
        if (y_end > LCD_H) y_end = LCD_H;
        const size_t pixels = (size_t)LCD_W * (y_end - y);

        if (y_end <= GAME_H) {                       /* pure game strip */
            memcpy(s_bounce, fb + (size_t)y * LCD_W * 2, pixels * 2);
            /* overlay the half-width soft-key stack onto the game's bottom-right corner */
            if (s_softkey && y < GAME_H && y_end > SK_Y0) {
                int o0 = (y > SK_Y0) ? y : SK_Y0;
                for (int row = o0; row < y_end; row++) {
                    uint8_t *dst = (uint8_t *)s_bounce + (size_t)(row - y) * LCD_W * 2 + (size_t)SK_X0 * 2;
                    const uint8_t *src = s_softkey + (size_t)(row - SK_Y0) * SK_W * 2;
                    memcpy(dst, src, (size_t)SK_W * 2);
                }
            }
        } else if (y >= GAME_H) {                    /* pure keypad strip */
            if (s_keypad) memcpy(s_bounce, s_keypad + (size_t)(y - GAME_H) * LCD_W * 2, pixels * 2);
            else          memset(s_bounce, 0, pixels * 2);
        } else {                                     /* straddles the game/keypad boundary */
            for (int row = y; row < y_end; row++) {
                uint8_t *dst = (uint8_t *)s_bounce + (size_t)(row - y) * LCD_W * 2;
                if (row < GAME_H)  memcpy(dst, fb + (size_t)row * LCD_W * 2, LCD_W * 2);
                else if (s_keypad) memcpy(dst, s_keypad + (size_t)(row - GAME_H) * LCD_W * 2, LCD_W * 2);
                else               memset(dst, 0, LCD_W * 2);
            }
        }

        esp_lcd_panel_draw_bitmap(s_panel, 0, y, LCD_W, y_end, s_bounce);
        /* Timeout instead of portMAX_DELAY: if a DMA-done IRQ is ever missed we log and
         * continue rather than hang the calling thread forever. */
        if (xSemaphoreTake(s_flush_done, pdMS_TO_TICKS(300)) != pdTRUE)
            esp_rom_printf("[lcd present timeout y=%d]\n", y);
    }
}
