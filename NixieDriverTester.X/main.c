#include <xc.h>

#include "config_bits.h"
#include "clock.h"
#include "pins.h"
#include "button.h"
#include "pwm.h"
#include "i2c.h"

#define DISPLAY_I2C_ADDRESS 0x3C

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

void InitDisplay(void)
{
    const uint8_t init1[] =
    {
        0x00,           // Start command sequence
        0xAE,           // Display off (10.1.12)
        0xD5, 0x80,     // Set display clock divider (10.1.16)
        0xA8, 0x3F      // Set multiplex ratio (10.1.11)
    };
    
    I2C_Write(DISPLAY_I2C_ADDRESS, init1, sizeof(init1));
    
    const uint8_t init2[] =
    {
        0x00,           // Start command sequence
        0xD3, 0x00,     // Set display offset: 0 (10.1.15)
        0x40 | 0x00,    // Set display start line (10.1.6)
        0x8D, 0x14      // Set charge pump (2.1 of the application note)
    };
    
    I2C_Write(DISPLAY_I2C_ADDRESS, init2, sizeof(init2));
    
    const uint8_t init3[] =
    {
        0x00,           // Start command sequence
        0x20, 0x00,     // Set memory mode: horizontal addressing (10.1.3)
        0xA0 | 0x00,    // Set segment remapping: 0 (10.1.8)
        0xC8            // Set COM output scan direction: inverted (10.1.14)
    };
    
    I2C_Write(DISPLAY_I2C_ADDRESS, init3, sizeof(init3));
    
    const uint8_t init4[] =
    {
        0x00,           // Start command sequence
        0xDA, 0x12,     // Set pin configuration: left/right remap (10.1.18)
        0x81 | 0xA0     // Set contrast: 0 (10.1.7)
    };
    
    I2C_Write(DISPLAY_I2C_ADDRESS, init4, sizeof(init4));
    
    const uint8_t init5[] =
    {
        0x00,           // Start command sequence
        0xA4,           // Render w/ RAM contents (10.1.9)
        0xA6,           // Set pixel state: normal (10.1.10)
        0x2E,           // Deactivate scrolling (10.2.3)
        0xA5,           // Render full white (10.1.9)
        0xAF            // Display on
    };
    
    I2C_Write(DISPLAY_I2C_ADDRESS, init5, sizeof(init5));
}

void InitDisplay_ex(void)
{
    const uint8_t init1[] =
    {
        0x00,           // Start command sequence
        0xA8, 0x3F,     // Set multiplex ratio (10.1.11)
        0xD3, 0x00,
        0x40,
        0xA0,
        0xC8,
        0xDA, 0x02,
        0x81, 0x7F,
        0xA4,
        0xA6,
        0xD5, 0x80,
        0x8D, 0x14,
        0xAF,
        0xA5
    };
    
    I2C_Write(DISPLAY_I2C_ADDRESS, init1, sizeof(init1));
}

void main(void)
{
    InitClock();
    InitPins();
    InitPWM(50);
    I2C_Host_Init();
    
    EnableInterrupts();
    
    InitDisplay_ex();
    
    __delay_ms(500);
    
    while (1)
    {
        GetButtonState();
        
        __delay_ms(10);
    }
}
