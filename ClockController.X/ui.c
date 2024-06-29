#include "ui.h"
#include "oled.h"
#include "gps.h"
#include "rtc.h"
#include "adc.h"
#include "boost_control.h"
#include "time_zone.h"
#include "button.h"
#include "boost_control.h"
#include "timer.h"
#include "ap33772.h"

#include <xc.h>

#define DISPLAY_TIMEOUT 100
#define DISPLAY_STATE_ON 1
#define DISPLAY_STATE_OFF 0

static uint16_t gDisplayTimer = DISPLAY_TIMEOUT;
static uint8_t gDisplayState = DISPLAY_STATE_ON;


#define STATE_PAGE_SCROLL 0

static uint8_t gState = STATE_PAGE_SCROLL;


#define PAGE_NONE  0
#define PAGE_STATUS 1
#define PAGE_TIME_ZONE 2
#define PAGE_BOOST 3
#define PAGE_USB_PD 4

#define PAGE_COUNT 4

static uint8_t gCurrentPage = PAGE_NONE;

#define xstr(s) str(s)
#define str(s) #s

void DrawPageTemplate(void)
{
    OLED_Clear();
    
    switch (gCurrentPage)
    {
        case PAGE_STATUS:
            OLED_DrawStringInverted(0, 0, xstr(PAGE_STATUS) "/" xstr(PAGE_COUNT) " STATUS           ");
            OLED_DrawString(1, 0, "20##-##-## ##:##:##");
            OLED_DrawString(2, 0, "GPS:");
            OLED_DrawString(3, 0, "###V @ ##%");
            break;
            
        case PAGE_TIME_ZONE:
            OLED_DrawStringInverted(0, 0, xstr(PAGE_TIME_ZONE) "/" xstr(PAGE_COUNT) " Time Zone & DST  ");
            OLED_DrawString(1, 0, "20##-##-## ##:##:##");
            OLED_DrawString(2, 0, "TZ: UTC");
            OLED_DrawString(3, 0, "DST: ");
            break;
            
        case PAGE_BOOST:
            OLED_DrawStringInverted(0, 0, xstr(PAGE_BOOST) "/" xstr(PAGE_COUNT) " Boost Converter  ");
            OLED_DrawString(1, 0, "Output: ### V");
            OLED_DrawString(2, 0, "PWM: ####/#### (##%)");
            OLED_DrawNumber16(2, 10, (TMR2_RESET << 2), 4);
            OLED_DrawString(3, 0, "ADC: #### mV");
            break;            
            
        case PAGE_USB_PD:
            OLED_DrawStringInverted(0, 0, xstr(PAGE_USB_PD) "/" xstr(PAGE_COUNT) " USB PD           ");
            OLED_DrawString(1, 0, "PDO #: ## A @ ## V");
            OLED_DrawString(2, 0, "#### mA @ ##### mV");
            break;
    }
}

void UI_TickSpinner()
{
    static const char SPINNER[] = { '/', '-', '\\', '|' };
    static uint8_t spinnerState = 0;
    
    OLED_DrawCharacterInverted(0, 20, SPINNER[spinnerState]);
    
    spinnerState = (spinnerState + 1) % sizeof(SPINNER);
}

void KeepDisplayAlive(void)
{
    if (DISPLAY_STATE_OFF == gDisplayState)
    {
        OLED_On();
        gDisplayState = DISPLAY_STATE_ON;
    }
    
    gDisplayTimer = DISPLAY_TIMEOUT;
}

void UI_HandleRotationCW(void)
{
    gCurrentPage = gCurrentPage < PAGE_COUNT ? gCurrentPage + 1 : 1;
    DrawPageTemplate();    
    UI_Update();
    
    KeepDisplayAlive();
    gButtonState.deltaR -= 2;
}

void UI_HandleRotationCCW(void)
{
    gCurrentPage = gCurrentPage > 1 ? gCurrentPage - 1 : PAGE_COUNT;
    DrawPageTemplate();
    UI_Update();
    
    KeepDisplayAlive();
    gButtonState.deltaR += 2;
}

void UI_HandleButtonPress(void)
{
    KeepDisplayAlive();
}

void DrawStatusPage(void)
{
    struct DateTime rtcTime;
    ConvertRtcToDateTime(&gRtc, &rtcTime);
    
    // Date
    OLED_DrawNumber8(1, 2, rtcTime.year, 2);
    OLED_DrawNumber8(1, 5, rtcTime.month, 2);
    OLED_DrawNumber8(1, 8, rtcTime.day, 2);
    
    // Time
    OLED_DrawNumber8(1, 11, rtcTime.hour, 2);
    OLED_DrawNumber8(1, 14, rtcTime.minute, 2);
    OLED_DrawNumber8(1, 17, rtcTime.second, 2);
    
    // GPS
    OLED_DrawString(2, 5, 
            'A' == gGpsData.status ? "OK " : 
              0 == gGpsData.status ? "?  " : "Acq");

    // "PD: ###V @ ##%"
    OLED_DrawNumber8(3, 0, gVoltage, 3);
    OLED_DrawNumber16(3, 7, BoostConverter_GetDutyCyclePct(), 2);
}

void DrawTimeZonePage(void)
{
    struct DateTime rtcTime;
    ConvertRtcToDateTime(&gRtc, &rtcTime);

    // Date
    OLED_DrawNumber8(1, 2, rtcTime.year, 2);
    OLED_DrawNumber8(1, 5, rtcTime.month, 2);
    OLED_DrawNumber8(1, 8, rtcTime.day, 2);
    
    // Time
    OLED_DrawNumber8(1, 11, rtcTime.hour, 2);
    OLED_DrawNumber8(1, 14, rtcTime.minute, 2);
    OLED_DrawNumber8(1, 17, rtcTime.second, 2);
    
    // Time Zone
    OLED_DrawCharacter(2, 7, gTimeZoneOffset < 0 ? '-' : '+');
    OLED_DrawNumber8(2, 8, (uint8_t)(gTimeZoneOffset < 0 ? -gTimeZoneOffset : gTimeZoneOffset), 2);
    OLED_DrawString(2, 11, TIME_ZONE_ABRV[gTimeZoneOffset + 12][TZ_LIST]);
}

void DrawBoostPage(void)
{
    OLED_DrawNumber8(1, 8, gVoltage, 3);
    OLED_DrawNumber16(2, 5, BoostConverter_GetDutyCycle(), 4);
    OLED_DrawNumber16(2, 16, BoostConverter_GetDutyCyclePct(), 2);
    OLED_DrawNumber16(3, 5, gAdcCv * 4, 4);
}

void DrawUsbPdPage(void)
{
    struct AP33772_Status status;
    AP33772_GetStatus(&status);
    
    OLED_DrawNumber8(1, 4, status.selectedPdoPos, 1);
    OLED_DrawNumber8(1, 7, status.pdoMaxAmps, 2);
    OLED_DrawNumber8(1, 14, status.pdoMaxVolts, 2);
    
    OLED_DrawNumber16(2, 0, status.current, 4);
    OLED_DrawNumber16(2, 10, status.voltage, 5);
}

typedef void PageDrawingFunction();

void UI_Update(void)
{
    static PageDrawingFunction* PAGE_DRAWING_FUNC[] =
    {
        &DrawStatusPage,
        &DrawTimeZonePage,
        &DrawBoostPage,
        &DrawUsbPdPage,
    };
    
    if (gDisplayTimer == 0)
    {
        OLED_Off();
        gDisplayState = DISPLAY_STATE_OFF;
    }
    else
    {
        --gDisplayTimer;
    }
    
    if (DISPLAY_STATE_OFF == gDisplayState) return;
    
    // First pass initialization
    if (PAGE_NONE == gCurrentPage)
    {
        gCurrentPage = PAGE_STATUS;
        DrawPageTemplate();
    }

    PAGE_DRAWING_FUNC[gCurrentPage - 1]();
}

