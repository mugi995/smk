#include "kbdef.h"
#include "gpio.h"
#include "user_matrix.h"

// Column COM pins: C0-C7 = P6.0-P6.7, C8-C11 = P5.0/P5.1/P5.2/P5.7,
// C12-C14 = P4.0/P4.2/P4.3. The tables replace the 15-case switches in the PWM
// and key-scan paths.
static const uint8_t __code col_port[15] = {6, 6, 6, 6, 6, 6, 6, 6, 5, 5, 5, 5, 4, 4, 4};
static const uint8_t __code col_mask[15] = {
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x80, 0x01, 0x04, 0x08,
};

void user_matrix_cols_deselect_all(void)
{
    GPIO_HIGH(6, KB_C_P6_MASK);
    GPIO_HIGH(5, KB_C_P5_MASK);
    GPIO_HIGH(4, KB_C_P4_MASK);
}

void user_matrix_col_select(uint8_t col)
{
    const uint8_t mask = col_mask[col];

    switch (col_port[col]) {
        case 4:
            P4 &= (uint8_t)~mask;
            break;
        case 5:
            P5 &= (uint8_t)~mask;
            break;
        default:
            P6 &= (uint8_t)~mask;
            break;
    }
}

void user_matrix_col_deselect(uint8_t col)
{
    const uint8_t mask = col_mask[col];

    switch (col_port[col]) {
        case 4:
            P4 |= mask;
            break;
        case 5:
            P5 |= mask;
            break;
        default:
            P6 |= mask;
            break;
    }
}

uint8_t user_matrix_read_rows(void)
{
    // bit0-3 = P7.0-3, bit4-5 = P5.3-4, bit6-7 forced high so they read as released.
    return (uint8_t)((P7 & 0x0F) | ((P5 << 1) & 0x30) | 0xC0);
}
