#include "time_utils.h"
#include "bcd_utils.h"

const uint8_t DATE_ORDER[] = { 4, 5, 2, 3, 0, 1 };

uint8_t DateAfter(volatile const char* a, volatile const char* b)
{
    for (uint8_t i = 0; i < 6; ++i)
    {
        uint8_t x = DATE_ORDER[i];
        if (a[x] > b[x]) return 1;
    }
    
    return 0;
}

uint8_t DateBefore(volatile const char* a, volatile const char* b)
{
    for (uint8_t i = 0; i < 6; ++i)
    {
        uint8_t x = DATE_ORDER[i];
        if (a[x] < b[x]) return 1;
    }
    
    return 0;
}

uint8_t TimeAfter(volatile const char* a, volatile const char* b)
{
    for (uint8_t i = 0; i < 6; ++i)
        if (a[i] > b[i]) return 1;
    
    return 0;
}

uint8_t TimeBefore(volatile const char* a, volatile const char* b)
{
    for (uint8_t i = 0; i < 6; ++i)
        if (a[i] < b[i]) return 1;
    
    return 0;
}

uint8_t DateTimeAfter(volatile const char* ad, volatile const char* at, volatile const char* bd, volatile const char* bt)
{
    if (DateAfter(ad, bd)) return 1;
    if (DateBefore(ad, bd)) return 0;

    if (TimeAfter(at, bt)) return 1;
    return 0;
}

uint8_t DateTimeBefore(volatile const char* ad, volatile const char* at, volatile const char* bd, volatile const char* bt)
{
    if (DateBefore(ad, bd)) return 1;
    if (DateAfter(ad, bd)) return 0;
    
    if (TimeBefore(at, bt)) return 1;
    return 0;
}

uint8_t GetDayOfWeek(volatile const char* date)
{
    // This set of month keys is only valid in years [2000, 2099].
    const int8_t MONTH_KEY[12] = { 2, 5, 5, 1, 3, 6, 1, 4, 7, 2, 5, 7 };
    
    int8_t day = BcdToBinary(date, 2);
    int8_t month = BcdToBinary(date + 2, 2);
    int8_t year = BcdToBinary(date + 4, 2);
    
    // Convert the raw month to it's equivalent "key" value
    if ((year % 4) || (month > 2))
    {
        // Non leap year, or after February, just use the look-up table.
        month = MONTH_KEY[month - 1];
    }
    else
    {
        // During leap years, January and February are special.
        month = (month == 1) ? 1 : 4;
    }
    
    int dayOfWeek = year / 10 + year % 10;  // Add the last two digits of the year
    dayOfWeek /= 4;                         // Divide the sum by 4
    dayOfWeek += day;                       // Add the day of the month
    dayOfWeek += month;                     // Add the month key
    dayOfWeek %= 7;                         // Modulo 7 for the day of the week. 0 = Saturday.
    
    return (uint8_t)dayOfWeek;
}
