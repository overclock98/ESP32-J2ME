#pragma once
#include <stdint.h>
#include <stdbool.h>

/* Display port for Wireless-Tag WT32-SC01 (ST7796 4-line HSPI 320x480).
 * Flint's flint.drawing rasterizer renders the framebuffer
 * itself, and stores RGB565 big-endian, so lcd_present() pushes
 * the buffer with no software byte-swap.
 *
 * The panel is split: the game renders the top GAME_H rows; the bottom KEYPAD_H
 * rows are an on-screen touch keypad drawn natively here and composited into the
 * full-frame push by lcd_present(). lcd_height() reports GAME_H so the game (and
 * the Java framebuffer) only ever own the top region. */

#define LCD_W      320
#define LCD_H      480                 /* full panel height */
#define KEYPAD_H   160                 /* on-screen keypad strip at the bottom */
#define GAME_H     (LCD_H - KEYPAD_H)  /* 320 — matches the game's native 240x320 height */
#define GAME_W     240                 /* 240 — native Nokia S40v5 width */
#define GAME_X0    ((LCD_W - GAME_W) / 2) /* 40 — horizontally centered */
#define MARGIN_W   GAME_X0             /* 40 — side margin width */

#ifdef __cplusplus
extern "C" {
#endif

void lcd_init(void);                                  /* SPI bus + panel + bounce buf + backlight; idempotent */
int  lcd_width(void);                                 /* GAME_W (240) — what the game sees */
int  lcd_height(void);                                /* GAME_H (320) — what the game sees */
void lcd_present(const uint8_t *fb, uint32_t len);    /* pushes 240x320 game to center window, keypad if dirty */

/* On-screen touch keypad (drawn natively into the bottom strip, composited by lcd_present). */
void lcd_keypad_init(void);            /* draw all buttons; safe to call after lcd_init */
int  lcd_keypad_hit(int x, int y);     /* screen coords -> button index 0..13, or -1 */
int  lcd_keypad_key(int idx);          /* button index -> ASCII key code, or -1 */
void lcd_keypad_set_active(int idx);   /* highlight button idx (-1 = none); redraws only on change */

#ifdef __cplusplus
}
#endif
