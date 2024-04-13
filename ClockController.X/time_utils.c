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

// https://www.almanac.com/how-find-day-week
uint8_t GetDayOfWeek(volatile const struct DateTime* date)
{
    // This set of month keys is only valid in years [2000..2099].
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
    dayOfWeek += date->day;                             // Add the day of the month
    dayOfWeek += monthKey;                              // Add the month key
    dayOfWeek %= 7;                                     // Modulo 7 for the day of the week. 0 = Saturday.
    
    return (uint8_t)dayOfWeek;
}

// Updates the passed date with the date of the requested Sunday in the month and year passed
// whichSunday: the 1-based count of the Sunday requested
void FindSunday(struct DateTime* date, uint8_t whichSunday)
{
    // Start with the earliest possible day that could be the requested Sunday.
    date->day = (whichSunday - 1) * 7 + 1;
    
    // Move forward based on the actual day of the week.
    int8_t dow = (int8_t)GetDayOfWeek(date) - DOW_SUNDAY;
    if (dow < DOW_SUNDAY) date->day += DOW_SUNDAY - dow;
    else if (dow > DOW_SUNDAY) date->day += 7 - (dow + DOW_SUNDAY);
}

// Computes the absolute difference between to values, accounting for "closer" distances due to round over/under.
// e.g. AbsModDiff(0, 59, 60) has a distance of 1, not 59
uint8_t AbsModDiff(uint8_t a, uint8_t b, uint8_t modulus)
{
    uint8_t diff = 0;
    
    if (a < b) diff = b - a;
    else if (b < a) diff = a - b;
    
    if (diff * 2 > modulus) diff = modulus - diff;
    
    return diff;
}

uint8_t TimesAreClose(volatile const struct DateTime* a, volatile const struct DateTime* b)
{
    if (a->hour != b->hour)
    {
        if (AbsModDiff(a->hour, b->hour, 24) >= 1) return 0;

        if (a->hour < b->hour) return (a->minute == 59) && (a->second == 59) && (b->minute == 0) && (b->second == 0);
        else return (a->minute == 0) && (a->second == 0) && (b->minute == 59) && (b->second == 59);
    }
    else if (a->minute != b->minute)
    {
        if (AbsModDiff(a->minute, b->minute, 60) >= 1) return 0;
        
        if (a->minute < b->minute) return (a->second == 59) && (b->second == 0);
        else return (a->second == 0) && (b->second == 59);
    }
    else
    {
        return AbsModDiff(a->second, b->second, 60) <= 1;
    }
}
