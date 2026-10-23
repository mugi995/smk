#include "kbdef.h"
#include "delay.h"
#include "gpio.h"
#include "host.h"
#include "keycodes.h"
#include "report.h"
#include "usb.h"

static uint8_t enc_prev;
static int8_t  enc_acc;

void kb_init(void)
{
    enc_prev = (uint8_t)((P0 >> 5) & 0x03);
    enc_acc  = 0;
}

static void enc_step(uint16_t keycode)
{
    host_consumer_send(keycode_to_consumer((uint8_t)keycode));
    delay_ms(10);
    host_consumer_send(0);
}

void kb_update(void)
{
    static const int8_t transition[16] = {
        0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0,
    };
    uint8_t cur = (uint8_t)((P0 >> 5) & 0x03);
    enc_acc += transition[(enc_prev << 2) | cur];
    enc_prev = cur;

    if (enc_acc >= 4) {
        enc_acc = 0;
        enc_step(KC_AUDIO_VOL_UP);
    } else if (enc_acc <= -4) {
        enc_acc = 0;
        enc_step(KC_AUDIO_VOL_DOWN);
    }
}

void kb_send_report(__xdata report_keyboard_t *report)
{
    usb_send_report(report);
}

void kb_send_nkro(__xdata report_nkro_t *report)
{
    usb_send_nkro(report);
}

void kb_send_extra(__xdata report_extra_t *report)
{
    usb_send_extra(report);
}
