#include <xc.h>

#include "config_bits.h"
#include "clock.h"
#include "pins.h"
#include "i2c.h"
#include "serial.h"
#include "rtc.h"
#include "gps.h"
#include "bcd_utils.h"

void __interrupt() ISR()
{
    // Dispatch interrupts to handlers (§12.9.6)
    if (PIR1bits.SSP1IF || PIR1bits.BCL1IF) I2C_HandleInterrupt();
    if (PIR1bits.RC1IF) GPS_HandleInterrupt();
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
    rtc.hour10 = gpsData.datetime.hour / 10;
    rtc.hour01 = gpsData.datetime.hour % 10;
    rtc.hoursType = 1;
    
    rtc.minute10 = gpsData.datetime.minute / 10;
    rtc.minute01 = gpsData.datetime.minute % 10;
    
    rtc.second10 = gpsData.datetime.second / 10;
    rtc.second01 = gpsData.datetime.second % 10;
    
    rtc.year10 = gpsData.datetime.year / 10;
    rtc.year01 = gpsData.datetime.year % 10;
    
    rtc.month10 = gpsData.datetime.month / 10;
    rtc.month01 = gpsData.datetime.month % 10;
    
    rtc.date10 = gpsData.datetime.date / 10;
    rtc.date01 = gpsData.datetime.date % 10;
    
    uint8_t buffer[sizeof(rtc) + 1];
    for (int i = 0; i < sizeof(rtc); ++i) buffer[i + 1] = ((uint8_t*)&rtc)[i];
    
    I2C_Write(I2C_RTC_ADDRESS, buffer, sizeof(buffer));
}

void CheckGPS()
{
    if (gpsData.updated)
    {
        if ('A' == gpsData.status)
        {
            GPS_ConvertToLocalTime(-6);

            // TODO: Compare to RTC and update RTC as needed
        }
        
        gpsData.updated = 0;
    }
}

void main(void)
{
    InitPins();
    I2C_Host_Init();
    SerialInit();
    EnableInterrupts();
    InitClock();
    
    // TODO: Initialize USB PD
    
    // TODO: Initialize display
    
    // TODO: Initialize ADC
    // TODO: Initialize PWM
    // TODO: Setup power control interrupt
    
    gpsData.updated = 0;
    
    while (1)
    {
        CheckGPS();
        ReadRTC();
        
        UpdateNixieDrivers();
        
        // TODO: handle button changes

        __delay_ms(100);
    }
}
