#ifndef GPS_H
#define	GPS_H

/*
 * Code is base on the document "u-blox 6 Receiver Description". Section notes in this file referece that document.
 */

#include <xc.h>
#include "time_utils.h"

// Why is 'V' invalid and 'A' valid? IDK. (UBLOX �18)
#define GPS_STATUS_VALID 'A'
#define GPS_STATUS_INVALID 'V'

struct GpsData
{
    struct DateTime datetime; ///< The date/time retrieved from GPS. This is in UTC unless converted.

    char status; ///< The status of the GPS.
    
    uint8_t updated; ///< Set when new data is available. Client should clear the bit after processing the data.
};

///
/// The last read GPS data.
volatile extern struct GpsData gGpsData;

///
/// Called by the ISR to process GPS (serial) interrupts.
void GPS_HandleInterrupt(void);

/// Converts gGpsData.datetime to local time by applying the passed timezone offset.
///
/// @param tzOffset The local time offset from UTC in hours.
void GPS_ConvertToLocalTime(int8_t tzOffset);

#endif	/* GPS_H */

