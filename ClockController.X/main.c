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
#include "button.h"
#include "oled.h"
#include "ui.h"
#include "time_zone.h"

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

uint8_t CRC(void* data, uint8_t size)
{
    uint8_t crc = 0;
    for (uint8_t i = 0; i < size; ++i) crc ^= ((uint8_t*)data)[i];
    
    return crc;
}

void UpdateNixieDrivers()
{
    static uint8_t lastCRC = 0;
    uint8_t crc = CRC(&gRtc, sizeof(gRtc));
    
    if (lastCRC == crc) return;
    lastCRC = crc;    
    
    uint8_t digit;
    
    digit = gRtc.hour10;   I2C_Write(0x01, &digit, sizeof(digit));
    digit = gRtc.hour01;   I2C_Write(0x02, &digit, sizeof(digit));
    digit = gRtc.minute10; I2C_Write(0x03, &digit, sizeof(digit));
    digit = gRtc.minute01; I2C_Write(0x04, &digit, sizeof(digit));
    digit = gRtc.second10; I2C_Write(0x05, &digit, sizeof(digit));
    digit = gRtc.second01; I2C_Write(0x06, &digit, sizeof(digit));

    digit = gRtc.date10;  I2C_Write(0x09, &digit, sizeof(digit));
    digit = gRtc.date01;  I2C_Write(0x0A, &digit, sizeof(digit));
    digit = gRtc.month10; I2C_Write(0x0B, &digit, sizeof(digit));
    digit = gRtc.month01; I2C_Write(0x0C, &digit, sizeof(digit));
    digit = gRtc.year10;  I2C_Write(0x0D, &digit, sizeof(digit));
    digit = gRtc.year01;  I2C_Write(0x0E, &digit, sizeof(digit));
}

void SynchronizeTime()
{
    struct DateTime rtcTime;
    ConvertRtcToDateTime(&gRtc, &rtcTime);

    if (!TimesAreClose(&gpsData.datetime, &rtcTime)) RTC_Set(&gpsData.datetime);
}

void CheckGPS()
{
    if (gpsData.updated)
    {
        if ('A' == gpsData.status)
        {
            GPS_ConvertToLocalTime(gTimeZoneOffset);
            SynchronizeTime();
        }
        
        gpsData.updated = 0;
    }
}

void HandleUserInteraction()
{
    UpdateButtons();
    
    if (ROTATION_CW == gButtonState.rotation)
        UI_HandleRotationCW();
    if (ROTATION_CCW == gButtonState.rotation)
        UI_HandleRotationCCW();
}

#define SKIP_PD

void main(void)
{
    InitClock();
    InitPins();
    
    I2C_Host_Init();
    SerialInit();
    EnableInterrupts();
    
    // Give other devices (*cough*OLED*cough*) time to finish power-up.
    __delay_ms(50);
    
    OLED_Init();

#ifndef SKIP_PD
    if (!AP33772Init()) while (1);
#endif
    
    InitButtons();
    
    InitTimer();
    InitAdc();
    InitPWM();
    InitBoostConverter();
    
    gpsData.updated = 0;
    
    uint8_t frameCounter = 0;
    while (1)
    {
        UpdateBoostConverter();
        HandleUserInteraction();
        
        if (frameCounter % 4 == 0)
        {
            RTC_Read();
            CheckGPS();

            UpdateNixieDrivers();
            UI_TickSpinner();
        }

        if (frameCounter % 64 == 0)
        {
            UI_Update();
        }
        
        ++frameCounter;
        __delay_ms(5);
    }
}
