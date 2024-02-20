#include <xc.h>

#include "pwm.h"
#include "timer.h"
#include "pps_outputs.h"

void InitPwmPins(void)
{
    // Remap the PWM output to RA2 (§18.3)
    RA2PPS = PPS_OUT_PWM3;
}

void InitPWM(uint16_t dutyCycle)
{
    InitPwmPins();
    
    SetPwmDutyCycle(dutyCycle);

    // Inverted do to level-shifting, enable
    PWM3CONbits.PWM3POL = 1;
    
    // Enable PWM3
    PWM3CONbits.EN = 1;
}

void SetPwmDutyCycle(uint16_t dutyCycle)
{
    // §23.11.2
    PWM3DC = dutyCycle << 6;
}