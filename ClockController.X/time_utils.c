#include "time_utils.h"
#include "bcd_utils.h"

const uint8_t DATE_ORDER[] = { 4, 5, 2, 3, 0, 1 };

int8_t DateCompare(volatile const struct DateTime* a, volatile const struct DateTime* b)
{
    if (a->year < b->year) return -1;
    if (a->year > b->year) return 1;
    if (a->month < b->month) return -1;
    if (a->month > b->month) return 1;
    if (a->day < b->day) return -1;
    if (a->day > b->day) return 1;
    return 0;
}

uint8_t DateAfter(volatile const struct DateTime* a, volatile const struct DateTime* b)
{
    return DateCompare(a, b) > 0;
}

uint8_t DateBefore(volatile const struct DateTime* a, volatile const struct DateTime* b)
{
    return DateCompare(a, b) < 0;
}

int8_t TimeCompare(volatile const struct DateTime* a, volatile const struct DateTime* b)
{
    if (a->hour < b->hour) return -1;
    if (a->hour > b->hour) return 1;
    if (a->minute < b->minute) return -1;
    if (a->minute > b->minute) return 1;
    if (a->minute < b->minute) return -1;
    if (a->minute > b->minute) return 1;
    return 0;
}

uint8_t TimeAfter(volatile const struct DateTime* a, volatile const struct DateTime* b)
{
    return TimeCompare(a, b) > 0;
}

uint8_t TimeBefore(volatile const struct DateTime* a, volatile const struct DateTime* b)
{
    return TimeCompare(a, b) < 0;
}

uint8_t DateTimeAfter(volatile const struct DateTime* a, volatile const struct DateTime* b)
{
    if (DateAfter(a, b)) return 1;
    if (DateBefore(a, b)) return 0;

    if (TimeAfter(a, b)) return 1;
    return 0;
}

uint8_t DateTimeBefore(volatile const struct DateTime* a, volatile const struct DateTime* b)
{
    if (DateBefore(a, b)) return 1;
    if (DateAfter(a, b)) return 0;
    
    if (TimeBefore(a, b)) return 1;
    return 0;
}

uint8_t GetDayOfWeek(volatile const struct DateTime* date)
{
    // This set of month keys is only valid in years [2000, 2099].
    const uint8_t MONTH_KEY[12] = { 2, 5, 5, 1, 3, 6, 1, 4, 7, 2, 5, 7 };
    
    uint8_t monthKey = 0;
    
    // Convert the raw month to it's equivalent "key" value
    if ((date->year % 4) || (date->month > 2))
    {
        // Non leap year, or after February, just use the look-up table.
        monthKey = MONTH_KEY[date->month - 1];
    }
    else
    {
        // During leap years, January and February are special.
        monthKey = (date->month == 1) ? 1 : 4;
    }
    
    int dayOfWeek = date->year / 10 + date->year % 10;  // Add the last two digits of the year
    dayOfWeek /= 4;                                     // Divide the sum by 4
    dayOfWeek += date->day;                            // Add the day of the month
    dayOfWeek += monthKey;                              // Add the month key
    dayOfWeek %= 7;                                     // Modulo 7 for the day of the week. 0 = Saturday.
    
    return (uint8_t)dayOfWeek;
}
