#include <xc.h>

#include "config_bits.h"
#include "clock.h"
#include "pins.h"
#include "button.h"
#include "pwm.h"
#include "i2c.h"
#include "oled.h"
#include "font8x5.h"
#include "adc.h"
#include "timer.h"

#define CATHODE_NONE 10

#ifdef BREADBOARD
#define HV_TARGET 120
#define HV_DEADBAND 2
#define HV_MIN (HV_TARGET - 2 * HV_DEADBAND)
#define HV_MAX (HV_TARGET + 2 * HV_DEADBAND)

#define ADC_SP 660L
#define PWM_SCALAR 64
#define PWM_MIN 105 * PWM_SCALAR
#define PWM_MAX 145 * PWM_SCALAR
#else
#define HV_TARGET 180
#define HV_DEADBAND 5
#define HV_MIN (HV_TARGET - 2 * HV_DEADBAND)
#define HV_MAX (HV_TARGET + 2 * HV_DEADBAND)

// This should be 1/4 the expected voltage from the voltage divider mV.
// Theoretically this should be 2,800mV, but it will vary with the exact resistance of the resistors in the voltage divider.
#define ADC_SP 711L

#define PWM_SCALAR 32
#define PWM_MIN 70 * PWM_SCALAR
#define PWM_MAX 81 * PWM_SCALAR
#endif

uint8_t gNixieAutoIncrement = 1;
uint8_t gCurrentCathode = CATHODE_NONE;

uint16_t gPwmDutyCycle = PWM_MIN * PWM_SCALAR;

uint32_t gAdcAccumulator = 0;
uint16_t gAdcAccumulatorCount = 0;
uint16_t gAdcCv = 0;

uint8_t gVoltage = 0;

uint8_t gLastButtonState = BUTTON_STATE_RELEASED;

void __interrupt() ISR()
{
    // Dispatch interrupts to handlers (�12.9.6)
    if (PIR1bits.SSP1IF || PIR1bits.BCL1IF) I2C_HandleInterrupt();
    
    if (PIR1bits.TMR2IF) TimerInterruptHandler();
    
    if (PIR1bits.ADIF)
    {
        PIR1bits.ADIF = 0;
        gAdcAccumulator += ADRES;
        ++gAdcAccumulatorCount;
    }
}

void EnableInterrupts()
{
    // Enable global and peripheral interrupts (�12.4)
    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1; 

    // Enable MSSP interrupts (�12.9.3)
    PIE1bits.SSP1IE = 1;
    PIE1bits.BCL1IE = 1;
    
    // Enable the TMR2 interrupt for tick counting
    PIE1bits.TMR2IE = 1;

    // Enable ACD interrupt
    PIR1bits.ADIF = 0;
    PIE1bits.ADIE = 1;
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
    
    gAdcCv =(uint16_t)(accum / count);
    
#ifndef BREADBOARD
    // (4096 * ADC_raw) / 1024 = mV on pin.
    // multiply by 63.5 to compensate for the voltage divider supplying the pin.
    // divide by 1000 to convert from mV to volts
    // This works out to (63.5 * (4096 / 1024)) / 1000 = 0.254, or ~(1/4 + 1/250).
    gVoltage = (uint8_t)(gAdcCv / 4) + (uint8_t)(gAdcCv / 210);
#else
    // this will be roughly 1/10s of volts
    gVoltage = (uint8_t)((gAdcCv / 5) - (gAdcCv / 52));
#endif
}

void AdjustVoltagePwm(void)
{
    const int16_t DELTA_PWM_MAX = PWM_SCALAR * 500;
    const int16_t I_MAX = 5 * PWM_SCALAR / 2;
    const int16_t Kp_N = 4;
    const int16_t Kp_D = 5;
    const int16_t Ki_N = 5;
    const int16_t Ki_D = PWM_SCALAR * 4;
    
    static int16_t i = 0;

    int16_t error = PWM_SCALAR * (int16_t)(ADC_SP - gAdcCv);
    int16_t p = (error * Kp_N) / Kp_D;
    i += error;
    
    // prevent integral wind-up
    //i = (9 * i) / 10;
    if (i > I_MAX) i = I_MAX;
    if (i < -I_MAX) i = -I_MAX;    
    
    int16_t delta = p + ((Ki_N * i) / Ki_D);
    
    if (delta > DELTA_PWM_MAX) delta = DELTA_PWM_MAX;
    if (delta < -DELTA_PWM_MAX) delta = -DELTA_PWM_MAX;
    
    gPwmDutyCycle = (uint16_t)((int16_t)gPwmDutyCycle + delta);
    
    if (gPwmDutyCycle > PWM_MAX) gPwmDutyCycle = PWM_MAX;
    else if (gPwmDutyCycle < PWM_MIN) gPwmDutyCycle = PWM_MIN;
    
    SetPwmDutyCycle(gPwmDutyCycle / PWM_SCALAR);
}

void DisplayNumber(uint16_t number, int8_t digitCount, uint8_t row, uint8_t x)
{
    while (digitCount > 0)
    {
        DrawCharacter(row, (uint8_t)(x + --digitCount), number % 10);
        number /= 10;
    }
}

void UpdateNixieState(void)
{
    uint8_t targetCathode = gCurrentCathode;
    static uint32_t nixieStartTickCount = 0;
    static uint32_t buttonStartTickCount = 0;
    
    // Shut off the nixie tube if the voltage is out of range.
    if ((gVoltage < HV_MIN) || (gVoltage > HV_MAX))
    {
        targetCathode = CATHODE_NONE;
    }
    else
    {
        if (CATHODE_NONE == gCurrentCathode)
        {
            nixieStartTickCount = gTickCount;
            targetCathode = 0;
        }
        
        if (gNixieAutoIncrement)
        {
            // This doesn't handle roll-over of the tick counter, but that's not important for this application.
            if (gTickCount - nixieStartTickCount > TICK_FREQ)
            {
                nixieStartTickCount = gTickCount;
                targetCathode = (gCurrentCathode + 1) % 10;
            }
        }
        
        if ((gLastButtonState != gButtonState) && (BUTTON_STATE_RELEASED == gButtonState))
        {
            if (gLongPress) gNixieAutoIncrement = !gNixieAutoIncrement;
            if (!gNixieAutoIncrement) targetCathode = (gCurrentCathode + 1) % 10;
        }
    }
    
    if (targetCathode != gCurrentCathode)
    {
        gCurrentCathode = targetCathode;
        I2C_Write(0x0F, &gCurrentCathode, sizeof(gCurrentCathode));        
    }
}

void DrawStaticDisplaySymbols(void)
{
        // Voltage: --- V
    DrawCharacter(0, 14, CHAR_V);
    
    // Duty Cycle: -- %
    DrawCharacter(3, 13, CHAR_SLH);
    DisplayNumber(TMR2_RESET << 2, 4, 3, 15);
}

void RefreshDisplay()
{
    uint32_t lastUpdateTick = 0;
    if (gTickCount - lastUpdateTick < TMR2_FREQ / 10) return;
    lastUpdateTick = gTickCount;
    
    // Scroll * vertically for proof of life
    for (uint8_t i = 0; i < 4; ++i)
        DrawCharacter(i, 20, i == (gTickCount / (TMR2_FREQ / 40)) % 4 ? CHAR_AST : CHAR_SPC);
    
    //
    // Nixie state
    //
    
    uint8_t nixieState = (gCurrentCathode != CATHODE_NONE) ? gCurrentCathode : CHAR_DSH;
    uint8_t autoState = gNixieAutoIncrement ? CHAR_AST : CHAR_SPC;
    
    DrawCharacter(0, 0, nixieState);
    DrawCharacter(0, 2, autoState);

    //
    // Voltage and duty cycle
    //
    
    
    DisplayNumber(gAdcCv, 4, 3, 0);
    DisplayNumber(gVoltage, 3, 0, 10);
    DisplayNumber(gPwmDutyCycle / PWM_SCALAR, 4, 3, 8);

    //
    // Button pressed indicator
    //

    if (gLastButtonState != gButtonState) InvertDisplay(!gButtonState);
}

void main(void)
{
    InitClock();
    InitPins();
    InitTimer();
    InitPWM(gPwmDutyCycle / PWM_SCALAR);
    InitAdc();
    I2C_Host_Init();
    
    __delay_ms(50);
    
    EnableInterrupts();
    
    SetupDisplay();
    DrawStaticDisplaySymbols();
    
    while (1)
    {
        CaptureAdc();
        AdjustVoltagePwm();
        
        UpdateButtonState();
        UpdateNixieState();
        RefreshDisplay();
        
        gLastButtonState = gButtonState;
        
        __delay_ms(5);
    }
}
