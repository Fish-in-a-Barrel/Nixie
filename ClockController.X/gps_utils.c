#include "gps_utils.h"
#include "bcd_utils.h"
#include "time_utils.h"

#include "gps.h"

void RollYearBack()
{
    --gGpsData.datetime.year;
}

void RollMonthBack()
{
    if (--gGpsData.datetime.month == 0)
    {
        gGpsData.datetime.month = 12;
        RollYearBack();
    }
}

void RollDayBack()
{
    if (--gGpsData.datetime.day == 0)
    {
        RollMonthBack();
        
        switch (gGpsData.datetime.month)
        {
            // 31 day months
            case 1: // January
            case 3: // March
            case 5: // May
            case 7: // July
            case 8: // August
            case 10: // October
            case 12: // December
                gGpsData.datetime.day = 31;
                break;
                
            // 30 day months
            case 4: // April
            case 6: // June
            case 9: // September
            case 11: // November
                gGpsData.datetime.day = 30;
                break;
                
            // February - I'm not bothering with anything more than standard leap days
            case 2:
            {
                gGpsData.datetime.day = (gGpsData.datetime.year % 4) ? 29 : 28;
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
    
    int8_t hour = (int8_t)gGpsData.datetime.hour;
    hour += tzOffset;
    
    // If hour < 0, then UTC is already at tomorrow and we need to roll the date back one day.
    if (hour < 0)
    {
        hour += 24;
        RollDayBack();
    }
    
    gGpsData.datetime.hour = (uint8_t)hour;
}

uint8_t GetDstOffset()
{
    // DST starts on the second Sunday in March
    struct DateTime dstStart = { gGpsData.datetime.year, 3, 1, 2, 0, 0};
    FindDayOfWeekN(&dstStart, DOW_SUNDAY, 2);
    
    if (DateTimeBefore(&gGpsData.datetime, &dstStart)) return 0;

    // DST ends on the first Sunday in November
    // Times are in standard time, so the end time is 1AM instead of 2AM.
    struct DateTime dstEnd = { gGpsData.datetime.year, 11, 1, 1, 0, 0};
    FindDayOfWeekN(&dstEnd, DOW_SUNDAY, 1);

    if (DateTimeAfter(&gGpsData.datetime, &dstEnd)) return 0;

    return 1;
}

// UTC to local time conversion
void GPS_ConvertToLocalTime(int8_t tzOffset)
{
    tzOffset += gGpsData.datetime.dst = GetDstOffset();
    RollTimeBack(tzOffset);
}

