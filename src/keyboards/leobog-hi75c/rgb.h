#pragma once

#include <stdint.h>

void rgb_init(void);
void rgb_task(void);
void rgb_scan_isr(uint8_t col);
void rgb_mark_dirty(void);
void rgb_scan_pause(void);
void rgb_scan_resume(void);
