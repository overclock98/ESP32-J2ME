#pragma once
#include <stdint.h>
#include <stdbool.h>

/* I2C capacitive-touch bridge for the Guition JC3248W535EN (AXS15231B touch), polled in a
 * dedicated native FreeRTOS task (like the QR project) so the I2C read can never block the
 * Java threads. The Java side only reads touch_poll_key() (a cheap shared value). */

#ifdef __cplusplus
extern "C" {
#endif

bool touch_init(void);                  /* I2C + AXS15231B touch + start polling task; true on success */
bool touch_get_point(int *x, int *y);   /* raw read (used by the polling task) */
int  touch_poll_key(void);              /* ASCII key currently under the finger, or -1 */

#ifdef __cplusplus
}
#endif
