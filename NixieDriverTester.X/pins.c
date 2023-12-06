#include "pins.h"
#include "pps_outputs.h"
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

void InitI2CPins(void)
{
    // Set RA4 & RA5 as digital inputs (§25.2.2.3)
    TRISA = 0x38;
    
    // Remap the SDA/SLC pins to 4/5 (§18.2, Table 18-1; §18.3; §18.8.2)
    SSP1DATPPS = 4;
    SSP1CLKPPS = 5;
    RA4PPS = PPS_OUT_SDA1;
    RA5PPS = PPS_OUT_SCL1;    
}

void InitButtonPins(void)
{
    // Set RA1 as a discrete input for the button
    TRISA |= 0x02;
}

void InitPwmPins(void)
{
    // Remap the PWM output to RA2 (§18.3)
    RA2PPS = PPS_OUT_PWM3;
}

void InitAdcPins(void)
{
    // Set RA0 as an analog input for voltage monitoring
    TRISA |= 0x01;
    ANSELA |= 0x01;
}

void InitPins(void)
{
    // Clear the analog registers (§16.5; §24.1.2.1)
    ANSELA = 0x00;

    ResetI2C();
    
    PORTA = 0;
    TRISA = 0x00;
    
    InitAdcPins();
    InitI2CPins();
    InitButtonPins();
    InitPwmPins();
}
