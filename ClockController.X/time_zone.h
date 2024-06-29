#ifndef TIME_ZONE_H
#define	TIME_ZONE_H

#include <xc.h>

extern int8_t gTimeZoneOffset;

#define TZ_LIST 0
#define TZ_LIST_ST 1
#define TZ_LIST_DST 2

extern const char* TIME_ZONE_ABRV[27][3];

void TimeZone_Save(void);

void TimeZone_Load(void);

#endif	/* TIME_ZONE_H */

