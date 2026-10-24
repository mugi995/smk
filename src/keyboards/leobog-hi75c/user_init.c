#include "kbdef.h"
#include "delay.h"
#include "gpio.h"
#include "user_init.h"
#include "utils.h"

void user_init(void)
{
    // A bootloader reboot leaves the USB core in its previous state, and the
    // host can miss the detach if we re-attach immediately. Reset the core and
    // stay disconnected long enough for the host to notice before usb_init().
    USBCON = _SWRST;
    _nop_();
    _nop_();
    _nop_();
    _nop_();
    _nop_();
    _nop_();
    USBCON = 0;
    delay_ms(200);

    // All latches high: columns idle high and the input pins stay readable.
    GPIO_WRITE(6, 0xFF);
    GPIO_WRITE(5, 0xFF);
    GPIO_WRITE(4, 0xFF);
    GPIO_WRITE(7, 0xFF);

    GPIO_DIR_WRITE(6, 0xFF);         // C0..C7
    GPIO_DIR_WRITE(5, KB_C_P5_MASK); // C8..C11; R4/R5 stay inputs
    GPIO_DIR_WRITE(4, KB_C_P4_MASK); // C12..C14

    // Rows are inputs with pull-ups.
    GPIO_INPUT(7, KB_R_P7_MASK);
    GPIO_INPUT(5, KB_R_P5_MASK);
    GPIO_PULLUP_WRITE(7, KB_R_P7_MASK);
    GPIO_PULLUP_WRITE(5, KB_R_P5_MASK);

    // LED SEG pins (PWM outputs)
    GPIO_DIR_WRITE(1, 0x3F);
    GPIO_DIR_WRITE(2, 0x3F);
    GPIO_DIR_WRITE(3, 0x3F);

    // Match the stock firmware's P0 state: P0.2/P0.3/P0.4 high, P0.7 low.
    // These may be LED driver / boost enables.
    P0   = 0x7C; // P0.2,3,4,5,6 high (keep encoder latches high)
    P0CR = 0x9C; // P0.2,3,4,7 outputs; P0.5/6 stay inputs

    // Encoder A/B (P0.5/P0.6): inputs with pull-ups.
    GPIO_HIGH(0, _P0_5 | _P0_6);
    GPIO_INPUT(0, _P0_5 | _P0_6);
    GPIO_PULLUP_ON(0, _P0_5 | _P0_6);

    // Stock firmware leaves P5 at the default drive strength.
    DRVCON = DRVCON_UNLOCK_P5;
    P5DRV  = 0x00;
    DRVCON = DRVCON_LOCK;
}
