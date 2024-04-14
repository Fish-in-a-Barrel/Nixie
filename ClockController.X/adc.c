#include "adc.h"

uint16_t gAdcCv = 0;
uint8_t gVoltage = 0;

static volatile uint32_t gAdcAccumulator = 0;
static volatile uint16_t gAdcAccumulatorCount = 0;

static volatile uint8_t gAdcResultIndex = 0;
static volatile uint16_t gAdcResult[2] = { 0, 0 };

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

void AdcInterruptHandler(void)
{
    PIR1bits.ADIF = 0;
    gAdcAccumulator += ADRES;
    ++gAdcAccumulatorCount;
}

void CaptureAdc(void)
{
    if (0 == gAdcAccumulatorCount)
    {
        gVoltage = 0;
        return;
    }
    
    uint32_t accum;
    uint32_t count;
    
    // Pause global interrupts while reading the ADC value because this is not atomic.
    INTCONbits.GIE = 0;
    accum = gAdcAccumulator;
    count = gAdcAccumulatorCount;
    gAdcAccumulator = 0;
    gAdcAccumulatorCount = 0;
    INTCONbits.GIE = 1;
    
    gAdcCv = (uint16_t)(accum / count);
    
    // (4096 * ADC_raw) / 1024 = mV on pin.
    // multiply by 63.5 to compensate for the voltage divider supplying the pin.
    // divide by 1000 to convert from mV to volts
    // This works out to (63.5 * (4096 / 1024)) / 1000 = 0.254, or ~(1/4 + 1/250).
    // The exact values used will depend on the precise value of the resistors.
    gVoltage = (uint8_t)(gAdcCv / 4) + (uint8_t)(gAdcCv / 210);
}
