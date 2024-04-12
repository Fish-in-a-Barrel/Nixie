#include "pins.h"
#include "pps_outputs.h"
#include "pps_inputs.h"
#include "clock.h"
#include <xc.h>

// This function is used to convince client devices to let go of the bus if they have desynced from the host.
// This usually happens as a result of a controller reset (e.g. during debugging).
void ResetI2C(void)
{
    // Setup RC1 (SDA) as input and RC0 (SCL) as output
    TRISC = 0x02;
    RC0 = 1;
    
    // If SDA is being held low, toggle SCL
    while (!RC1)
    {
        RC0 = !RC0;
        __delay_ms(10);
    }
    
    // Clear the outputs
    PORTC = 0;
}

void InitI2CPins(void)
{
    ResetI2C();
    
    // Set RC0 & RC1 as digital inputs (§25.2.2.3)
    TRISC |= 0x03;
    
    // Remap the SDA/SLC pins to 4/5 (§18.2, Table 18-1; §18.8.1, §18.8.2)
    SSP1CLKPPS = PPS_INPUT(PPS_PORT_C, 0);
    SSP1DATPPS = PPS_INPUT(PPS_PORT_C, 1);
    RC0PPS = PPS_OUT_SCL1;    
    RC1PPS = PPS_OUT_SDA1;
}

void InitSerialPins(void)
{
    // Set RA5 as digital input for RX
    TRISA |= 0x20;

    // Remap the RX pin to RA5, and TX to RA4 (§18.2, Table 18-1; §18.8.1, §18.8.2; §24)
    RX1PPS = PPS_INPUT(PPS_PORT_A, 5);
    RA4PPS = PPS_OUT_TX1;
}

void InitPins(void)
{
    // Clear the analog registers (§16.5; §24.1.2.1)
    ANSELA = 0x00;
    ANSELC = 0x00;

    TRISA = 0x00;
    TRISC = 0x00;
    
    PORTA = 0;
    PORTC = 0;
    
    InitI2CPins();
    InitSerialPins();
}
