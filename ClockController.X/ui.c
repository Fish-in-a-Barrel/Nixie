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
#include "nixie.h"

#include <xc.h>

#define DISPLAY_TIMEOUT 100
#define DISPLAY_STATE_ON 1
#define DISPLAY_STATE_OFF 0

static uint16_t gDisplayTimer = DISPLAY_TIMEOUT;
static uint8_t gDisplayState = DISPLAY_STATE_ON;


#define STATE_PAGE_SCROLL 0
#define STATE_FIELD_SELECT 1
#define STATE_VALUE_SCROLL 2

static uint8_t gState = STATE_PAGE_SCROLL;

#define FIELD_TIME_ZONE 0
#define FIELD_DST 1

static uint8_t gField = FIELD_TIME_ZONE;

#define PAGE_NONE  0
#define PAGE_STATUS 1
#define PAGE_TIME_ZONE 2
#define PAGE_BOOST 3
#define PAGE_USB_PD 4
#define PAGE_NIXIE_STATUS 5

#define PAGE_COUNT 5

static uint8_t gCurrentPage = PAGE_NONE;

#define xstr(s) str(s)
#define str(s) #s

static uint8_t gFirstClick = 1;

void DrawPageTemplate(void)
{
    OLED_Clear();
    
    switch (gCurrentPage)
    {
        case PAGE_STATUS:
            OLED_DrawString(0, 0, xstr(PAGE_STATUS) "/" xstr(PAGE_COUNT) " STATUS           ", 1);
            OLED_DrawString(1, 0, "20##-##-## ##:##:##", 0);
            OLED_DrawString(2, 0, "GPS:", 0);
            OLED_DrawString(3, 0, "###V @ ##%", 0);
            break;
            
        case PAGE_TIME_ZONE:
            OLED_DrawString(0, 0, xstr(PAGE_TIME_ZONE) "/" xstr(PAGE_COUNT) " Time Zone & DST  ", 1);
            OLED_DrawString(1, 0, "20##-##-## ##:##:##", 0);
            OLED_DrawString(2, 0, "  TZ: UTC", 0);
            OLED_DrawString(3, 0, "  DST: ", 0);
            break;
            
        case PAGE_BOOST:
            OLED_DrawString(0, 0, xstr(PAGE_BOOST) "/" xstr(PAGE_COUNT) " Boost Converter  ", 1);
            OLED_DrawString(1, 0, "Output: ### V", 0);
            OLED_DrawString(2, 0, "PWM: ####/#### (##%)", 0);
            OLED_DrawNumber16(2, 10, (TMR2_RESET << 2), 4);
            OLED_DrawString(3, 0, "ADC: #### mV", 0);
            break;            
            
        case PAGE_USB_PD:
            OLED_DrawString(0, 0, xstr(PAGE_USB_PD) "/" xstr(PAGE_COUNT) " USB PD           ", 1);
            OLED_DrawString(1, 0, "PDO #: ## A @ ## V", 0);
            OLED_DrawString(2, 0, "#### mA @ ##### mV", 0);
            break;
            
        case PAGE_NIXIE_STATUS:
            OLED_DrawString(0, 0, xstr(PAGE_NIXIE_STATUS) "/" xstr(PAGE_COUNT) " Nixie Tubes      ", 1);
            OLED_DrawString(1, 0, "?? : ?? : ??", 0);
            OLED_DrawString(2, 0, "?? : ?? : ??", 0);
            break;            
    }
}

void UI_TickSpinner(void)
{
    static const char SPINNER[] = { '/', '-', '\\', '|' };
    static uint8_t spinnerState = 0;
    
    OLED_DrawCharacter(0, 20, SPINNER[spinnerState], 1);
    
    spinnerState = (spinnerState + 1) % sizeof(SPINNER);
}

/// Keeps the screen on in response to user interaction.
///
/// @return 1 if the action should be discarded before further processing because the screen was off.
uint8_t KeepDisplayAlive(void)
{
    uint8_t ignoreAction = 0;
    
    if (DISPLAY_STATE_OFF == gDisplayState)
    {
        OLED_On();
        gDisplayState = DISPLAY_STATE_ON;
        ignoreAction = 1;
    }
    
    gDisplayTimer = DISPLAY_TIMEOUT;
    
    return ignoreAction;
}

void UI_HandleRotationCW(void)
{
    gButtonState.deltaR -= 2;
    if (KeepDisplayAlive()) return;

    switch (gState)
    {
        case STATE_PAGE_SCROLL:
            gCurrentPage = gCurrentPage < PAGE_COUNT ? gCurrentPage + 1 : 1;
            DrawPageTemplate();
            break;
            
        case STATE_FIELD_SELECT:
            gField = !gField;
            break;
            
        case STATE_VALUE_SCROLL:
            // Time zone range is [-12, +14]]
            if (0 == gField) 
            {
                gTimeZoneOffset = gTimeZoneOffset < 14 ? gTimeZoneOffset + 1 : -12;
            }                        
            else
            {
                gDstType = !gDstType;
            }
            
            TimeZone_Save();
            break;
    }
    
    UI_Update();
}

void UI_HandleRotationCCW(void)
{
    gButtonState.deltaR += 2;
    if (KeepDisplayAlive()) return;

    switch (gState)
    {
        case STATE_PAGE_SCROLL:
            gCurrentPage = gCurrentPage > 1 ? gCurrentPage - 1 : PAGE_COUNT;
            DrawPageTemplate();
            break;
            
        case STATE_FIELD_SELECT:
            gField = !gField;
            break;
            
        case STATE_VALUE_SCROLL:
            // Time zone range is [-12, +14]]
            if (0 == gField)
            {
                gTimeZoneOffset = gTimeZoneOffset > -12 ? gTimeZoneOffset - 1 : 14;
            }
            else
            {
                gDstType = !gDstType;
            }
            
            TimeZone_Save();
            break;
    }
}

void UI_HandleButtonPress(void)
{
    gButtonState.c.edge = 0;
    
    // The button is "clicked" by power coming up. Ignore this phantom click.
    if (gFirstClick)
    {
        gFirstClick = 0;
        return;
    }
    
    if (KeepDisplayAlive()) return;
    
    if (gCurrentPage == PAGE_TIME_ZONE)
    {
        switch (gState)
        {
            case STATE_PAGE_SCROLL:
                gState = STATE_FIELD_SELECT;
                break;
            case STATE_FIELD_SELECT:
                gState = STATE_VALUE_SCROLL;
                break;
            case STATE_VALUE_SCROLL:
                gState = STATE_PAGE_SCROLL;
                break;
        }
    }
    else
    {
        gDisplayTimer = 0;
        gDisplayState = DISPLAY_STATE_OFF;
        OLED_Off();
    }
    
    UI_Update();
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
    OLED_DrawString(
        2,
        5, 
        'A' == gGpsData.status ? "OK " : (0 == gGpsData.status ? "?  " : "Acq"),
        0);

    // "PD: ###V @ ##%"
    OLED_DrawNumber8(3, 0, gVoltage, 3);
    OLED_DrawNumber16(3, 7, BoostConverter_GetDutyCyclePct(), 2);

    if (AdcOverVoltageProtectionTripped())
    {
        OLED_DrawString(3, 16, " ! OVP ! ", 1);
    }
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
    OLED_DrawCharacter(2, 7, gTimeZoneOffset < 0 ? '-' : '+', 0);
    OLED_DrawNumber8(2, 8, (uint8_t)(gTimeZoneOffset < 0 ? -gTimeZoneOffset : gTimeZoneOffset), 2);
    OLED_DrawString(2, 13, "  ", 0); // Erase any straggling letters from previous TZ
    OLED_DrawString(2, 11, TIME_ZONE_ABRV[gTimeZoneOffset + 12][gGpsData.datetime.dst], 0);
    
    // DST
    OLED_DrawString(3, 7, DST_TYPE_ABRV[gDstType], 0);
    
    // DST indicator
    if (DST_TYPE_OFF !=  gDstType)
    {
        if ('A' == gGpsData.status)
        {
            OLED_DrawString(3, 18, gGpsData.datetime.dst ? " ON" : "OFF", 0);
        }
        else
        {
            OLED_DrawString(3, 18, "???", 0);
        }
    }
    else
    {
        OLED_DrawString(3, 18, "   ", 0);
    }
    
    char* indicators[2] = { "  ", "  " }; 
    if (STATE_FIELD_SELECT == gState) indicators[gField] = "\x10 ";
    if (STATE_VALUE_SCROLL == gState) indicators[gField] = "\x1E\x1F";
    
    OLED_DrawString(2, 0, indicators[0], 0);
    OLED_DrawString(3, 0, indicators[1], 0);
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

void DrawNixieStatusPage(void)
{
    OLED_DrawCharacter(1,  0, ((gNixieStatus >> 0x1) & 1) ? '\x03' : '!', 0);
    OLED_DrawCharacter(1,  1, ((gNixieStatus >> 0x2) & 1) ? '\x03' : '!', 0);
    OLED_DrawCharacter(1,  5, ((gNixieStatus >> 0x3) & 1) ? '\x03' : '!', 0);
    OLED_DrawCharacter(1,  6, ((gNixieStatus >> 0x4) & 1) ? '\x03' : '!', 0);
    OLED_DrawCharacter(1, 10, ((gNixieStatus >> 0x5) & 1) ? '\x03' : '!', 0);
    OLED_DrawCharacter(1, 11, ((gNixieStatus >> 0x6) & 1) ? '\x03' : '!', 0);
    
    OLED_DrawCharacter(2,  0, ((gNixieStatus >> 0xB) & 1) ? '\x03' : '!', 0);
    OLED_DrawCharacter(2,  1, ((gNixieStatus >> 0xC) & 1) ? '\x03' : '!', 0);
    OLED_DrawCharacter(2,  5, ((gNixieStatus >> 0x9) & 1) ? '\x03' : '!', 0);
    OLED_DrawCharacter(2,  6, ((gNixieStatus >> 0xA) & 1) ? '\x03' : '!', 0);
    OLED_DrawCharacter(2, 10, ((gNixieStatus >> 0xD) & 1) ? '\x03' : '!', 0);
    OLED_DrawCharacter(2, 11, ((gNixieStatus >> 0xE) & 1) ? '\x03' : '!', 0);
}

typedef void PageDrawingFunction(void);

void UI_Update(void)
{
    static PageDrawingFunction* PAGE_DRAWING_FUNC[] =
    {
        &DrawStatusPage,
        &DrawTimeZonePage,
        &DrawBoostPage,
        &DrawUsbPdPage,
        &DrawNixieStatusPage,
    };
    
    if (gDisplayTimer == 0)
    {
        //OLED_Off();
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

