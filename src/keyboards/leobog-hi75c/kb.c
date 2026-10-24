#include "kbdef.h"
#include "delay.h"
#include "gpio.h"
#include "host.h"
#include "keycodes.h"
#include "matrix.h"
#include "pwm.h"
#include "report.h"
#include "rgb.h"
#include "settings.h"
#include "usb.h"
#include "user_matrix.h"
#include <stdbool.h>

static uint8_t enc_prev;
static int8_t  enc_acc;
static uint8_t led_col;

static void led_pwm_init(void)
{
#ifdef LED_GPIO_TEST
    // Diagnostic: disable PWM and drive the SEG pins as plain GPIO high.
    // The matrix scan drives one COM (key column) low at a time, so the LEDs
    // should light during the scan if P1/P2/P3 really are the SEG pins.
    PWM00CON = 0x00;
    PWM10CON = 0x00;
    PWM20CON = 0x00;
    P1       = 0xFF;
    P2       = 0xFF;
    P3       = 0xFF;
    return;
#else

    PWM0PERDH = 0x04;
    PWM0PERDL = 0xB0;
    PWM1PERDH = 0x04;
    PWM1PERDL = 0xB0;
    PWM2PERDH = 0x04;
    PWM2PERDL = 0xB0;

    PWM01CON = 0x08;
    PWM02CON = 0x08;
    PWM03CON = 0x08;
    PWM04CON = 0x08;
    PWM05CON = 0x08;
    PWM11CON = 0x08;
    PWM12CON = 0x08;
    PWM13CON = 0x08;
    PWM14CON = 0x08;
    PWM15CON = 0x08;
    PWM21CON = 0x08;
    PWM22CON = 0x08;
    PWM23CON = 0x08;
    PWM24CON = 0x08;
    PWM25CON = 0x08;

    PWM00CON = 0x89;
    PWM10CON = 0x89;
    PWM20CON = 0x89;

    PWM00CON |= 0x40; // PWM0IE
    IEN1 |= _EPWM0;
#endif
}

// Called from pwm_interrupt_handler() (the PWM0 vector in src/sino51lib/pwm.c).
void led_scan_isr(void)
{
    // Clear PWM0 period interrupt flag (PWM0IF, bit5).
    PWM00CON &= (uint8_t)~0x20;

    user_matrix_col_deselect(led_col);
    led_col++;
    if (led_col >= 15) {
        led_col = 0;
    }
    user_matrix_col_select(led_col);
}

void kb_init(void)
{
    enc_prev = (uint8_t)((P0 >> 5) & 0x03);
    enc_acc  = 0;
    led_pwm_init();
    rgb_init();
}

// dir 0 = CCW / VOL_DOWN, dir 1 = CW / VOL_UP.
static void enc_step(uint8_t dir)
{
    uint8_t layer = matrix_active_layer();
    if (layer >= 2) {
        layer = 1;
    }

    uint16_t code = user_settings.encoder[layer][dir];
    if (!code) {
        code = (dir == 0) ? KC_AUDIO_VOL_DOWN : KC_AUDIO_VOL_UP;
    }

    matrix_tap_keycode(code);
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
        enc_step(1);
    } else if (enc_acc <= -4) {
        enc_acc = 0;
        enc_step(0);
    }
}

bool kb_process_record(uint16_t keycode, bool key_pressed)
{
    switch (keycode) {
        case KB_BRI_UP:
            if (key_pressed) {
                rgb_brightness_step(16);
            }
            return false;
        case KB_BRI_DN:
            if (key_pressed) {
                rgb_brightness_step(-16);
            }
            return false;
        default:
            return true;
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
