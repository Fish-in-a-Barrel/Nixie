#include "adc.h"

volatile uint8_t gAdcResultIndex = 0;
volatile uint16_t gAdcResult[2] = { 0, 0 };

void InitAdc(void)
{
    // Enable up the fixed voltage reference at 4.096V (§26.3.1)
    FVRCON = 0x83;
    
    // Select the F_osc/64 clock for the slowest possible conversion (§27.4.2)
    ADCON1bits.CS = 0x5;
    
    // Right-justify the result registers  (§27.4.2)
    ADCON1bits.FM = 0x1;
    
    // Use the internal fixed voltage reference (FVR)  (§27.4.2)
    ADCON1bits.ADPREF = 0x3;
    
    // Select RA0 as the ADC channel (§27.4.1)
#ifndef BREADBOARD
    // Select RA0 as the ADC channel (§27.4.1)
    ADCON0bits.CHS = 0;
#else
    // Select RC5 as the ADC channel (§27.4.1)
    ADCON0bits.CHS = 0x15;
#endif
    
    // Trigger acquisition with PWM3 (§27.4.3)
    ADACTbits.ACT = 0x7;
    
    // Enable the ADC (§27.4.1)
    ADCON0bits.ON = 1;
}
