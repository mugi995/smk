// Effect engine for the 15 COM x 18 SEG (6 rows x RGB) LED matrix.
// COM = key column, SEG = 3 * row + {0=R, 1=G, 2=B}, and the 18 PWM channels
// are shared by every column: the scan interrupt loads the column that is about
// to be selected. rgb_render() only fills rgb_frame[]; rgb_scan_isr() remains
// the only writer of the PWM duty registers and is not part of the engine.

#include "rgb.h"

#include "pwm.h"
#include "settings.h"
#include "tick.h"
#include <stdint.h>

#define RGB_COLS 15
#define RGB_ROWS 6

// Two frames: rgb_render() fills the inactive one and then swaps, so the PWM
// scan interrupt never reads a half-written frame while the effect animates.
// [buffer][column (COM)][row][colour], 8-bit per channel. One column is 18
// contiguous bytes, which is exactly the SEG order the PWM channels expect.
static __xdata uint8_t rgb_frame[2][RGB_COLS][RGB_ROWS][3];
static uint8_t         rgb_active;

// Effect state: rgb_phase is the animation step, rgb_tick counts scans since the
// last frame. Both are 8-bit so the hue arithmetic wraps on its own.
static uint8_t rgb_phase;
static uint8_t rgb_tick;

// 0..255 ramp that rises and falls over one 256-step phase, so a steadily
// advancing phase becomes a breathing level.
static uint8_t triangle(uint8_t x)
{
    return (x < 128) ? (uint8_t)(x << 1) : (uint8_t)((255u - x) << 1);
}

// Standard 6-sector HSV -> RGB, 8 bits per channel. The sector comes from
// h * 6 / 256 instead of a division: 90 cells are converted per frame and this
// 8051 has no divide instruction. >> 8 for / 255 differs by at most 1 LSB.
static void hsv_to_rgb(uint8_t h, uint8_t s, uint8_t v, __xdata uint8_t *out)
{
    if (s == 0) {
        out[0] = v;
        out[1] = v;
        out[2] = v;
        return;
    }

    const uint16_t hx     = (uint16_t)((uint16_t)h * 6u); // 0..1530
    const uint8_t  sector = (uint8_t)(hx >> 8);           // 0..5
    const uint8_t  frac   = (uint8_t)(hx & 0xFFu);        // position inside the sector

    const uint8_t up   = (uint8_t)(((uint16_t)s * frac) >> 8);                   // saturation fading in
    const uint8_t down = (uint8_t)(((uint16_t)s * (uint8_t)(255u - frac)) >> 8); // saturation fading out
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

// Fills the whole matrix from user_settings.rgb_mode, whose value is a VialRGB
// effect id. Effects this engine does not implement fall back to SOLID.
static void rgb_render(void)
{
    const uint8_t mode  = user_settings.rgb_mode;
    const uint8_t base  = user_settings.rgb_hue;
    const uint8_t sat   = user_settings.rgb_sat;
    const uint8_t val   = user_settings.rgb_val;
    const uint8_t phase = rgb_phase;

    const uint8_t    next = (uint8_t)(rgb_active ^ 1);
    __xdata uint8_t *cell = &rgb_frame[next][0][0][0];

    for (uint8_t col = 0; col < RGB_COLS; col++) {
        for (uint8_t row = 0; row < RGB_ROWS; row++) {
            uint8_t hue = base;
            uint8_t v   = val;

            switch (mode) {
                case 0: // OFF
                    v = 0;
                    break;
                case 4: // GRADIENT_UP_DOWN
                    hue = (uint8_t)(base + row * 16);
                    break;
                case 5: // GRADIENT_LEFT_RIGHT
                    hue = (uint8_t)(base + col * 8);
                    break;
                case 6: // BREATHING
                    v = (uint8_t)(((uint16_t)val * triangle(phase)) >> 8);
                    break;
                case 13: // CYCLE_ALL
                    hue = (uint8_t)(base + phase);
                    break;
                case 14: // CYCLE_LEFT_RIGHT
                    hue = (uint8_t)(base + col * 8 + phase);
                    break;
                case 15: // CYCLE_UP_DOWN
                    hue = (uint8_t)(base + row * 16 + phase);
                    break;
                case 16: // RAINBOW_MOVING_CHEVRON
                    hue = (uint8_t)(base + (col > 7 ? col - 7 : 7 - col) * 12 + phase);
                    break;
                case 26: // HUE_BREATHING
                    hue = (uint8_t)(base + triangle(phase));
                    break;
                case 27: // HUE_PENDULUM
                    hue = (uint8_t)(base + triangle((uint8_t)(phase << 1)));
                    break;
                case 28: // HUE_WAVE
                    hue = (uint8_t)(base + col * 6 + row * 10 + phase);
                    break;
                default: // SOLID (2) and anything unimplemented
                    break;
            }

            hsv_to_rgb(hue, sat, v, cell);
            cell += 3;
        }
    }

    rgb_active = next;
}

void rgb_init(void)
{
    // settings_load() runs before kb_init() (main.c). If it found no valid
    // record (a blank NVM or one from before the RGB fields existed), start
    // with a visible default; otherwise keep exactly what was stored.
    if (!settings_loaded) {
        user_settings.rgb_mode  = 2; // SOLID (diagnostic: no animation flicker)
        user_settings.rgb_speed = 0;
        user_settings.rgb_hue   = 0;
        user_settings.rgb_sat   = 255;
        user_settings.rgb_val   = 128;
    }

    rgb_render();

    // DUTY1 = 0 and DUTY2 = 0 in every channel: the first scan interrupt fills
    // the column it selects, so nothing lights before that.
    SET_PWM_DUTY_1(PWM20, 0);
    SET_PWM_DUTY_2(PWM20, 0);
    SET_PWM_DUTY_1(PWM21, 0);
    SET_PWM_DUTY_2(PWM21, 0);
    SET_PWM_DUTY_1(PWM22, 0);
    SET_PWM_DUTY_2(PWM22, 0);
    SET_PWM_DUTY_1(PWM23, 0);
    SET_PWM_DUTY_2(PWM23, 0);
    SET_PWM_DUTY_1(PWM24, 0);
    SET_PWM_DUTY_2(PWM24, 0);
    SET_PWM_DUTY_1(PWM25, 0);
    SET_PWM_DUTY_2(PWM25, 0);
    SET_PWM_DUTY_1(PWM10, 0);
    SET_PWM_DUTY_2(PWM10, 0);
    SET_PWM_DUTY_1(PWM11, 0);
    SET_PWM_DUTY_2(PWM11, 0);
    SET_PWM_DUTY_1(PWM12, 0);
    SET_PWM_DUTY_2(PWM12, 0);
    SET_PWM_DUTY_1(PWM13, 0);
    SET_PWM_DUTY_2(PWM13, 0);
    SET_PWM_DUTY_1(PWM14, 0);
    SET_PWM_DUTY_2(PWM14, 0);
    SET_PWM_DUTY_1(PWM15, 0);
    SET_PWM_DUTY_2(PWM15, 0);
    SET_PWM_DUTY_1(PWM03, 0);
    SET_PWM_DUTY_2(PWM03, 0);
    SET_PWM_DUTY_1(PWM04, 0);
    SET_PWM_DUTY_2(PWM04, 0);
    SET_PWM_DUTY_1(PWM05, 0);
    SET_PWM_DUTY_2(PWM05, 0);
    SET_PWM_DUTY_1(PWM00, 0);
    SET_PWM_DUTY_2(PWM00, 0);
    SET_PWM_DUTY_1(PWM01, 0);
    SET_PWM_DUTY_2(PWM01, 0);
    SET_PWM_DUTY_1(PWM02, 0);
    SET_PWM_DUTY_2(PWM02, 0);
}

// Called from kb_update(), i.e. once per main-loop pass. The key scan runs in the
// timer interrupt, and the main loop is far faster than one scan, so the frame
// divider counts scans through tick_scans() instead of counting calls.
void rgb_task(void)
{
    static uint16_t scans_seen;

    const uint16_t scans = tick_scans();
    if (scans == scans_seen) {
        return;
    }
    scans_seen = scans;

    if (++rgb_tick < 16) {
        return;
    }
    rgb_tick  = 0;
    rgb_phase = (uint8_t)(rgb_phase + 1 + (user_settings.rgb_speed >> 5));
    rgb_render();
}

// col is 0..14, the column led_scan_isr() is about to select. Called with every
// COM deselected, so the new duty values cannot be shown by the previous column.
void rgb_scan_isr(uint8_t col)
{
    const __xdata uint8_t *f = &rgb_frame[rgb_active][col][0][0];

    // Left-shift by 2 keeps the stock scaling (duty = 4 * colour, 0..1020).
    SET_PWM_DUTY_2(PWM20, (uint16_t)f[0] << 2);
    SET_PWM_DUTY_2(PWM21, (uint16_t)f[1] << 2);
    SET_PWM_DUTY_2(PWM22, (uint16_t)f[2] << 2);
    SET_PWM_DUTY_2(PWM23, (uint16_t)f[3] << 2);
    SET_PWM_DUTY_2(PWM24, (uint16_t)f[4] << 2);
    SET_PWM_DUTY_2(PWM25, (uint16_t)f[5] << 2);
    SET_PWM_DUTY_2(PWM10, (uint16_t)f[6] << 2);
    SET_PWM_DUTY_2(PWM11, (uint16_t)f[7] << 2);
    SET_PWM_DUTY_2(PWM12, (uint16_t)f[8] << 2);
    SET_PWM_DUTY_2(PWM13, (uint16_t)f[9] << 2);
    SET_PWM_DUTY_2(PWM14, (uint16_t)f[10] << 2);
    SET_PWM_DUTY_2(PWM15, (uint16_t)f[11] << 2);
    SET_PWM_DUTY_2(PWM03, (uint16_t)f[12] << 2);
    SET_PWM_DUTY_2(PWM04, (uint16_t)f[13] << 2);
    SET_PWM_DUTY_2(PWM05, (uint16_t)f[14] << 2);
    SET_PWM_DUTY_2(PWM00, (uint16_t)f[15] << 2);
    SET_PWM_DUTY_2(PWM01, (uint16_t)f[16] << 2);
    SET_PWM_DUTY_2(PWM02, (uint16_t)f[17] << 2);
}
