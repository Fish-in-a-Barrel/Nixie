#include "pins.h"
#include "clock.h"
#include <xc.h>

// This function is used to convince client devices to let go of the bus if they have desynced from the host.
// This usually happens as a result of a controller reset (e.g. during debugging).
void ResetI2C(void)
{
    // Setup RA4 (SDA) as input and RA5 (SCL) as output
    TRISA = 0x10;
    RA5 = 1;
    
    // If SDA is being held low, toggle SCL
    while (!RA4)
    {
        RA5 = !RA5;
        __delay_ms(10);
    }
}

void InitPins(void)
{
    // Clear the analog registers (§16.5; §24.1.2.1)
    ANSELA = 0x00;

    ResetI2C();
    
    // Clear all inputs. Set RA2 so the inverted signal turns off the booster MOSFET
    TRISA = 0x00;
    PORTA = 0x04;
}
