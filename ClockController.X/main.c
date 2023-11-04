#include "config_bits.h"
#include "i2c_register_bits.h"

// 1MHz
#define _XTAL_FREQ (1000 * 1000)

void I2C_IDLE()
{
  while ((SSP1STAT & SSPxSTAT_START_MASK) || (SSP1CON2 & (SSPxCON2_ACKEN_ENABLED | SSPxCON2_RCEN_ENABLED)));
}

void main(void) {
    return;
}
