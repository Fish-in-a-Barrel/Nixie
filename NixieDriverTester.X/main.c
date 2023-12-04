#include <xc.h>

#include "config_bits.h"
#include "clock.h"
#include "pins.h"
#include "button.h"
#include "pwm.h"
#include "i2c.h"
#include "oled.h"

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

void main(void)
{
    InitClock();
    InitPins();
    InitPWM(50);
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
        
        DrawCharacter(0, 0, counter++ % 10);
        
        __delay_ms(500);
    }
}
