#ifndef PWM_H
#define	PWM_H

#include <xc.h>

void InitPWM(void);

void SetPwmDutyCycle(uint16_t dutyCycle);

void PWM_Enable(uint16_t dutyCycle);

void PWM_Disable(void);

#endif	/* PWM_H */
