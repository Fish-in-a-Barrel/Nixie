#ifndef TIME_ZONE_H
#define	TIME_ZONE_H

#include <xc.h>

extern int8_t gTimeZoneOffset;

#define TZ_LIST 0
#define TZ_LIST_ST 1
#define TZ_LIST_DST 2

extern char TIME_ZONE_ABRV[27][3][4];

#endif	/* TIME_ZONE_H */

