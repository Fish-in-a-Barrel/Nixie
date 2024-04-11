#include "gps_utils.h"
#include "bcd_utils.h"
#include "time_utils.h"

#include "gps.h"

// Updates the passed date with the date of the requested Sunday in the month and year passed
// whichSunday: the 1-based count of the Sunday requested
void FindSunday(char* date, uint8_t whichSunday)
{
    uint8_t sunday = (whichSunday - 1) * 7 + 1;
    sunday += ((7 - GetDayOfWeek(date) + DOW_SUNDAY) % 7);
    BinaryToBcd(sunday, date, 2);
}

void RollYearBack()
{
    uint8_t year = BcdToBinary(gpsData.date + 4, 2) - 1;
    BinaryToBcd(year, gpsData.date + 4, 2);
}

uint8_t RollMonthBack()
{
    uint8_t month = BcdToBinary(gpsData.date + 2, 2);
    
    if (--month == 0)
    {
        month = 12;
        RollYearBack();
    }
    
    BinaryToBcd(month, gpsData.date + 2, 2);
    return month;
}

void RollDayBack()
{
    uint8_t day = BcdToBinary(gpsData.date, 2);
    
    if (--day == 0)
    {
        uint8_t month = RollMonthBack();
        
        switch (month)
        {
            // 31 day months
            case 1: // January
            case 3: // March
            case 5: // May
            case 7: // July
            case 8: // August
            case 10: // October
            case 12: // December
                day = 31;
                break;
                
            // 30 day months
            case 4: // April
            case 6: // June
            case 9: // September
            case 11: // November
                day = 30;
                break;
                
            // February - I'm not bothering with anything more than standard leap days
            case 2:
            {
                uint8_t year = BcdToBinary(gpsData.date + 4, 2);
                day = (year % 4) ? 29 : 28;
                break;
            }

            default:
                // LOL what?!?
                break;
        }
    }
    
    BinaryToBcd(day, gpsData.date, 2);
}

void RollTimeBack(int8_t tzOffset)
{
    int8_t hour = (int8_t)BcdToBinary(gpsData.time, 2);
    hour -= tzOffset;
    
    // If hour < 0, then UTC is already at tomorrow and we need to roll the date back one day.
    if (hour < 0)
    {
        hour += 24;
        RollDayBack();
    }
    
    BinaryToBcd((uint8_t)hour, gpsData.time, 2);
}

void RollMonthForward()
{
    uint8_t month = BcdToBinary(gpsData.date + 2, 2);

    // This code is only triggered by DST=true, which means we don't have to worry about rolling over the year.
    BinaryToBcd(++month, gpsData.date + 2, 2);
}

void RollDayForward()
{
    uint8_t day = BcdToBinary(gpsData.date, 2);
    uint8_t month = BcdToBinary(gpsData.date + 2, 2);
    
    // This code is only triggered by DST=true, which means we only need to consider March->November
    uint8_t dayMax = 31;
    switch (month)
    {
        // 30 day months
        case 4: // April
        case 6: // June
        case 9: // September
        case 11: // November
            day = 30;
            break;
    }
    
    if (++day > dayMax)
    {
        day = 1;
        RollMonthForward();        
    }
    
    BinaryToBcd(day, gpsData.date, 2);
}

void UpdateDST()
{
    // DST starts on the second Sunday in March
    char dstStart[6] = "dd03yy";
    dstStart[4] = gpsData.date[4];
    dstStart[5] = gpsData.date[5];
    FindSunday(dstStart, 2);

    // DST ends on the first Sunday in November
    char dstEnd[6] = "dd11yy";
    dstEnd[4] = gpsData.date[4];
    dstEnd[5] = gpsData.date[5];
    FindSunday(dstEnd, 1);
    
    // Times are in standard time, so the end time is 1AM instead of 2AM.
    gpsData.dst = 
        DateTimeAfter(gpsData.date, gpsData.time, dstStart, "020000") &&
        DateTimeBefore(gpsData.date, gpsData.time, dstEnd, "010000");

    if (gpsData.dst && (BcdToBinary(gpsData.time, 2) + 1 >= 24))
    {
        RollDayForward();
    
        BinaryToBcd(0, gpsData.time, 2);
    }
}

// This is a very crude UTC to local time conversion
void GPS_ConvertToLocalTime(int8_t tzOffset)
{
    RollTimeBack(tzOffset);
    UpdateDST();
}

