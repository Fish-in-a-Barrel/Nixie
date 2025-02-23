#ifndef ADC_H
#define	ADC_H

#include <xc.h>

extern uint16_t gAdcCv; ///< The current raw value of the ADC.

///
/// Initializes the ADC.
void InitAdc(void);

///
/// Called by the ISR to process ADC interrupts.
void AdcInterruptHandler(void);

///
/// Called to update the gAdcCv and gVoltage globals.
void CaptureAdc(void);

///
/// @returns 1 if the over-volage protection has kicked in.
uint8_t AdcOverVoltageProtectionTripped(void);

#endif	/* ADC_H */

