#pragma once

#include <stdint.h>

// Vial's commands over the same raw HID collection as VIA (0xFE prefix).
// Called from via.c with the 32-byte report in place; the answer is written
// back into the same buffer.
void vial_process(__xdata uint8_t *data);
