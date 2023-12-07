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

#define CATHODE_NONE 10

#ifdef BREADBOARD
#define HV_TARGET 120
#define HV_DEADBAND 5
#define HV_MIN (HV_TARGET - HV_DEADBAND)
#define HV_MAX (HV_TARGET + HV_DEADBAND)

#define PWM_MIN 65
#define PWM_MAX 90
#else
#define HV_TARGET 180
#define HV_DEADBAND 5
#define HV_MIN (HV_TARGET - HV_DEADBAND)
#define HV_MAX (HV_TARGET + HV_DEADBAND)

#define PWM_MIN 80
#define PWM_MAX 95
#endif

uint32_t gTickCount = 0;
uint8_t gVoltage = 0;

uint8_t gNixieAutoIncrement = 1;
uint8_t gCurrentCathode = CATHODE_NONE;

// This is a 10-bit fixed-precision int with 2 mantissa bits
uint16_t gPwmDutyCycle = PWM_MIN << 2;

void __interrupt() ISR()
{
    // Dispatch interrupts to handlers (§12.9.6)
    if (PIR1bits.SSP1IF || PIR1bits.BCL1IF) I2C_HandleInterrupt();
    
    if (PIR1bits.TMR2IF)
    {
        ++gTickCount;
        PIR1bits.TMR2IF = 0;
    }
}

void EnableInterrupts()
{
    // Enable global and peripheral interrupts (§12.4)
    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1; 

    // Enable MSSP interrupts (§12.9.3)
    PIE1bits.SSP1IE = 1;
    PIE1bits.BCL1IE = 1;
    
    // Enable the TMR2 interrupt for tick counting
    //PIE1bits.TMR2IE = 1;
}

void GetCurrentVoltage(void)
{
    ADCON0bits.GO = 1;
    while (ADCON0bits.GO);
    
    // Pause global interrupts while reading the ADC value because this is not atomic.
    INTCONbits.GIE = 0;
    PIR1bits.ADIF = 0;
    uint16_t adc = ADRES;
    INTCONbits.GIE = 1;
    
#ifndef BREADBOARD
    // (ADC_raw / 1024) * 4.096 = V on pin.
    // multiply by 50 to compensate for the voltage divider supplying the pin.
    // This works out to 50 * 4.096 / 1024 = 0.2, or 1/5.
    gVoltage = (uint8_t)(adc / 5);
#else
    // this will be roughly 1/10s of volts
    gVoltage = (uint8_t)((adc / 5) - (adc / 65));
#endif
}

void AdjustVoltagePwm(void)
{
    // No real control strategy here. Just push the voltage towards the SP by adjusting the PWM slowly.
    if (gVoltage < HV_MIN)
    {
        if (PWM_MAX > gPwmDutyCycle >> 2) SetPwmDutyCycle(++gPwmDutyCycle);
    }
    else if (gVoltage > HV_MAX)
    {
        if (PWM_MIN < gPwmDutyCycle >> 2) SetPwmDutyCycle(--gPwmDutyCycle);
    }
}

void UpdateNixieState(void)
{
    uint8_t targetCathode = CATHODE_NONE;
    static uint32_t nixieStartTickCount = 0;
    static uint32_t buttonStartTickCount = 0;
    
    // Shut off the nixie tube if the voltage is out of range.
    if ((gVoltage >= HV_MIN) && (gVoltage <= HV_MAX))
    {
        if (gNixieAutoIncrement)
        {
            // This doesn't handle roll-over of the tick counter, but that's not important for this application.
            if (gTickCount - nixieStartTickCount < _PWM_FREQ)
            {
                nixieStartTickCount = gTickCount;

                targetCathode = (11 == gCurrentCathode) ? 0 : (gCurrentCathode + 1) % 10;
            }
        }
        else if (11 == gCurrentCathode)
        {
            targetCathode = 0;
        }
        
        switch (GetButtonState())
        {
            case 0b10: // Transition to "pressed" state
                buttonStartTickCount = gTickCount;
                break;
            case 0b11: // Transition to "released" state
                gCurrentCathode = (gCurrentCathode + 1) % 10;
                I2C_Write(0x0F, &gCurrentCathode, sizeof(gCurrentCathode));
                
                // If held for more than 2 seconds, toggle auto-increment
                if ((gTickCount - buttonStartTickCount) < 2 * _PWM_FREQ)
                {
                    gNixieAutoIncrement = !gNixieAutoIncrement;
                }
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
    DrawCharacter(3, 4, CHAR_V);
    
    // Duty Cycle: -- %
    DrawCharacter(3, 13, CHAR_PCT);
}

void DisplayNumber(uint8_t number, int8_t digitCount, uint8_t x, uint8_t row)
{
    while (digitCount > 0)
    {
        DrawCharacter(row, (uint8_t)(x + --digitCount), number % 10);
        number /= 10;
    }
}

void RefreshDisplay()
{
    static uint8_t ticker = 0;
    for (uint8_t i = 0; i < 4; ++i)
        DrawCharacter(i, 20, i == ticker % 4 ? CHAR_AST : CHAR_SPC);

    ++ticker;
    
    uint8_t nixieState = (gCurrentCathode > CATHODE_NONE) ? gCurrentCathode : CHAR_DSH;
    uint8_t autoState = gNixieAutoIncrement ? CHAR_AST : CHAR_SPC;
    
    DrawCharacter(0, 0, nixieState);
    DrawCharacter(0, 1, autoState);

    DisplayNumber(gVoltage, 3, 0, 3);
    DisplayNumber((uint8_t)(gPwmDutyCycle >> 2), 2, 10, 3);
}

void main(void)
{
    InitClock();
    InitPins();
    InitPWM(gPwmDutyCycle);
    InitAdc();
    I2C_Host_Init();
    
    EnableInterrupts();
    
    SetupDisplay();
    DrawStaticDisplaySymbols();
    
    while (1)
    {
        GetCurrentVoltage();
        AdjustVoltagePwm();
        UpdateNixieState();
        RefreshDisplay();
        
        __delay_ms(100);
    }
}
