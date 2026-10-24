// R0 test pattern for the 15 COM x 18 SEG (6 rows x RGB) LED matrix.
// COM = key column, SEG = 3 * row + {0=R, 1=G, 2=B}, and the 18 PWM channels
// are shared by every column: the scan interrupt loads the column that is about
// to be selected. No effect engine, no host protocol.

#include "rgb.h"

#include "pwm.h"
#include <stdint.h>

// [column (COM)][row][colour], 8-bit per channel. One column is 18 contiguous
// bytes, which is exactly the SEG order the PWM channels expect.
static __xdata uint8_t rgb_frame[15][6][3];

void rgb_init(void)
{
    // Rows 0-3 are solid red/green/blue/white, rows 4-5 are off. The pattern is
    // the same in every column, so a frame written one PWM period late still
    // looks right; that shift is invisible here by design.
    for (uint8_t col = 0; col < 15; col++) {
        for (uint8_t rgb = 0; rgb < 3; rgb++) {
            rgb_frame[col][0][rgb] = (rgb == 0) ? 0x80 : 0x00;
            rgb_frame[col][1][rgb] = (rgb == 1) ? 0x80 : 0x00;
            rgb_frame[col][2][rgb] = (rgb == 2) ? 0x80 : 0x00;
            rgb_frame[col][3][rgb] = 0x80;
            rgb_frame[col][4][rgb] = 0x00;
            rgb_frame[col][5][rgb] = 0x00;
        }
    }

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

// col is 0..14, the column led_scan_isr() is about to select. Called with every
// COM deselected, so the new duty values cannot be shown by the previous column.
void rgb_scan_isr(uint8_t col)
{
    const __xdata uint8_t *f = &rgb_frame[col][0][0];

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
