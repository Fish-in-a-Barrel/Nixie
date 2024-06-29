#ifndef TIME_ZONE_H
#define	TIME_ZONE_H

#include <xc.h>

#define DST_TYPE_OFF 0
#define DST_TYPE_AUTO_US 1

extern int8_t gTimeZoneOffset;
extern uint8_t gDstType;

#define TZ_LIST 0
#define TZ_LIST_ST 1
#define TZ_LIST_DST 2

extern const char* TIME_ZONE_ABRV[27][3];

extern const char* DST_TYPE_ABRV[2];

void TimeZone_Save(void);

void TimeZone_Load(void);

#endif	/* TIME_ZONE_H */

