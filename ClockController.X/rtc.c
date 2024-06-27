#include "rtc.h"
#include "i2c.h"

struct RtcData gRtc;

void RTC_Read()
{
    uint8_t READ_START_ADDRESS = 0x00;

    I2C_WriteRead(I2C_RTC_ADDRESS, &READ_START_ADDRESS, sizeof(READ_START_ADDRESS), &gRtc, sizeof(gRtc));
}

void RTC_Set(volatile struct DateTime* dt)
{
    // Byte 0 is the starting register address for the write.
    uint8_t buffer[sizeof(struct RtcData) + 1] = { 0 };
    ConvertDateTimeToRtc((struct RtcData*)(buffer + 1), dt, HOUR_TYPE_24);
    
    I2C_Write(I2C_RTC_ADDRESS, buffer, sizeof(buffer));
}

void ConvertRtcToDateTime(const volatile struct RtcData* rtc, volatile struct DateTime* datetime)
{
    datetime->year = rtc->year10 * 10 + rtc->year01;
    datetime->month = rtc->month10 * 10 + rtc->month01;
    datetime->day = rtc->date10 * 10 + rtc->date01;

    datetime->hour = rtc->hour10 * 10 + rtc->hour01;
    datetime->minute = rtc->minute10 * 10 + rtc->minute01;
    datetime->second = rtc->second10 * 10 + rtc->second01;
}

void ConvertDateTimeToRtc(volatile struct RtcData* rtc, const volatile struct DateTime* datetime, uint8_t hourType)
{
    rtc->hour10 = datetime->hour / 10;
    rtc->hour01 = datetime->hour % 10;
    rtc->hoursType = hourType;

    rtc->minute10 = datetime->minute / 10;
    rtc->minute01 = datetime->minute % 10;

    rtc->second10 = datetime->second / 10;
    rtc->second01 = datetime->second % 10;

    rtc->year10 = datetime->year / 10;
    rtc->year01 = datetime->year % 10;

    rtc->month10 = datetime->month / 10;
    rtc->month01 = datetime->month % 10;

    rtc->date10 = datetime->day / 10;
    rtc->date01 = datetime->day % 10;
}