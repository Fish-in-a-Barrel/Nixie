#include <xc.h>

#include "config_bits.h"
#include "clock.h"
#include "i2c.h"
#include "rtc.h"

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

uint8_t rtcBuffer[6] = { 0, 0, 0, 0, 0, 0 };
struct RtcData rtc;

void UpdateNixieDrivers()
{
    uint8_t digit = rtc.seconds01;
    
    I2C_Write(0x01, &digit, sizeof(digit));
}

void ReadRTC()
{
    uint8_t READ_START_ADDRESS = 0x00;

    I2C_WriteRead(I2C_RTC_ADDRESS, &READ_START_ADDRESS, sizeof(READ_START_ADDRESS), &rtc, sizeof(rtc));
}

void SetClock()
{
    rtc.minutes01 = 5;
    rtc.minutes10 = 4;
    
    rtc.hour01 = 2;
    rtc.hours10 = 1;
    rtc.hoursType = 1;
    
    rtc.date01 = 2;
    rtc.date10 = 1;
    
    rtc.month01 = 1;
    rtc.month10 = 1;
    
    rtc.year01 = 3;
    rtc.year10 = 2;
    
    uint8_t buffer[sizeof(rtc) + 1];
    for (int i = 0; i < sizeof(rtc); ++i) buffer[i + 1] = ((uint8_t*)&rtc)[i];
    
    I2C_Write(I2C_RTC_ADDRESS, buffer, sizeof(buffer));
}

void main(void)
{
    I2C_Host_Init();
    EnableInterrupts();
    
    //SetClock();
    
    while (1)
    {
        ReadRTC();
        UpdateNixieDrivers();

        __delay_ms(1000);
    }
}
