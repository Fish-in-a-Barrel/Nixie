#include <xc.h>

#include "pwm.h"
#include "timer.h"
#include "pps_outputs.h"

void InitPwmPins(void)
{
    // Remap the PWM output to RA2 (§18.3)
    RA2PPS = PPS_OUT_PWM3;
}

void InitPWM()
{
    InitPwmPins();
    
    // Enable PWM3
    PWM3CONbits.EN = 1;
}

void PWM_Enable(uint16_t dutyCycle)
{
    // §23.11.2
    PWM3DC = dutyCycle << 6;

    PWM3CONbits.EN = 1;
}

void PWM_Disable(void)
{
    PWM3CONbits.EN = 0;
}

#pragma warning disable 1510 // ignore code duplication
void SetPwmDutyCycle(uint16_t dutyCycle)
{
    // §23.11.2
    PWM3DC = dutyCycle << 6;
}