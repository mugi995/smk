#include "kbdef.h"
#include "delay.h"
#include "gpio.h"
#include "host.h"
#include "keycodes.h"
#include "pwm.h"
#include "report.h"
#include "usb.h"
#include "user_matrix.h"
#include <stdbool.h>

static uint8_t  enc_prev;
static int8_t   enc_acc;
static uint16_t led_brightness = 512;

static void led_apply_brightness(void)
{
    SET_PWM_DUTY(PWM20, 0, led_brightness);
    SET_PWM_DUTY(PWM21, 0, led_brightness);
    SET_PWM_DUTY(PWM22, 0, led_brightness);
    SET_PWM_DUTY(PWM23, 0, led_brightness);
    SET_PWM_DUTY(PWM24, 0, led_brightness);
    SET_PWM_DUTY(PWM25, 0, led_brightness);
    SET_PWM_DUTY(PWM10, 0, led_brightness);
    SET_PWM_DUTY(PWM11, 0, led_brightness);
    SET_PWM_DUTY(PWM12, 0, led_brightness);
    SET_PWM_DUTY(PWM13, 0, led_brightness);
    SET_PWM_DUTY(PWM14, 0, led_brightness);
    SET_PWM_DUTY(PWM15, 0, led_brightness);
    SET_PWM_DUTY(PWM03, 0, led_brightness);
    SET_PWM_DUTY(PWM04, 0, led_brightness);
    SET_PWM_DUTY(PWM05, 0, led_brightness);
    SET_PWM_DUTY(PWM00, 0, led_brightness);
    SET_PWM_DUTY(PWM01, 0, led_brightness);
    SET_PWM_DUTY(PWM02, 0, led_brightness);
}

static void led_pwm_init(void)
{
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

    led_apply_brightness();
}

void kb_init(void)
{
    enc_prev = (uint8_t)((P0 >> 5) & 0x03);
    enc_acc  = 0;
    led_pwm_init();
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

    for (uint8_t col = 0; col < 15; col++) {
        user_matrix_col_select(col);
        delay_us(100);
        user_matrix_col_deselect(col);
    }
}

bool kb_process_record(uint16_t keycode, bool key_pressed)
{
    switch (keycode) {
        case KB_BRI_UP:
            if (key_pressed && led_brightness <= 1020 - 128) {
                led_brightness += 128;
                led_apply_brightness();
            }
            return false;
        case KB_BRI_DN:
            if (key_pressed && led_brightness >= 128) {
                led_brightness -= 128;
                led_apply_brightness();
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
