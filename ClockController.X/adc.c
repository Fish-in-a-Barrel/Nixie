#include "adc.h"
#include "pps_inputs.h"
#include "boost_control.h"

uint16_t gAdcCv = 0;

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
    
    // Select RC2 as the ADC channel (§27.4.1)
    ADCON0bits.CHS = PPS_INPUT(PPS_PORT_C, 2); 
    
    // Trigger acquisition with PWM3, which is driving the boost converter (§27.4.3)
    ADACTbits.ACT = 0x4;
    
    // Enable the ADC (§27.4.1)
    ADCON0bits.ON = 1;
}

void AdcInterruptHandler(void)
{
    PIR1bits.ADIF = 0;
    
    gAdcCv = ADRES;
    BoostConverter_Update(gAdcCv);
}
