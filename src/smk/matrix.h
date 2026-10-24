#pragma once

#include <stdint.h>

void    matrix_init();
uint8_t matrix_task();

void matrix_scan_full();

// Sample the rows of one column from the LED COM interrupt. The column has been
// selected for a full PWM period by then.
void matrix_sample_column(uint8_t col, uint8_t rows);

uint8_t matrix_active_layer(void);
void    matrix_tap_keycode(uint16_t qcode);

#ifdef VIA_ENABLE
// The debounced state of a row: bit c set while the key in column c is down.
uint16_t matrix_get_row(uint8_t row);
#endif
