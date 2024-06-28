#ifndef BOOST_CONTROL_H
#define	BOOST_CONTROL_H

#include <xc.h>

// The Nixie tube will be blanked if the high-voltage supply is outside the target range.
#define HV_TARGET 180
#define HV_DEADBAND 10
#define HV_MIN (HV_TARGET - HV_DEADBAND)
#define HV_MAX (HV_TARGET + HV_DEADBAND)

void InitBoostConverter(void);

void UpdateBoostConverter(void);

uint16_t BoostConverter_GetDutyCycle(void);

uint16_t BoostConverter_GetDutyCyclePct(void);

#endif	/* BOOST_CONTROL_H */
