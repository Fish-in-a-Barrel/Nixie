#ifndef TIME_UTILS_H
#define	TIME_UTILS_H

#include <xc.h>

#define DOW_SATURDAY 0
#define DOW_SUNDAY 1
#define DOW_MONDAY 2
#define DOW_TUESDAY 3
#define DOW_WEDNESDAY 4
#define DOW_THURSDAY 5
#define DOW_FRIDAY 6

//
// Date format = ddmmyy
//

// True if a > b
uint8_t DateAfter(volatile const char* a, volatile const char* b);

// True if a < b
uint8_t DateBefore(volatile const char* a, volatile const char* b);

uint8_t GetDayOfWeek(volatile const char* date);

//
// Time format = hhmmss
//

// True if a > b
uint8_t TimeAfter(volatile const char* a, volatile const char* b);

// True if a < b
uint8_t TimeBefore(volatile const char* a, volatile const char* b);


// True if a > b
uint8_t DateTimeAfter(volatile const char* ad, volatile const char* at, volatile const char* bd, volatile const char* bt);

// True if a < b
uint8_t DateTimeBefore(volatile const char* ad, volatile const char* at, volatile const char* bd, volatile const char* bt);

#endif	/* TIME_UTILS_H */

