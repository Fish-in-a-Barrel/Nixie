#include "timer.h"
#include "clock.h"

uint32_t gTickCount = 0;

#if TMR2_RESET > 0xFF
/*
    The timer reset value is too large (greater than 255). To fix:
    - pre-scale the clock (§21.10.3)
    - lower the system oscillator frequency
    - decrease the frequency
 */
#error Timer reset value to large for T2PR
#endif

void InitTimer()
{
    // Use the F_osc/4 source, as required for PWM (§21.10.5, §23.9)
    T2CLKCON = 0x1;
    
    // Set the timer period (§21.10.2)
    T2PR = TMR2_RESET;
    
    // Mode is free-running, period-pulse, software-gated (§21.10.4)
    T2HLT = 0x00;    

    // 1:1 pre-scaler, configurable post-scaler (§21.10.3)
    T2CONbits.CKPS = 0x0;
    T2CONbits.OUTPS = TMR2_POST - 1;
    
    // Enable the timer
    T2CONbits.ON = 1;
}

void TimerInterruptHandler(void)
{
    PIR1bits.TMR2IF = 0;
    ++gTickCount;
}
