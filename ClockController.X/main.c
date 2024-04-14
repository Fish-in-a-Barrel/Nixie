#include <xc.h>

#include "config_bits.h"
#include "clock.h"
#include "pins.h"
#include "i2c.h"
#include "serial.h"
#include "rtc.h"
#include "gps.h"
#include "bcd_utils.h"
#include "ap33772.h"
#include "timer.h"
#include "adc.h"
#include "pwm.h"
#include "boost_control.h"

void __interrupt() ISR()
{
    // Dispatch interrupts to handlers (§12.9.6)
    if (PIR1bits.SSP1IF || PIR1bits.BCL1IF) I2C_HandleInterrupt();
    if (PIR1bits.RC1IF) GPS_HandleInterrupt();
    if (PIR1bits.TMR2IF) TimerInterruptHandler();
    if (PIR1bits.ADIF) AdcInterruptHandler();
}

void EnableInterrupts()
{
    // Enable global and peripheral interrupts (§12.4)
    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1; 

    // Enable MSSP Interrupts (§12.9.3)
    PIE1bits.SSP1IE = 1;
    PIE1bits.BCL1IE = 1;
    
    // Enable EUSART interrupts (§12.9.3)
    PIE1bits.RC1IE = 1;
    
    // Enable the TMR2 interrupt for tick counting
    PIE1bits.TMR2IE = 1;

    // Enable ACD interrupt
    PIR1bits.ADIF = 0;
    PIE1bits.ADIE = 1;
}

struct RtcData rtc;

uint8_t CRC(void* data, uint8_t size)
{
    uint8_t crc = 0;
    for (uint8_t i = 0; i < size; ++i) crc ^= ((uint8_t*)data)[i];
    
    return crc;
}

void UpdateNixieDrivers()
{
    static uint8_t lastCRC = 0;
    uint8_t crc = CRC(&rtc, sizeof(rtc));
    
    if (lastCRC == crc) return;
    lastCRC = crc;    
    
    uint8_t digit;
    
    digit = rtc.hour10;   I2C_Write(0x01, &digit, sizeof(digit));
    digit = rtc.hour01;   I2C_Write(0x02, &digit, sizeof(digit));
    digit = rtc.minute10; I2C_Write(0x03, &digit, sizeof(digit));
    digit = rtc.minute01; I2C_Write(0x04, &digit, sizeof(digit));
    digit = rtc.second10; I2C_Write(0x05, &digit, sizeof(digit));
    digit = rtc.second01; I2C_Write(0x06, &digit, sizeof(digit));

    digit = rtc.date10;  I2C_Write(0x09, &digit, sizeof(digit));
    digit = rtc.date01;  I2C_Write(0x0A, &digit, sizeof(digit));
    digit = rtc.month10; I2C_Write(0x0B, &digit, sizeof(digit));
    digit = rtc.month01; I2C_Write(0x0C, &digit, sizeof(digit));
    digit = rtc.year10;  I2C_Write(0x0D, &digit, sizeof(digit));
    digit = rtc.year01;  I2C_Write(0x0E, &digit, sizeof(digit));
}

void ReadRTC()
{
    uint8_t READ_START_ADDRESS = 0x00;

    I2C_WriteRead(I2C_RTC_ADDRESS, &READ_START_ADDRESS, sizeof(READ_START_ADDRESS), &rtc, sizeof(rtc));
}

void SetRTC()
{
    // Byte 0 is the starting register address for the write.
    uint8_t buffer[sizeof(rtc) + 1] = { 0 };
    ConvertDateTimeToRtc((struct RtcData*)(buffer + 1), &gpsData.datetime, HOUR_TYPE_24);
    
    I2C_Write(I2C_RTC_ADDRESS, buffer, sizeof(buffer));
}

void SynchronizeTime()
{
    struct DateTime rtcTime;
    ConvertRtcToDateTime(&rtc, &rtcTime);

    if (!TimesAreClose(&gpsData.datetime, &rtcTime)) SetRTC();
}

void CheckGPS()
{
    if (gpsData.updated)
    {
        if ('A' == gpsData.status)
        {
            GPS_ConvertToLocalTime(-6);
            SynchronizeTime();
        }
        
        gpsData.updated = 0;
    }
}

void main(void)
{
    InitClock();
    InitPins();
    
    I2C_Host_Init();
    SerialInit();
    EnableInterrupts();

    AP33772Init();
    
    // TODO: Initialize display
    
    InitTimer();
    InitAdc();
    InitPWM();
    InitBoostConverter();
    
    gpsData.updated = 0;
    
    uint8_t frameCounter = 0;
    while (1)
    {
        UpdateBoostConverter();
        
        if (frameCounter % 16 == 0)
        {
            ReadRTC();
            CheckGPS();

            //UpdateNixieDrivers();

            // TODO: handle button changes
        }

        __delay_ms(5);
    }
}
