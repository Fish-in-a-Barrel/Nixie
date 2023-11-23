#ifndef GPS_H
#define	GPS_H

#include <xc.h>

struct GpsData
{
    uint8_t time[6];
    uint8_t date[6];
    uint8_t status;
    uint8_t lat[2];
    uint8_t lon[3];
    uint8_t dst;
    
    uint8_t updated;
};

volatile extern struct GpsData gpsData;

void GPS_HandleInterrupt(void);

void GPS_ConvertToLocalTime(void);

#endif	/* GPS_H */

