#include <xc.h>

#include "pwm.h"
#include "timer.h"

void InitPWM(uint16_t dutyCycle)
{
    SetPwmDutyCycle(dutyCycle);

    // Enable PWM3
    PWM3CON = 0x80;
}

void SetPwmDutyCycle(uint16_t dutyCycle)
{
    // §23.11.2
    PWM3DC = dutyCycle << 6;
}