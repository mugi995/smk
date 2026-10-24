// Uniform-colour RGB: every key gets the same colour. The LED scan only
// switches the COM line (as in v0.5.0); the 18 PWM duties are written by
// rgb_apply() only when Vial/VIA changes the colour or brightness. There is no
// per-key data, no animation, and no extra work in the scan interrupt.

#include "rgb.h"

#include "pwm.h"
#include "settings.h"
#include <stdint.h>

// The stock DUTY formula overflows PERD if the value is 255 (0xB4 + 17 + 1020
// > 1200), so cap the brightness below that.
#define RGB_MAX_VAL 240

// Stock duty offset: DUTY1 = 0xB4 + SEG, DUTY2 = DUTY1 + 4 * colour.
static const uint16_t __code rgb_duty_base[18] = {
    0xB4 + 0, 0xB4 + 1, 0xB4 + 2, 0xB4 + 3, 0xB4 + 4, 0xB4 + 5, 0xB4 + 6, 0xB4 + 7, 0xB4 + 8, 0xB4 + 9, 0xB4 + 10, 0xB4 + 11, 0xB4 + 12, 0xB4 + 13, 0xB4 + 14, 0xB4 + 15, 0xB4 + 16, 0xB4 + 17,
};

// Standard 6-sector HSV -> RGB, 8 bits per channel. s == 0 is grey/white.
static void hsv_to_rgb(uint8_t h, uint8_t s, uint8_t v, uint8_t out[3])
{
    if (s == 0) {
        out[0] = v;
        out[1] = v;
        out[2] = v;
        return;
    }

    const uint16_t hx     = (uint16_t)((uint16_t)h * 6u);
    const uint8_t  sector = (uint8_t)(hx >> 8);
    const uint8_t  frac   = (uint8_t)(hx & 0xFFu);

    const uint8_t up   = (uint8_t)(((uint16_t)s * frac) >> 8);
    const uint8_t down = (uint8_t)(((uint16_t)s * (uint8_t)(255u - frac)) >> 8);
    const uint8_t p    = (uint8_t)(((uint16_t)v * (uint8_t)(255u - s)) >> 8);
    const uint8_t q    = (uint8_t)(((uint16_t)v * (uint8_t)(255u - up)) >> 8);
    const uint8_t t    = (uint8_t)(((uint16_t)v * (uint8_t)(255u - down)) >> 8);

    switch (sector) {
        case 0:
            out[0] = v;
            out[1] = t;
            out[2] = p;
            break;
        case 1:
            out[0] = q;
            out[1] = v;
            out[2] = p;
            break;
        case 2:
            out[0] = p;
            out[1] = v;
            out[2] = t;
            break;
        case 3:
            out[0] = p;
            out[1] = q;
            out[2] = v;
            break;
        case 4:
            out[0] = t;
            out[1] = p;
            out[2] = v;
            break;
        default:
            out[0] = v;
            out[1] = p;
            out[2] = q;
            break;
    }
}

void rgb_apply(void)
{
    uint8_t c[3];

    if (user_settings.rgb_mode == 0) {
        c[0] = c[1] = c[2] = 0;
    } else {
        hsv_to_rgb(user_settings.rgb_hue, user_settings.rgb_sat, user_settings.rgb_val, c);
    }

    const uint16_t r = (uint16_t)c[0] << 2;
    const uint16_t g = (uint16_t)c[1] << 2;
    const uint16_t b = (uint16_t)c[2] << 2;

    // One RGB LED per key: R/G/B repeat for each of the six rows.
    SET_PWM_DUTY_2(PWM20, rgb_duty_base[0] + r);
    SET_PWM_DUTY_2(PWM21, rgb_duty_base[1] + g);
    SET_PWM_DUTY_2(PWM22, rgb_duty_base[2] + b);
    SET_PWM_DUTY_2(PWM23, rgb_duty_base[3] + r);
    SET_PWM_DUTY_2(PWM24, rgb_duty_base[4] + g);
    SET_PWM_DUTY_2(PWM25, rgb_duty_base[5] + b);
    SET_PWM_DUTY_2(PWM10, rgb_duty_base[6] + r);
    SET_PWM_DUTY_2(PWM11, rgb_duty_base[7] + g);
    SET_PWM_DUTY_2(PWM12, rgb_duty_base[8] + b);
    SET_PWM_DUTY_2(PWM13, rgb_duty_base[9] + r);
    SET_PWM_DUTY_2(PWM14, rgb_duty_base[10] + g);
    SET_PWM_DUTY_2(PWM15, rgb_duty_base[11] + b);
    SET_PWM_DUTY_2(PWM03, rgb_duty_base[12] + r);
    SET_PWM_DUTY_2(PWM04, rgb_duty_base[13] + g);
    SET_PWM_DUTY_2(PWM05, rgb_duty_base[14] + b);
    SET_PWM_DUTY_2(PWM00, rgb_duty_base[15] + r);
    SET_PWM_DUTY_2(PWM01, rgb_duty_base[16] + g);
    SET_PWM_DUTY_2(PWM02, rgb_duty_base[17] + b);
}

void rgb_init(void)
{
    // settings_load() runs before kb_init() (main.c). If it found no valid
    // record, start with the same white as v0.5.0.
    if (!settings_loaded) {
        user_settings.rgb_mode = 2; // solid
        user_settings.rgb_hue  = 0;
        user_settings.rgb_sat  = 0; // white
        user_settings.rgb_val  = 128;
    }

    if (user_settings.rgb_mode != 0) {
        user_settings.rgb_mode = 2;
    }
    if (user_settings.rgb_val > RGB_MAX_VAL) {
        user_settings.rgb_val = RGB_MAX_VAL;
    }

    SET_PWM_DUTY_1(PWM20, rgb_duty_base[0]);
    SET_PWM_DUTY_1(PWM21, rgb_duty_base[1]);
    SET_PWM_DUTY_1(PWM22, rgb_duty_base[2]);
    SET_PWM_DUTY_1(PWM23, rgb_duty_base[3]);
    SET_PWM_DUTY_1(PWM24, rgb_duty_base[4]);
    SET_PWM_DUTY_1(PWM25, rgb_duty_base[5]);
    SET_PWM_DUTY_1(PWM10, rgb_duty_base[6]);
    SET_PWM_DUTY_1(PWM11, rgb_duty_base[7]);
    SET_PWM_DUTY_1(PWM12, rgb_duty_base[8]);
    SET_PWM_DUTY_1(PWM13, rgb_duty_base[9]);
    SET_PWM_DUTY_1(PWM14, rgb_duty_base[10]);
    SET_PWM_DUTY_1(PWM15, rgb_duty_base[11]);
    SET_PWM_DUTY_1(PWM03, rgb_duty_base[12]);
    SET_PWM_DUTY_1(PWM04, rgb_duty_base[13]);
    SET_PWM_DUTY_1(PWM05, rgb_duty_base[14]);
    SET_PWM_DUTY_1(PWM00, rgb_duty_base[15]);
    SET_PWM_DUTY_1(PWM01, rgb_duty_base[16]);
    SET_PWM_DUTY_1(PWM02, rgb_duty_base[17]);

    rgb_apply();
}

void rgb_save(void)
{
    settings_mark_dirty();
}

void rgb_brightness_step(int8_t delta)
{
    int16_t value = (int16_t)user_settings.rgb_val + delta;

    if (value < 0) {
        value = 0;
    }
    if (value > RGB_MAX_VAL) {
        value = RGB_MAX_VAL;
    }

    user_settings.rgb_val = (uint8_t)value;
    rgb_apply();
    settings_mark_dirty();
}

// VialRGB (0x08/0x07, data[1] = 0x40..0x44).
void rgb_vial_get(__xdata uint8_t *data)
{
    switch (data[1]) {
        case 0x40: // info: protocol version 1, maximum brightness
            data[2] = 1;
            data[3] = 0;
            data[4] = RGB_MAX_VAL;
            break;
        case 0x41: // mode: effect, speed, hue, sat, val
            data[2] = user_settings.rgb_mode;
            data[3] = 0;
            data[4] = 0; // speed is unused
            data[5] = user_settings.rgb_hue;
            data[6] = user_settings.rgb_sat;
            data[7] = user_settings.rgb_val;
            break;
        case 0x42: { // supported effect IDs greater than the request
            const uint16_t gt  = (uint16_t)(data[2] | ((uint16_t)data[3] << 8));
            uint8_t       *out = &data[2];
            if (2 > gt) {
                out[0] = 2; // solid colour
                out[1] = 0;
                out += 2;
            }
            out[0] = 0xFF;
            out[1] = 0xFF;
            break;
        }
        default:
            break;
    }
}

void rgb_vial_set(__xdata uint8_t *data)
{
    if (data[1] != 0x41) {
        return;
    }

    const uint16_t mode = (uint16_t)(data[2] | ((uint16_t)data[3] << 8));

    user_settings.rgb_mode = (mode == 0) ? 0 : 2;
    user_settings.rgb_hue  = data[5];
    user_settings.rgb_sat  = data[6];
    user_settings.rgb_val  = (data[7] > RGB_MAX_VAL) ? RGB_MAX_VAL : data[7];

    rgb_apply();
}

// VIA qmk_rgblight (0x08/0x07, data[1] = 2, data[2] = value id).
void rgb_via_get(__xdata uint8_t *data)
{
    switch (data[2]) {
        case 1: // brightness, scaled to 0..255 for VIA
            data[3] = (uint8_t)(((uint16_t)user_settings.rgb_val * 255u) / RGB_MAX_VAL);
            break;
        case 2: // effect: 0 = off, 1 = solid
            data[3] = (user_settings.rgb_mode == 0) ? 0 : 1;
            break;
        case 3: // speed: unused
            data[3] = 0;
            break;
        case 4: // colour: hue, sat
            data[3] = user_settings.rgb_hue;
            data[4] = user_settings.rgb_sat;
            break;
        default:
            break;
    }
}

void rgb_via_set(__xdata uint8_t *data)
{
    switch (data[2]) {
        case 1:
            user_settings.rgb_val = (uint8_t)(((uint16_t)data[3] * RGB_MAX_VAL) / 255u);
            rgb_apply();
            break;
        case 2:
            user_settings.rgb_mode = (data[3] == 0) ? 0 : 2;
            rgb_apply();
            break;
        case 3:
            break;
        case 4:
            user_settings.rgb_hue = data[3];
            user_settings.rgb_sat = data[4];
            rgb_apply();
            break;
        default:
            break;
    }
}
