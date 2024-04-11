#ifndef BCD_UTILS_H
#define	BCD_UTILS_H

#include <xc.h>

uint8_t BcdToBinary(volatile const char bcd[], uint8_t len);

void BinaryToBcd(uint8_t data, volatile char bcd[], uint8_t len);

#endif	/* BCD_UTILS_H */

