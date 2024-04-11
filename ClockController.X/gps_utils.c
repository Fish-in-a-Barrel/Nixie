#include "gps_utils.h"
#include "bcd_utils.h"
#include "time_utils.h"

#include "gps.h"

// Updates the passed date with the date of the requested Sunday in the month and year passed
// whichSunday: the 1-based count of the Sunday requested
void FindSunday(struct DateTime* date, uint8_t whichSunday)
{
    uint8_t sunday = (whichSunday - 1) * 7 + 1;
    sunday += ((7 - GetDayOfWeek(date) + DOW_SUNDAY) % 7);
    
    date->day = sunday;
}

void RollYearBack()
{
    --gpsData.datetime.year;
}

void RollMonthBack()
{
    if (--gpsData.datetime.month == 0)
    {
        gpsData.datetime.month = 12;
        RollYearBack();
    }
}

void RollDayBack()
{
    if (--gpsData.datetime.day == 0)
    {
        RollMonthBack();
        
        switch (gpsData.datetime.month)
        {
            // 31 day months
            case 1: // January
            case 3: // March
            case 5: // May
            case 7: // July
            case 8: // August
            case 10: // October
            case 12: // December
                gpsData.datetime.day = 31;
                break;
                
            // 30 day months
            case 4: // April
            case 6: // June
            case 9: // September
            case 11: // November
                gpsData.datetime.day = 30;
                break;
                
            // February - I'm not bothering with anything more than standard leap days
            case 2:
            {
                gpsData.datetime.day = (gpsData.datetime.year % 4) ? 29 : 28;
                break;
            }

            default:
                // LOL what?!?
                break;
        }
    }
}

void RollTimeBack(int8_t tzOffset)
{
    int8_t hour = (int8_t)gpsData.datetime.hour;
    hour -= tzOffset;
    
    // If hour < 0, then UTC is already at tomorrow and we need to roll the date back one day.
    if (hour < 0)
    {
        hour += 24;
        RollDayBack();
    }
    
    gpsData.datetime.hour = (uint8_t)hour;
}

void RollMonthForward()
{
    // This code is only triggered by DST=true, which means we don't have to worry about rolling over the year because that only happens in
    // November.
    ++gpsData.datetime.month;
}

void RollDayForward()
{
    // This code is only triggered by DST=true, which means we only need to consider March->November
    uint8_t dayMax = 31;
    switch (gpsData.datetime.month)
    {
        // 30 day months
        case 4: // April
        case 6: // June
        case 9: // September
        case 11: // November
            dayMax = 30;
            break;
    }
    
    if (++gpsData.datetime.day > dayMax)
    {
        gpsData.datetime.day = 1;
        RollMonthForward();        
    }
}

void UpdateDST()
{
    // DST starts on the second Sunday in March
    struct DateTime dstStart = { gpsData.datetime.year, 3, 1, 2, 0, 0};
    FindSunday(&dstStart, 2);
    
    if (DateTimeBefore(&gpsData.datetime, &dstStart)) return;

    // DST ends on the first Sunday in November
    // Times are in standard time, so the end time is 1AM instead of 2AM.
    struct DateTime dstEnd = { gpsData.datetime.year, 11, 1, 1, 0, 0};
    FindSunday(&dstEnd, 1);

    if (DateTimeBefore(&gpsData.datetime, &dstEnd)) return;

    // If we've made it this far, then we're in DST
    if (++gpsData.datetime.hour >= 24) RollDayForward();
}

// This is a very crude UTC to local time conversion
void GPS_ConvertToLocalTime(int8_t tzOffset)
{
    RollTimeBack(tzOffset);
    UpdateDST();
}

