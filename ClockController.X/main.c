#include <xc.h>

#include "config_bits.h"
#include "clock.h"
#include "i2c.h"

void __interrupt() ISR()
{
    if (PIR1bits.SSP1IF) I2C_HandleInterrupt();
    if (PIR1bits.BCL1IF) I2C_HandleCollisionInterrupt();
}

void EnableInterrupts()
{
    // Enable global and peripheral interrupts (§12.4)
    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1; 

    // Enable Interrupts
    PIE1bits.SSP1IE = 1;
    PIE1bits.BCL1IE = 1;
}

void main(void)
{
    I2C_Host_Init();
    EnableInterrupts();
    
    uint8_t counter = 0;
    uint8_t buffer = 0;
    while (1)
    {
        buffer = (++counter) % 4;
        I2C_Write(0x01, I2C_WRITE, &buffer, sizeof(buffer));

        __delay_ms(1000);
    }
}
