#include "ui.h"
#include "oled.h"
#include "gps.h"
#include "rtc.h"
#include "adc.h"
#include "boost_control.h"

#include <xc.h>

#define STATE_PAGE_SCROLL 0

#define PAGE_NONE  0xFF
#define PAGE_STATUS 0
#define PAGE_COUNT 1

static uint8_t gState = STATE_PAGE_SCROLL;
static uint8_t gCurrentPage = PAGE_NONE;

void DrawPageTemplate(void)
{
    OLED_Clear();
    
    switch (gCurrentPage)
    {
        case PAGE_STATUS:
            OLED_DrawStringInverted(0, 0, "1/1 - STATUS        ");
            DrawString(1, 0, "20##-##-##");
            DrawString(2, 0, "##:##:##");
            DrawString(3, 0, "###V @ ##% / GPS:");
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

void UI_HandleRotationCW(void)
{
    gCurrentPage = (gCurrentPage + 1) % PAGE_COUNT;
    DrawPageTemplate();
}

void UI_HandleRotationCCW(void)
{
    gCurrentPage = gCurrentPage > 0 ? gCurrentPage - 1 : PAGE_COUNT - 1;
    DrawPageTemplate();
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
    OLED_DrawNumber8(2, 0, rtcTime.hour, 2);
    OLED_DrawNumber8(2, 3, rtcTime.minute, 2);
    OLED_DrawNumber8(2, 6, rtcTime.second, 2);

    // "PD: ###V @ ##%"
    OLED_DrawNumber8(3, 0, gVoltage, 3);
    OLED_DrawNumber16(3, 7, BoostConverter_GetDutyCyclePct(), 2);
    
    // GPS
    DrawString(3, 17, 'A' == gpsData.status ? "OK" : "Acq");
}

void UI_Update(void)
{
    if (PAGE_NONE == gCurrentPage) UI_HandleRotationCW();

    switch (gCurrentPage)
    {
        case PAGE_STATUS: DrawStatusPage(); break;
    }
}

