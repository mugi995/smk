#include "pwm_hw.h"

#if PWM_HW_PRESENT

#    include "interrupts.h"

#    ifdef LED_COM_ISR
void led_scan_isr(void);
#    endif

void pwm_interrupt_handler(void) __interrupt(PWM_VECTOR)
{
#    ifdef LED_COM_ISR
    led_scan_isr();
#    endif
}

#endif
