#ifndef GPS_H
#define	GPS_H

#include <xc.h>

struct GpsData
{
    char time[6];
    char date[6];
    char status;
    char lat[2];
    char lon[3];
    uint8_t dst;
    
    uint8_t updated;
};

volatile extern struct GpsData gpsData;

void GPS_HandleInterrupt(void);

void GPS_ConvertToLocalTime(int8_t tzOffset);

#endif	/* GPS_H */

