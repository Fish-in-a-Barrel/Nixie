#include "boost_control.h"
#include "timer.h"
#include "pwm.h"
#include "adc.h"

#include <xc.h>

#ifndef SKIP_PD
    // This defines the maximum allowed voltage (~200V).
    #define ADC_HI_LIMIT 787L
#else
    // ~12V if we're in debug and running off the PICKit.
    #define ADC_HI_LIMIT 47L
#endif

// The ADC returns a 10-bit value dividing the range 0mv to 4096mV evenly so mV/4 = ADC.
// The expected voltage from the voltage divider is ~2,835mV, which means the expected ADC value is ~709. This will
// vary with the exact resistance of the resistors in the voltage divider.
#define ADC_SP 721L
#define ADC_DEADBAND 5

// The voltage booster is controlled by changing the duty-cycle (DC) of the PWM signal driving it.
// By scaling the working DC value, we can have a working value with more granularity than the actual PWM allows. This
// can allow for some smoother control, depending on the algorithm.
#define PWM_DC_SCALAR 0x20
#define PWM_DC_100 ((TMR2_RESET << 2) * PWM_DC_SCALAR)
#define PWM_DC_MAX (uint16_t)(0.95 * PWM_DC_100)

// Control constants when over SP
#define PID_P_NUM_HI 1
#define PID_P_DENOM_HI 1
#define PID_DPWMDC_MAX_HI 0x1

// Control constants when below SP
#define PID_P_NUM_LO 1
#define PID_P_DENOM_LO 2
#define PID_DPWMDC_MAX_LO 0x1

static uint16_t gPwmDutyCycle = (uint16_t)(0.8 * PWM_DC_100);
static uint8_t gOverVoltageProtection = 0;

void BoostConverter_Init(void)
{
    SetPwmDutyCycle(gPwmDutyCycle);
}

void BoostConverter_Update(uint16_t adc)
{
#ifdef SKIP_PD
    return;
#endif
    
    // If save voltage levels are exceeded, stop the PWM.
    if (adc > ADC_HI_LIMIT)
    {
        PWM_Disable();
        gOverVoltageProtection = 1;
        
        return;
    }
    
    // While in OVP, wait for voltage to drop back to a save level before halving the PWM DC and restarting.
    if (gOverVoltageProtection)
    {
        if (adc > ADC_SP) return;
        
        gPwmDutyCycle /= 2;
        PWM_Enable(gPwmDutyCycle / PWM_DC_SCALAR);
        
        gOverVoltageProtection = 0;
        return;
    }
    
    int16_t cvError = (int16_t)adc - ADC_SP;
    uint16_t dPwmDc = 0;
    if (cvError > 0)
    {
        dPwmDc = (uint16_t)(cvError * PID_P_NUM_HI);
        //dPwmDc = (uint16_t)((cvError * PID_P_NUM_HI) / PID_P_DENOM_HI);
        if (dPwmDc > PID_DPWMDC_MAX_HI) dPwmDc = PID_DPWMDC_MAX_HI;
        
        if (dPwmDc <= gPwmDutyCycle) gPwmDutyCycle -= dPwmDc;
        else dPwmDc = 0;
    }
    else if (cvError < 0)
    {
        dPwmDc = (uint16_t)(-cvError * PID_P_NUM_LO);
        //dPwmDc = (uint16_t)((cvError * PID_P_NUM_LO) / PID_P_DENOM_LO);
        if (dPwmDc > PID_DPWMDC_MAX_LO) dPwmDc = PID_DPWMDC_MAX_LO;
        
        if (gPwmDutyCycle + dPwmDc < PWM_DC_100) gPwmDutyCycle += dPwmDc;
        else dPwmDc = PWM_DC_100;
    }
    
    SetPwmDutyCycle(gPwmDutyCycle / PWM_DC_SCALAR);
}

uint8_t BoostConverter_GetVoltage(void)
{
    // (4096 * ADC) / 1024 = mV on pin.
    // multiply by 63.5 to compensate for the voltage divider supplying the pin.
    // divide by 1000 to convert from mV to volts
    // This works out to (63.5 * (4096 / 1024)) / 1000 = 0.254, or ~(1/4 + 1/250).
    // The exact values used will depend on the precise value of the resistors.
    return (uint8_t)(gAdcCv / 4) - (uint8_t)(gAdcCv / 330);
}

uint16_t BoostConverter_GetDutyCycle(void)
{
    return gPwmDutyCycle / PWM_DC_SCALAR;
}

uint16_t BoostConverter_GetDutyCyclePct(void)
{
    return gPwmDutyCycle / (PWM_DC_100 / 100);
}

uint8_t BoostConverter_OverVoltageProtectionOn(void)
{
    return gOverVoltageProtection;
}