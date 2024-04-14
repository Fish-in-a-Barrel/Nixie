#ifndef ADC_H
#define	ADC_H

#include <xc.h>

extern uint16_t gAdcCv;
extern uint8_t gVoltage;

void InitAdc(void);

void AdcInterruptHandler(void);

void CaptureAdc(void);

#endif	/* ADC_H */

