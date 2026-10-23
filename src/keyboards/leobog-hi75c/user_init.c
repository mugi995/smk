#include "kbdef.h"
#include "gpio.h"
#include "user_init.h"

void user_init(void)
{
    // Columns idle high; the matrix scan pulls one low at a time.
    GPIO_WRITE(6, 0xFF);
    GPIO_WRITE(5, KB_C_P5_MASK);
    GPIO_WRITE(4, KB_C_P4_MASK);

    GPIO_DIR_WRITE(6, 0xFF);         // C0..C7
    GPIO_DIR_WRITE(5, KB_C_P5_MASK); // C8..C11; R4/R5 stay inputs
    GPIO_DIR_WRITE(4, KB_C_P4_MASK); // C12..C14

    // Rows are inputs with pull-ups.
    GPIO_INPUT(7, KB_R_P7_MASK);
    GPIO_INPUT(5, KB_R_P5_MASK);
    GPIO_PULLUP_WRITE(7, KB_R_P7_MASK);
    GPIO_PULLUP_WRITE(5, KB_R_P5_MASK);

    // Stock firmware leaves P5 at the default drive strength.
    DRVCON = DRVCON_UNLOCK_P5;
    P5DRV  = 0x00;
    DRVCON = DRVCON_LOCK;
}
