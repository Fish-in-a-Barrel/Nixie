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
#ifndef BREADBOARD
    // Set RA1 as a discrete input for the button
    TRISA |= 0x02;
#else
    // Set RB6 as a discrete input for the button
    TRISB |= 0x40;
    //WPUB |= 0x40;
#endif
}

void InitPwmPins(void)
{
#ifndef BREADBOARD
    // Remap the PWM output to RA2 (§18.3)
    RA2PPS = PPS_OUT_PWM3;
#else
    // Remap the PWM output to RB7 (§18.3)
    RB7PPS = PPS_OUT_PWM3;
#endif
}

void InitAdcPins(void)
{
#ifndef BREADBOARD
    // Set RA0 as an analog input for voltage monitoring
    TRISA |= 0x01;
    ANSELA |= 0x01;
#else
    // Set RC5 as an analog input for voltage monitoring
    TRISC |= 0x20;
    ANSELC |= 0x20;
#endif
}

void InitPins(void)
{
    // Clear the analog registers (§16.5; §24.1.2.1)
    ANSELA = 0x00;

#ifdef BREADBOARD
    TRISB = 0x00;
    ANSELB = 0x00;
    PORTB = 0x00;

    TRISC = 0x00;
    ANSELC = 0x00;
    PORTC = 0x00;
#endif

    ResetI2C();
    
    PORTA = 0x00;
    TRISA = 0x00;
        
    InitAdcPins();
    InitI2CPins();
    InitButtonPins();
    InitPwmPins();
}
