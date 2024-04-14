#ifndef ADC_H
#define	ADC_H

#include <xc.h>

void InitAdc(void);

extern uint32_t gAdcAccumulator;
extern uint16_t gAdcAccumulatorCount;
extern uint16_t gAdcCv;

#endif	/* ADC_H */

