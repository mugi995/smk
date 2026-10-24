#pragma once

#include <stdint.h>

void    matrix_init();
uint8_t matrix_task();

void matrix_scan_full();

uint8_t matrix_active_layer(void);
void    matrix_tap_keycode(uint16_t qcode);

#ifdef VIA_ENABLE
// The debounced state of a row: bit c set while the key in column c is down.
uint16_t matrix_get_row(uint8_t row);
#endif
