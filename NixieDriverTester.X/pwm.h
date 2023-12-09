#ifndef PWM_H
#define	PWM_H

void InitPWM(uint16_t dutyCycle);

// dutyCycle: % as a 10-bit fixed-precision int with 2 mantissa bits
void SetPwmDutyCycle(uint16_t dutyCycle);

#endif	/* PWM_H */
