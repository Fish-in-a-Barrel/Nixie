#ifndef TIME_UTILS_H
#define	TIME_UTILS_H

#include <xc.h>

struct DateTime
{
    uint8_t year;
    uint8_t month;
    uint8_t day;
    
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

#define DOW_SATURDAY 0
#define DOW_SUNDAY 1
#define DOW_MONDAY 2
#define DOW_TUESDAY 3
#define DOW_WEDNESDAY 4
#define DOW_THURSDAY 5
#define DOW_FRIDAY 6

uint8_t GetDayOfWeek(volatile const struct DateTime* date);

// Updates the passed date with the date of the requested Sunday in the month and year passed.
// date: The structure to update. Year and month should be initialized before calling.
// dow: The day-of-week to find, 0 = Saturday.
// n: Which occurrence to find, 1-based. 
void FindDayOfWeekN(struct DateTime* date, uint8_t dow, uint8_t n);

// True if a > b
uint8_t DateAfter(volatile const struct DateTime* a, volatile const struct DateTime* b);

// True if a < b
uint8_t DateBefore(volatile const struct DateTime* a, volatile const struct DateTime* b);

// True if a > b
uint8_t TimeAfter(volatile const struct DateTime* a, volatile const struct DateTime* b);

// True if a < b
uint8_t TimeBefore(volatile const struct DateTime* a, volatile const struct DateTime* b);

// True if a > b
uint8_t DateTimeAfter(volatile const struct DateTime* a, volatile const struct DateTime* b);

// True if a < b
uint8_t DateTimeBefore(volatile const struct DateTime* a, volatile const struct DateTime* b);

// True if a and b are within a second of each other
uint8_t TimesAreClose(volatile const struct DateTime* a, volatile const struct DateTime* b);

#endif	/* TIME_UTILS_H */

