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
#include "nixie.h"

void __interrupt() ISR()
{
    // Dispatch interrupts to handlers (§12.9.6)
    if (PIR1bits.SSP1IF || PIR1bits.BCL1IF) I2C_HandleInterrupt();
    if (PIR1bits.RC1IF) GPS_HandleInterrupt();
    if (PIR1bits.TMR2IF) TimerInterruptHandler();
    if (PIR1bits.ADIF) AdcInterruptHandler();
    if (PIR0bits.IOCIF) Buttons_HandleInterrupt();
}

void EnableInterrupts()
{
    // Enable global and peripheral interrupts (§12.4)
    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1;    
    
    // Enable IOC interrupts (§17.2)
    PIE0bits.IOCIE = 1;

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

void SynchronizeTime()
{
    struct DateTime rtcTime;
    ConvertRtcToDateTime(&gRtc, &rtcTime);

    if (!TimesAreClose(&gGpsData.datetime, &rtcTime)) RTC_Set(&gGpsData.datetime);
}

void CheckGPS()
{
    if (gGpsData.updated)
    {
        if ('A' == gGpsData.status)
        {
            GPS_ConvertToLocalTime(gTimeZoneOffset);
            SynchronizeTime();
        }
        
        gGpsData.updated = 0;
    }
}

void HandleUserInteraction()
{
    if (gButtonState.deltaR >= 2) UI_HandleRotationCW();
    if (gButtonState.deltaR <= -2) UI_HandleRotationCCW();
    
    if (gButtonState.c.state && gButtonState.c.edge) UI_HandleButtonPress();
    
    gButtonState.rotation = ROTATION_NONE;
}

void main(void)
{
    InitClock();
    InitPins();
    
    TimeZone_Load();
    
    I2C_Host_Init();
    SerialInit();
    EnableInterrupts();
    
    Buttons_Init();
    
    // Give other devices (*cough*OLED*cough*) time to finish power-up.
    __delay_ms(50);
    
    OLED_Init();

#ifndef SKIP_PD
    if (!AP33772_Init()) while (1);
#endif
    
    InitTimer();
    InitAdc();
    InitPWM();
    InitBoostConverter();
    
    // Power-on can be detected as a state change in buttons. That should have stabilized at this point, so reset the
    // edge states.
    Button_ResetEdges();
    
    gGpsData.updated = 0;
    
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
        }

        if (frameCounter % 64 == 0)
        {
            UI_TickSpinner();
            UI_Update();
        }
        
        ++frameCounter;
        __delay_ms(5);
    }
}
