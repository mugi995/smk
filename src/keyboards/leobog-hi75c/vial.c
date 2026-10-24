// SPDX-License-Identifier: GPL-2.0-or-later
// Vial's 0xFE commands on top of VIA's raw HID: the keyboard UID, the XZ
// definition and the unlock status. The keymap itself is VIA's dynamic keymap
// (via.c/dynamic_keymap.c), so there is nothing else to answer here.
//
// The definition is generated from vial.json by
// utils/vial_generate_definition.py (minified JSON, XZ-compressed) and
// committed as vial_definition.h, so the 8051 only serves bytes.

#include "vial.h"

#ifdef VIAL_ENABLE

#    include "vial_definition.h"

#    define VIAL_PROTOCOL_VERSION 6
#    define VIAL_REPORT_SIZE      32

// Nonzero and fixed: the WebHID Vial app probes 0xFE 0x00 and treats a nonzero
// UID as "this is a Vial keyboard" (the browser does not expose the USB serial
// number). The eight bytes are "Hi75Cvia".
static const uint8_t vial_uid[8] = {0x48, 0x69, 0x37, 0x35, 0x43, 0x76, 0x69, 0x61};

static void zero_report(__xdata uint8_t *data)
{
    for (uint8_t i = 0; i < VIAL_REPORT_SIZE; i++) {
        data[i] = 0;
    }
}

void vial_process(__xdata uint8_t *data)
{
    switch (data[1]) {
        // Get keyboard ID: 32-bit Vial protocol version, then the 8-byte UID.
        case 0x00:
            zero_report(data);
            data[0] = VIAL_PROTOCOL_VERSION & 0xFF;
            data[1] = (VIAL_PROTOCOL_VERSION >> 8) & 0xFF;
            for (uint8_t i = 0; i < 8; i++) {
                data[4 + i] = vial_uid[i];
            }
            break;

        // Definition size: 32-bit little endian.
        case 0x01: {
            const uint32_t size = sizeof(keyboard_definition);
            zero_report(data);
            data[0] = (uint8_t)(size & 0xFF);
            data[1] = (uint8_t)((size >> 8) & 0xFF);
            data[2] = (uint8_t)((size >> 16) & 0xFF);
            data[3] = (uint8_t)((size >> 24) & 0xFF);
            break;
        }

        // Definition page: page number at data[2..5] little endian; 32 bytes out.
        case 0x02: {
            const uint32_t page   = (uint32_t)data[2] | ((uint32_t)data[3] << 8) | ((uint32_t)data[4] << 16) | ((uint32_t)data[5] << 24);
            const uint32_t offset = page * VIAL_REPORT_SIZE;
            for (uint8_t i = 0; i < VIAL_REPORT_SIZE; i++) {
                const uint32_t index = offset + i;
                data[i]              = (index < sizeof(keyboard_definition)) ? keyboard_definition[index] : 0;
            }
            break;
        }

        // Unlock status: 1 = insecure, so the GUI may edit without the combo.
        case 0x05:
            zero_report(data);
            data[0] = 1;
            break;

        // QMK settings query: no settings. The GUI loops until the first
        // 16-bit QSID is 0xFFFF, so an all-zero answer would spin forever.
        case 0x09:
            zero_report(data);
            data[0] = 0xFF;
            data[1] = 0xFF;
            break;

        // QMK settings get/set/reset: nothing to do without settings.
        case 0x0A:
        case 0x0B:
        case 0x0C:
            zero_report(data);
            break;

        // Dynamic entries (tap dance, combo, key override, alt repeat): none.
        case 0x0D:
            zero_report(data);
            break;

        default:
            zero_report(data);
            break;
    }
}

#endif // VIAL_ENABLE
