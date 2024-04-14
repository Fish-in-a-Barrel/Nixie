#include "adc.h"

uint32_t gAdcAccumulator = 0;
uint16_t gAdcAccumulatorCount = 0;
uint16_t gAdcCv = 0;

volatile uint8_t gAdcResultIndex = 0;
volatile uint16_t gAdcResult[2] = { 0, 0 };

void InitAdcPins(void)
{
    // Set RC2 as an analog input for voltage monitoring
    TRISC |= 0x04;
    ANSELC |= 0x04;
}

void InitAdc(void)
{
    InitAdcPins();
    
    // Enable up the fixed voltage reference at 4.096V (§26.3.1)
    FVRCON = 0x83;
    
    // Select the ADCRC clock (§27.4.2)
    ADCON1bits.CS = 0x7;
    
    // Right-justify the result registers  (§27.4.2)
    ADCON1bits.FM = 0x1;
    
    // Use the internal fixed voltage reference (FVR)  (§27.4.2)
    ADCON1bits.ADPREF = 0x3;
    
#ifndef BREADBOARD
    // Select RA0 as the ADC channel (§27.4.1)
    ADCON0bits.CHS = 0;
#else
    // Select RC5 as the ADC channel (§27.4.1)
    ADCON0bits.CHS = 0x15;
#endif
    
    // Trigger acquisition with TMR2 post-scaled (§27.4.3)
    ADACTbits.ACT = 0x4;
    
    // Enable the ADC (§27.4.1)
    ADCON0bits.ON = 1;
}
