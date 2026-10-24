// RGB effect engine for the 15 COM x 18 SEG (6 rows x RGB) LED matrix.
// COM = key column, SEG = 3 * row + {0=R, 1=G, 2=B}, and the 18 PWM channels
// are shared by every column: the scan interrupt loads the column that is about
// to be selected.
//
// Two rules keep this from disturbing the keyboard:
//  - rgb_task() renders ONE column (6 keys) per key scan. A 90-key conversion
//    in one main-loop pass delayed the key processing and the USB stack.
//  - matrix_scan_full() drives the same column lines for the key scan. While it
//    runs, rgb_scan_pause() disables the LED interrupt and zeroes the duties;
//    otherwise the LEDs of the scanned column show the previous column's colour.
//
// Frames are double-buffered: rgb_render_col() fills the inactive buffer and
// rgb_task() swaps it after the last column, so the ISR never reads a
// half-written frame.

#include "rgb.h"

#include "pwm.h"
#include "settings.h"
#include "tick.h"
#include <stdint.h>

#define RGB_COLS 15
#define RGB_ROWS 6

// [buffer][column (COM)][row][colour], 8-bit per channel. One column is 18
// contiguous bytes, exactly the SEG order the PWM channels expect.
static __xdata uint8_t rgb_frame[2][RGB_COLS][RGB_ROWS][3];
static uint8_t         rgb_active;
static uint8_t         rgb_next;

// Animation state. rgb_dirty forces a redraw for static effects.
static uint8_t rgb_phase;
static uint8_t rgb_col;
static bool    rgb_dirty = true;

// 0..255 ramp that rises and falls over one 256-step phase.
static uint8_t triangle(uint8_t x)
{
    return (x < 128) ? (uint8_t)(x << 1) : (uint8_t)((255u - x) << 1);
}

// Standard 6-sector HSV -> RGB, 8 bits per channel.
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

static bool rgb_animated(uint8_t mode)
{
    switch (mode) {
        case 0: // OFF
        case 2: // SOLID
        case 4: // GRADIENT_UP_DOWN
        case 5: // GRADIENT_LEFT_RIGHT
            return false;
        default:
            return true;
    }
}

// Fills one column (6 keys) of the inactive buffer.
static void rgb_render_col(uint8_t col)
{
    const uint8_t mode  = user_settings.rgb_mode;
    const uint8_t base  = user_settings.rgb_hue;
    const uint8_t sat   = user_settings.rgb_sat;
    const uint8_t val   = user_settings.rgb_val;
    const uint8_t phase = rgb_phase;

    __xdata uint8_t *cell = &rgb_frame[rgb_next][col][0][0];

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

// Stock duty offset: DUTY1 = 0xB4 + SEG, DUTY2 = DUTY1 + 4 * colour. Keeping
// the offset staggers the 18 channels instead of switching them all at once.
static const uint16_t __code rgb_duty_base[18] = {
    0xB4 + 0, 0xB4 + 1, 0xB4 + 2, 0xB4 + 3, 0xB4 + 4, 0xB4 + 5, 0xB4 + 6, 0xB4 + 7, 0xB4 + 8, 0xB4 + 9, 0xB4 + 10, 0xB4 + 11, 0xB4 + 12, 0xB4 + 13, 0xB4 + 14, 0xB4 + 15, 0xB4 + 16, 0xB4 + 17,
};

void rgb_init(void)
{
    // settings_load() runs before kb_init() (main.c). If it found no valid
    // record, start with a visible static default; otherwise keep what was
    // stored.
    if (!settings_loaded) {
        user_settings.rgb_mode  = 2; // SOLID
        user_settings.rgb_speed = 0;
        user_settings.rgb_hue   = 0;
        user_settings.rgb_sat   = 255;
        user_settings.rgb_val   = 128;
    }

    rgb_active = 0;
    rgb_next   = 1;
    rgb_col    = 0;
    rgb_phase  = 0;
    rgb_dirty  = true;

    // DUTY1 = the stock offset; DUTY2 = the same offset (zero pulse width).
    SET_PWM_DUTY_1(PWM20, rgb_duty_base[0]);
    SET_PWM_DUTY_2(PWM20, rgb_duty_base[0]);
    SET_PWM_DUTY_1(PWM21, rgb_duty_base[1]);
    SET_PWM_DUTY_2(PWM21, rgb_duty_base[1]);
    SET_PWM_DUTY_1(PWM22, rgb_duty_base[2]);
    SET_PWM_DUTY_2(PWM22, rgb_duty_base[2]);
    SET_PWM_DUTY_1(PWM23, rgb_duty_base[3]);
    SET_PWM_DUTY_2(PWM23, rgb_duty_base[3]);
    SET_PWM_DUTY_1(PWM24, rgb_duty_base[4]);
    SET_PWM_DUTY_2(PWM24, rgb_duty_base[4]);
    SET_PWM_DUTY_1(PWM25, rgb_duty_base[5]);
    SET_PWM_DUTY_2(PWM25, rgb_duty_base[5]);
    SET_PWM_DUTY_1(PWM10, rgb_duty_base[6]);
    SET_PWM_DUTY_2(PWM10, rgb_duty_base[6]);
    SET_PWM_DUTY_1(PWM11, rgb_duty_base[7]);
    SET_PWM_DUTY_2(PWM11, rgb_duty_base[7]);
    SET_PWM_DUTY_1(PWM12, rgb_duty_base[8]);
    SET_PWM_DUTY_2(PWM12, rgb_duty_base[8]);
    SET_PWM_DUTY_1(PWM13, rgb_duty_base[9]);
    SET_PWM_DUTY_2(PWM13, rgb_duty_base[9]);
    SET_PWM_DUTY_1(PWM14, rgb_duty_base[10]);
    SET_PWM_DUTY_2(PWM14, rgb_duty_base[10]);
    SET_PWM_DUTY_1(PWM15, rgb_duty_base[11]);
    SET_PWM_DUTY_2(PWM15, rgb_duty_base[11]);
    SET_PWM_DUTY_1(PWM03, rgb_duty_base[12]);
    SET_PWM_DUTY_2(PWM03, rgb_duty_base[12]);
    SET_PWM_DUTY_1(PWM04, rgb_duty_base[13]);
    SET_PWM_DUTY_2(PWM04, rgb_duty_base[13]);
    SET_PWM_DUTY_1(PWM05, rgb_duty_base[14]);
    SET_PWM_DUTY_2(PWM05, rgb_duty_base[14]);
    SET_PWM_DUTY_1(PWM00, rgb_duty_base[15]);
    SET_PWM_DUTY_2(PWM00, rgb_duty_base[15]);
    SET_PWM_DUTY_1(PWM01, rgb_duty_base[16]);
    SET_PWM_DUTY_2(PWM01, rgb_duty_base[16]);
    SET_PWM_DUTY_1(PWM02, rgb_duty_base[17]);
    SET_PWM_DUTY_2(PWM02, rgb_duty_base[17]);
}

// Called by the protocol handlers when the stored effect changes.
void rgb_mark_dirty(void)
{
    rgb_dirty = true;
}

// Called around matrix_scan_full(): the key scan drives the same COM lines, so
// the LED interrupt stops and the duties go to zero; without this, each scanned
// column shows the previous column's colour for the whole key scan.
void rgb_scan_pause(void)
{
    PWM00CON &= (uint8_t)~0x40; // PWM0 period interrupt off

    // DUTY1 = DUTY2 = 0 is the only state that cannot leave a pulse running
    // while the key scan drives a column.
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

void rgb_scan_resume(void)
{
    // Restore the stock DUTY1 offsets; the next interrupt fills DUTY2.
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

    PWM00CON |= 0x40; // PWM0 period interrupt on
}

// Called once per main-loop pass. One column per key scan: no long block, and
// the 15 columns of a frame take 15 scans (about 15 ms).
void rgb_task(void)
{
    static uint16_t scans_seen;

    const uint16_t scans = tick_scans();
    if (scans == scans_seen) {
        return;
    }
    scans_seen = scans;

    if (!rgb_animated(user_settings.rgb_mode) && !rgb_dirty) {
        return;
    }

    rgb_render_col(rgb_col);
    if (++rgb_col < RGB_COLS) {
        return;
    }

    rgb_col    = 0;
    rgb_active = rgb_next;
    rgb_next   = (uint8_t)(rgb_active ^ 1);
    rgb_phase  = (uint8_t)(rgb_phase + 1 + (user_settings.rgb_speed >> 5));
    rgb_dirty  = false;
}

// col is 0..14, the column led_scan_isr() is about to select. Called with every
// COM deselected, so the new duty values cannot be shown by the previous column.
void rgb_scan_isr(uint8_t col)
{
    const __xdata uint8_t *f = &rgb_frame[rgb_active][col][0][0];

    SET_PWM_DUTY_2(PWM20, (uint16_t)(rgb_duty_base[0] + ((uint16_t)f[0] << 2)));
    SET_PWM_DUTY_2(PWM21, (uint16_t)(rgb_duty_base[1] + ((uint16_t)f[1] << 2)));
    SET_PWM_DUTY_2(PWM22, (uint16_t)(rgb_duty_base[2] + ((uint16_t)f[2] << 2)));
    SET_PWM_DUTY_2(PWM23, (uint16_t)(rgb_duty_base[3] + ((uint16_t)f[3] << 2)));
    SET_PWM_DUTY_2(PWM24, (uint16_t)(rgb_duty_base[4] + ((uint16_t)f[4] << 2)));
    SET_PWM_DUTY_2(PWM25, (uint16_t)(rgb_duty_base[5] + ((uint16_t)f[5] << 2)));
    SET_PWM_DUTY_2(PWM10, (uint16_t)(rgb_duty_base[6] + ((uint16_t)f[6] << 2)));
    SET_PWM_DUTY_2(PWM11, (uint16_t)(rgb_duty_base[7] + ((uint16_t)f[7] << 2)));
    SET_PWM_DUTY_2(PWM12, (uint16_t)(rgb_duty_base[8] + ((uint16_t)f[8] << 2)));
    SET_PWM_DUTY_2(PWM13, (uint16_t)(rgb_duty_base[9] + ((uint16_t)f[9] << 2)));
    SET_PWM_DUTY_2(PWM14, (uint16_t)(rgb_duty_base[10] + ((uint16_t)f[10] << 2)));
    SET_PWM_DUTY_2(PWM15, (uint16_t)(rgb_duty_base[11] + ((uint16_t)f[11] << 2)));
    SET_PWM_DUTY_2(PWM03, (uint16_t)(rgb_duty_base[12] + ((uint16_t)f[12] << 2)));
    SET_PWM_DUTY_2(PWM04, (uint16_t)(rgb_duty_base[13] + ((uint16_t)f[13] << 2)));
    SET_PWM_DUTY_2(PWM05, (uint16_t)(rgb_duty_base[14] + ((uint16_t)f[14] << 2)));
    SET_PWM_DUTY_2(PWM00, (uint16_t)(rgb_duty_base[15] + ((uint16_t)f[15] << 2)));
    SET_PWM_DUTY_2(PWM01, (uint16_t)(rgb_duty_base[16] + ((uint16_t)f[16] << 2)));
    SET_PWM_DUTY_2(PWM02, (uint16_t)(rgb_duty_base[17] + ((uint16_t)f[17] << 2)));
}
