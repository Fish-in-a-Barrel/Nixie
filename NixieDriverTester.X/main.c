#include <xc.h>

#include "config_bits.h"
#include "clock.h"
#include "pins.h"
#include "button.h"

void main(void)
{
    InitClock();
    InitPins();
    
    while (1)
    {
        GetButtonState();
        
        __delay_ms(10);
    }
}
