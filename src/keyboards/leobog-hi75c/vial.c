#include "usb.h"

#ifdef VIAL_ENABLE

static const char vial_def[] = "{\"matrix\":{\"rows\":2,\"cols\":2},\"layouts\":{\"keymap\":[[\"0,0\",\"0,1\"],[\"1,0\",\"1,1\"]]}}";

static uint8_t       raw_hid_response[32];
static volatile bool raw_hid_pending;

static void zero_report(uint8_t *data)
{
    for (uint8_t i = 0; i < 32; i++) {
        data[i] = 0;
    }
}

void raw_hid_task(void)
{
    if (!raw_hid_pending) {
        return;
    }
    raw_hid_pending = false;
    raw_hid_send(raw_hid_response, 32);
}

void raw_hid_receive(uint8_t *data, uint8_t length)
{
    (void)length;

    uint16_t def_size = (uint16_t)(sizeof(vial_def) - 1);

    switch (data[0]) {
        case 0x01:
            zero_report(data);
            data[0] = 0x01;
            data[1] = 0x00;
            data[2] = 0x09;
            break;

        case 0xFE:
            switch (data[1]) {
                case 0x00:
                    zero_report(data);
                    data[0]  = 0x06;
                    data[4]  = 0x48;
                    data[5]  = 0x69;
                    data[6]  = 0x37;
                    data[7]  = 0x35;
                    data[8]  = 0x43;
                    data[9]  = 0x00;
                    data[10] = 0x01;
                    data[11] = 0x01;
                    break;
                case 0x01:
                    zero_report(data);
                    data[0] = (uint8_t)(def_size & 0xFF);
                    data[1] = (uint8_t)(def_size >> 8);
                    break;
                case 0x02: {
                    uint16_t page   = (uint16_t)(data[2] | ((uint16_t)data[3] << 8));
                    uint16_t offset = (uint16_t)(page * 32);
                    for (uint8_t i = 0; i < 32; i++) {
                        uint16_t idx = (uint16_t)(offset + i);
                        data[i]      = (idx < def_size) ? (uint8_t)vial_def[idx] : 0;
                    }
                    break;
                }
                case 0x05:
                    zero_report(data);
                    data[0] = 1;
                    break;
                default:
                    zero_report(data);
                    break;
            }
            break;

        default:
            zero_report(data);
            break;
    }

    for (uint8_t i = 0; i < 32; i++) {
        raw_hid_response[i] = data[i];
    }
    raw_hid_pending = true;
}

#endif // VIAL_ENABLE
