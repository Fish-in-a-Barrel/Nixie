#ifndef ADC_H
#define	ADC_H

#include <xc.h>

extern uint16_t gAdcCv; ///< The current raw value of the ADC.
extern uint8_t gVoltage;///< The calculated voltage in V.

///
/// Initializes the ADC.
void InitAdc(void);

///
/// Called by the ISR to process ADC interrupts.
void AdcInterruptHandler(void);

///
/// Called to update the gAdcCv and gVoltage globals.
void CaptureAdc(void);

#endif	/* ADC_H */

