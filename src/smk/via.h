#pragma once

// VIA over raw HID (usage page 0xFF60): the commands for the keymap, the
// switch matrix and the protocol version. 32-byte reports; usb.c takes them in
// and sends the answer.

#include <stdint.h>

#define VIA_REPORT_SIZE 32

// What QMK's via.h calls VIA_PROTOCOL_VERSION: the version VIA checks for.
// Vial's GUI only accepts 9, so a Vial build reports that; VIA itself takes
// both 9 and 12.
#ifdef VIAL_ENABLE
#    define VIA_PROTOCOL_VERSION 0x0009
#else
#    define VIA_PROTOCOL_VERSION 0x000C
#endif

// Answers the command in `data` in place (an unknown one becomes 0xFF).
void via_process(__xdata uint8_t *data);
