#include <xc.h>

#include "config_bits.h"
#include "clock.h"
#include "pins.h"
#include "button.h"
#include "pwm.h"
#include "i2c.h"
#include "oled.h"
#include "adc.h"

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

void CheckAdc(void)
{
    //if (!PIR1bits.ADIF) return;    
    
    ADCON0bits.GO = 1;
    while (ADCON0bits.GO);
    
    // Pause global interrupts while reading the ADC value because this is not atomic.
    INTCONbits.GIE = 0;
    PIR1bits.ADIF = 0;
    uint16_t adc = ADRES;
    INTCONbits.GIE = 1;
    
    // Raw value in mV
    adc *= 4;
    
    uint8_t digits[] = { 0, 0, 0 };
    uint8_t i = 2;
    while (adc > 0)
    {
        digits[i--] = adc % 10;
        adc /= 10;
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
    InitPWM(50);
    InitAdc();
    I2C_Host_Init();
    
    EnableInterrupts();
    
    SetupDisplay();
    
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
    
    uint8_t counter = 0;
    
    while (1)
    {
        GetButtonState();
        CheckAdc();
        
        DrawCharacter(0, 0, counter++ % 10);
        
        __delay_ms(50);
    }
}
