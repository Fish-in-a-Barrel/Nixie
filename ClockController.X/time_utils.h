#ifndef TIME_UTILS_H
#define	TIME_UTILS_H

#include <xc.h>

struct Datetime
{
    uint8_t year;
    uint8_t month;
    uint8_t date;
    
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

uint8_t GetDayOfWeek(volatile const struct Datetime* date);

// True if a > b
uint8_t DateAfter(volatile const struct Datetime* a, volatile const struct Datetime* b);

// True if a < b
uint8_t DateBefore(volatile const struct Datetime* a, volatile const struct Datetime* b);

// True if a > b
uint8_t TimeAfter(volatile const struct Datetime* a, volatile const struct Datetime* b);

// True if a < b
uint8_t TimeBefore(volatile const struct Datetime* a, volatile const struct Datetime* b);

// True if a > b
uint8_t DateTimeAfter(volatile const struct Datetime* a, volatile const struct Datetime* b);

// True if a < b
uint8_t DateTimeBefore(volatile const struct Datetime* a, volatile const struct Datetime* b);

#endif	/* TIME_UTILS_H */

