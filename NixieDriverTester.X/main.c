#include <xc.h>

#include "config_bits.h"
#include "clock.h"
#include "pins.h"
#include "button.h"
#include "pwm.h"

void main(void)
{
    InitClock();
    InitPins();
    InitPWM(50);
    
    while (1)
    {
        GetButtonState();
        
        __delay_ms(10);
    }
}
