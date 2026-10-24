#pragma once

#include <stdint.h>

void rgb_init(void);
void rgb_apply(void);
void rgb_save(void);
void rgb_brightness_step(int8_t delta);
void rgb_vial_get(__xdata uint8_t *data);
void rgb_vial_set(__xdata uint8_t *data);
void rgb_via_get(__xdata uint8_t *data);
void rgb_via_set(__xdata uint8_t *data);
