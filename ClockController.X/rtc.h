#ifndef RTC_H
#define	RTC_H

#include <xc.h>

#include "time_utils.h"

#define I2C_RTC_ADDRESS 0x68

#define HOUR_TYPE_12 0
#define HOUR_TYPE_24 1

struct RtcData
{
    // 0x00 - seconds
    uint8_t second01:4;
    uint8_t second10:3;
    uint8_t zero0:1;
    
    // 0x01 - minutes
    uint8_t minute01:4;
    uint8_t minute10:3;
    uint8_t zero1:1;
    
    // 0x02 - hours
    uint8_t hour01:4;
    uint8_t hour10:2;
    uint8_t hoursType:1; // 0 = 12, 1 = 24
    uint8_t zero2:1;
    
    // 0x3 - day
    uint8_t day:3;
    uint8_t zero3:5;
    
    // 0x4 - date
    uint8_t date01:4;
    uint8_t date10:2;
    uint8_t zero4:2;
    
    // 0x5 - month
    uint8_t month01:4;
    uint8_t month10:1;
    uint8_t zero5:2;
    uint8_t century:1;
    
    // 0x6 - year
    uint8_t year01:4;
    uint8_t year10:4;
};

extern struct RtcData gRtc;

void RTC_Read(void);

void RTC_Set(volatile struct DateTime* dt);

void ConvertRtcToDateTime(const volatile struct RtcData* rtc, volatile struct DateTime* datetime);

void ConvertDateTimeToRtc(volatile struct RtcData* rtc, const volatile struct DateTime* datetime, uint8_t hourType);

#endif	/* RTC_H */

