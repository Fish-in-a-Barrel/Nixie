#ifndef BOOST_CONTROL_H
#define	BOOST_CONTROL_H

#include <xc.h>

// The Nixie tube will be blanked if the high-voltage supply is outside the target range.
#define HV_TARGET 180
#define HV_DEADBAND 10
#define HV_MIN (HV_TARGET - HV_DEADBAND)
#define HV_MAX (HV_TARGET + HV_DEADBAND)

///
/// Initializes the boost converter.
void InitBoostConverter(void);

///
/// Adjusts the boost converter's PWM duty-cycle based on the current ADC reading.
void UpdateBoostConverter(void);

///
/// @returns The raw duty cycle value set to the PWM.
uint16_t BoostConverter_GetDutyCycle(void);

///
/// @returns The duty cycle set to the PWM as a percentage.
/// @note This is a fixed-precision value with 1 decimal digit.
uint16_t BoostConverter_GetDutyCyclePct(void);

#endif	/* BOOST_CONTROL_H */
