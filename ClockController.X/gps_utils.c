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

// tzOffset must be negative
void RollTimeBack(int8_t tzOffset)
{
    if (tzOffset >= 0) return;
    
    int8_t hour = (int8_t)gpsData.datetime.hour;
    hour += tzOffset;
    
    // If hour < 0, then UTC is already at tomorrow and we need to roll the date back one day.
    if (hour < 0)
    {
        hour += 24;
        RollDayBack();
    }
    
    gpsData.datetime.hour = (uint8_t)hour;
}

uint8_t GetDstOffset()
{
    // DST starts on the second Sunday in March
    struct DateTime dstStart = { gpsData.datetime.year, 3, 1, 2, 0, 0};
    FindSunday(&dstStart, 2);
    
    if (DateTimeBefore(&gpsData.datetime, &dstStart)) return 0;

    // DST ends on the first Sunday in November
    // Times are in standard time, so the end time is 1AM instead of 2AM.
    struct DateTime dstEnd = { gpsData.datetime.year, 11, 1, 1, 0, 0};
    FindSunday(&dstEnd, 1);

    if (DateTimeBefore(&gpsData.datetime, &dstEnd)) return 0;

    return 1;
}

// UTC to local time conversion
void GPS_ConvertToLocalTime(int8_t tzOffset)
{
    tzOffset += GetDstOffset();
    RollTimeBack(tzOffset);
}

