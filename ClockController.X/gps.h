#ifndef GPS_H
#define	GPS_H

#include <xc.h>
#include "time_utils.h"

struct GpsData
{
    struct Datetime datetime;

    char status;
    
    uint8_t updated;
};

volatile extern struct GpsData gpsData;

void GPS_HandleInterrupt(void);

void GPS_ConvertToLocalTime(int8_t tzOffset);

#endif	/* GPS_H */

