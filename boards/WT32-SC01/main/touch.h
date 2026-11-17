#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool bsp_touch_init(void);
bool touch_get_point(int *x, int *y);
int  touch_poll_key(void);

#ifdef __cplusplus
}
#endif
