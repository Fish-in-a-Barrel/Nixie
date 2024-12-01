#include "boost_control.h"
#include "timer.h"
#include "pwm.h"
#include "adc.h"

#include <xc.h>

// The ADC returns a 10-bit value dividing the range 0mv to 4096mV evenly so mV/4 = ADC.
// The expected voltage from the voltage divider is ~2,835mV, which means the expected ADC value is ~709. This will vary with the exact resistance of
// the resistors in the voltage divider.
#define ADC_SP 709L
#define ADC_DEADBAND 5

// The voltage booster is controlled by changing the duty-cycle (DC) of the PWM signal driving it.
// By scaling the working DC value, we can have a working value with more granularity than the actual PWM allows. This can allow for some smoother
// control, depending on the algorithm.
#define PWM_DC_SCALAR 32
#define PWX_DC_100 ((TMR2_RESET << 2) * PWM_DC_SCALAR)
#define PWM_DC_MIN (uint16_t)(0.85 * PWX_DC_100)
#define PWM_DC_MAX (uint16_t)(0.95 * PWX_DC_100)

static uint16_t gPwmDutyCycle = (PWM_DC_MIN + PWM_DC_MAX) / 2;

void InitBoostConverter(void)
{
    SetPwmDutyCycle(gPwmDutyCycle);
}

void UpdateBoostConverter()
{
    CaptureAdc();

    if (gAdcCv + ADC_DEADBAND < ADC_SP) ++gPwmDutyCycle;
    else if (gAdcCv - ADC_DEADBAND > ADC_SP) --gPwmDutyCycle;
    
    if (gPwmDutyCycle > PWM_DC_MAX) gPwmDutyCycle = PWM_DC_MAX;
    else if (gPwmDutyCycle < PWM_DC_MIN) gPwmDutyCycle = PWM_DC_MIN;
    
    SetPwmDutyCycle(gPwmDutyCycle / PWM_DC_SCALAR);
}

uint16_t BoostConverter_GetDutyCycle(void)
{
    return gPwmDutyCycle / PWM_DC_SCALAR;
}

uint16_t BoostConverter_GetDutyCyclePct(void)
{
    return gPwmDutyCycle / (PWX_DC_100 / 100);
}
