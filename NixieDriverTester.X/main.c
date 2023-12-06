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
#define PWM_MIN 80
#define PWM_MAX 95

uint8_t gVoltage = 0;

// This is a 10-bit fixed-precision int with 2 mantissa bits
uint16_t gPwm = PWM_MIN << 2;

void __interrupt() ISR()
{
    // Dispatch interrupts to handlers (§12.9.6)
    if (PIR1bits.SSP1IF || PIR1bits.BCL1IF) I2C_HandleInterrupt();
}

void EnableInterrupts()
{
    // Enable global and peripheral interrupts (§12.4)
    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1; 

    // Enable MSSP Interrupts (§12.9.3)
    PIE1bits.SSP1IE = 1;
    PIE1bits.BCL1IE = 1;
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
    if (HV_TARGET - HV_DEADBAND > gVoltage)
    {
        if (PWM_MAX < gPwm >> 2) SetPwmDutyCycle(++gPwm);
    }
    else if (HV_TARGET + HV_DEADBAND < gVoltage)
    {
        if (PWM_MIN > gPwm >> 2) SetPwmDutyCycle(--gPwm);
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

    uint8_t number = gVoltage;
    uint8_t digits[] = { 0, 0, 0 };
    uint8_t i = 2;
    while (number > 0)
    {
        digits[i--] = number % 10;
        number /= 10;
    }

    // Voltage: --- V
    DrawCharacter(3, 0, digits[0]);
    DrawCharacter(3, 1, digits[1]);
    DrawCharacter(3, 2, digits[2]);
}

void main(void)
{
    InitClock();
    InitPins();
    InitPWM(gPwm);
    InitAdc();
    I2C_Host_Init();
    
    EnableInterrupts();
    
    SetupDisplay();
    DrawStaticDisplaySymbols();
    
    while (1)
    {
        GetButtonState();
        
        // TODO: react to button presses
        
        GetCurrentNixieVoltage();
        AdjustVoltagePwm();
        
        // TODO: disable nixie tubes if the voltage is out of band.
        
        // TODO: auto-increment nixie cathode
        
        RefreshDisplay();
        
        __delay_ms(50);
    }
}
