#include <xc.h>

#include "pwm.h"
#include "clock.h"

#define TMR2_RESET ((_XTAL_FREQ / 4) / _PWM_FREQ)

#if TMR2_RESET > 0xFF
/*
    The timer reset value is too large (greater than 255). To fix:
    - pre-scale the clock (§21.10.3)
    - lower the system oscillator frequency
    - decrease the PWM frequency
 */
#error Timer reset value to large for T2PR
#endif

void InitPWM(uint16_t dutyCycle)
{
    // Use the F_osc/4 source, as required for PWM (§21.10.5, §23.9)
    T2CLKCON = 0x1;
    
    // 250 tick counter reset results (§21.10.2)
    T2PR = TMR2_RESET;
    SetPwmDutyCycle(dutyCycle);
    
    // Mode is free-running, period-pulse, software-gated (§21.10.4)
    T2HLT = 0x00;    
    
    // Enable the timer with a 1:1 pre-scaler (§21.10.3)
    T2CON = 0x80 | 0x00 | 0x00;

    // Enable PWM3
    PWM3CON = 0x80;
}

void SetPwmDutyCycle(uint16_t dutyCycle)
{
    uint16_t pwm = dutyCycle * TMR2_RESET / 100;
    
    // §23.11.2
    PWM3DC = pwm << 6;
}