#include <xc.h>

#include "config_bits.h"
#include "clock.h"
#include "pins.h"
#include "button.h"
#include "pwm.h"
#include "i2c.h"
#include "oled.h"
#include "adc.h"

#define HV_TARGET 180
#define HV_DEADBAND 5
#define HV_MIN (HV_TARGET - HV_DEADBAND)
#define HV_MAX (HV_TARGET + HV_DEADBAND)

#define PWM_MIN 80
#define PWM_MAX 95

uint32_t gTickCount_32kHz = 0;
uint8_t gVoltage = 0;

uint8_t gNixieAutoIncrement = 1;
uint8_t gCurrentCathode = 11;

// This is a 10-bit fixed-precision int with 2 mantissa bits
uint16_t gPwmDutyCycle = PWM_MIN << 2;

void __interrupt() ISR()
{
    // Dispatch interrupts to handlers (§12.9.6)
    if (PIR1bits.SSP1IF || PIR1bits.BCL1IF) I2C_HandleInterrupt();
    
    if (PIR1bits.TMR2IF)
    {
        ++gTickCount_32kHz;
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
    PIE1bits.TMR2IE = 1;
}

void GetCurrentNixieVoltage(void)
{
    ADCON0bits.GO = 1;
    while (ADCON0bits.GO);
    
    // Pause global interrupts while reading the ADC value because this is not atomic.
    INTCONbits.GIE = 0;
    PIR1bits.ADIF = 0;
    uint16_t adc = ADRES;
    INTCONbits.GIE = 1;
    
    // (ADC_raw / 1024) * 4.096 = mV on pin.
    // multiply by 50 to compensate for the voltage divider supplying the pin.
    // This works out to 50 * 4.096 / 1024 = 0.2, or 1/5.
    gVoltage = (uint8_t)(adc / 5);
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
    uint8_t targetCathode = 11;
    static uint32_t nixieStartTickCount = 0;
    static uint32_t buttonStartTickCount = 0;
    
    // Shut off the nixie tube if the voltage is out of range.
    if ((gVoltage >= HV_MIN) && (gVoltage <= HV_MAX))
    {
        if (gNixieAutoIncrement)
        {
            // This doesn't handle roll-over of the tick counter, but that's not important for this application.
            if (gTickCount_32kHz - nixieStartTickCount > 32000)
            {
                nixieStartTickCount = gTickCount_32kHz;

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
                buttonStartTickCount = gTickCount_32kHz;
                break;
            case 0b11: // Transition to "released" state
                gCurrentCathode = (gCurrentCathode + 1) % 10;
                I2C_Write(0x0F, &gCurrentCathode, sizeof(gCurrentCathode));
                
                // If held for more than 2 seconds, toggle auto-increment
                if ((gTickCount_32kHz - buttonStartTickCount) > 64000)
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
    DrawCharacter(3, 0, 1);
    DrawCharacter(3, 1, 8);
    DrawCharacter(3, 2, 0);
    DrawCharacter(3, 4, 10);
    
    // Duty Cycle: -- %
    DrawCharacter(3, 10, 5);
    DrawCharacter(3, 11, 0);
    DrawCharacter(3, 13, 11);
    
    // Current driven tube: *
    DrawCharacter(0, 0, 12);
    
    // Button indicator: *
    DrawCharacter(0, 19, 12);
}

void RefreshDisplay()
{
    uint8_t counter = 0;
        
    DrawCharacter(0, 0, counter++ % 10);

    
    //
    // Voltage
    //
    
    uint8_t number = gVoltage;
    uint8_t digits[] = { 0, 0, 0 };
    int8_t i = 2;
    while (i >= 0)
    {
        digits[i--] = number % 10;
        number /= 10;
    }

    DrawCharacter(3, 0, digits[0]);
    DrawCharacter(3, 1, digits[1]);
    DrawCharacter(3, 2, digits[2]);
    
    //
    // Duty Cycle
    //
    
    number = gPwmDutyCycle >> 2;
    i = 1;
    while (i >= 0)
    {
        digits[i--] = number % 10;
        number /= 10;
    }

    DrawCharacter(3, 10, digits[0]);
    DrawCharacter(3, 11, digits[1]);
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
        GetCurrentNixieVoltage();
        AdjustVoltagePwm();
        UpdateNixieState();
        RefreshDisplay();
        
        __delay_ms(10);
    }
}
