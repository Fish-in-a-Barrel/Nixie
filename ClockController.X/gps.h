#ifndef GPS_H
#define	GPS_H

/*
 * Code is base on the document "u-blox 6 Receiver Description". Section notes in this file referece that document.
 */

#include <xc.h>
#include "time_utils.h"

// Why is 'V' invalid and 'A' valid? IDK. (UBLOX §18)
#define GPS_STATUS_VALID 'A'
#define GPS_STATUS_INVALID 'V'

struct GpsData
{
    struct DateTime datetime;

    char status;
    
    uint8_t updated;
};

volatile extern struct GpsData gGpsData;

void GPS_HandleInterrupt(void);

void GPS_ConvertToLocalTime(int8_t tzOffset);

#endif	/* GPS_H */

