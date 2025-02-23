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
void BoostConverter_Init(void);

///
/// Adjusts the boost converter's PWM duty-cycle based on the current ADC reading.
/// @param adc The current ADC value.
void BoostConverter_Update(uint16_t adc);

///
/// @returns The voltage level.
uint8_t BoostConverter_GetVoltage(void);

///
/// @returns The raw duty cycle value set to the PWM.
uint16_t BoostConverter_GetDutyCycle(void);

///
/// @returns The duty cycle set to the PWM as a percentage.
/// @note This is a fixed-precision value with 1 decimal digit.
uint16_t BoostConverter_GetDutyCyclePct(void);

///
/// @returns 0 if OVP is off, !=0 if it is on.
uint8_t BoostConverter_OverVoltageProtectionOn(void);

#endif	/* BOOST_CONTROL_H */
