#ifndef PWM_H
#define	PWM_H

#include <xc.h>

void InitPWM(void);

void SetPwmDutyCycle(uint16_t dutyCycle);

#endif	/* PWM_H */
