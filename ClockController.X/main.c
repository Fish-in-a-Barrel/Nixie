#include "config_bits.h"
#include "i2c_register_bits.h"
#include "pps_outputs.h"

#define _XTAL_FREQ (1000 * 1000ul) // 1 MHz
#define I2C_BAUD (100 * 1000ul) // 100 kHz

void I2C_Host_Init()
{
    // enable I2C pins SCL and SDA for serial communication (§25.2.4)
    SSP1CON1 = SSPxCON1_SSPEN_ENABLED | SSPxCON1_SSMP_HOST;
    SSP1CON2 = 0x00;
    
    // disable slew rate control for standard speed
    SSP1STAT |= 
              SSPxSTAT_SLEW_RATE_CTL_ENABLED // 100 kHz
            | SSPxSTAT_CKE_SMBUS_DISABLED;
    
    // set the baud rate (§25.3)
    SSP1ADD = _XTAL_FREQ / (4 * I2C_BAUD + 1);
    
    // Remap the SDA/SLC pins to 4/5 (§18.2, Table 18-1 / §18.8.2)
    SSP1DATPPS = 4;
    SSP1CLKPPS = 5;
    RA4PPS = PPS_OUT_SDA1;
    RA5PPS = PPS_OUT_SCL1;
    
    // set RA4 & RA5 as digital inputs (§25.2.2.3)
    TRISA = 0x30;
}

void I2C_IsIdle()
{
  while (
          (SSP1STAT & SSPxSTAT_START_MASK) || 
          (SSP1CON2 & (SSPxCON2_ACKEN_ENABLED | SSPxCON2_RCEN_ENABLED)));
}

void I2C_Start()
{
    I2C_IsIdle();
    
     // initiate start condition on SDA line
    SSP1CON2 |= SSPxCON2_SEN_ENABLED;
    while (SSP1CON2 & SSPxCON2_SEN_ENABLED);
}

void I2C_Stop()
{
    I2C_IsIdle();
    
    // initiate Stop condition on SDA and SCL pins
    SSP1CON2 |= SSPxCON2_PEN_ENABLED;
}

void I2C_Restart()
{
    I2C_IsIdle();

    // initiate Repeated Start condition on SDA and SCL pins.
    SSP1CON2 |= SSPxCON2_RSEN_ENABLED;
}

void I2C_ACK(void)
{
    I2C_IsIdle();
    
    // acknowledge Data bit
    SSP1CON2 &= ~SSPxCON2_ACKDT_ACK;
    
    // acknowledge Sequence Enable bit
    SSP1CON2 |= ~SSPxCON2_ACKEN_ENABLED;
}

void I2C_NACK(void)
{
    I2C_IsIdle();
    
    // don't acknowledge Data bit
    SSP1CON2 |= SSPxCON2_ACKDT_ACK;
    
    // acknowledge Sequence Enable bit
    SSP1CON2 |= SSPxCON2_ACKEN_ENABLED;
}

// input parameter to this function can be a byte ( either address or data)
uint8_t I2C_Write(uint8_t data)
{
    I2C_IsIdle();
    
    SSP1BUF = data;
    while (BF) {}
    
    return ACKSTAT;
}

#define I2C_READ 1
#define I2C_WRITE 0
#define I2C_ADDRESS(address, rw) ((address << 1) | rw)

void main(void)
{
    I2C_Host_Init();
    
    uint8_t counter = 0;
    
    while (1)
    {
        I2C_Start();
        I2C_Write(I2C_ADDRESS(0x27, I2C_WRITE)); // address
        I2C_Write(counter % 2 ? 0xFF : 0x00); // data
        I2C_Stop();
        
        counter = (counter + 1) % 10;
        PORTA = (uint8_t)((counter % 2) << 2);
        
        __delay_ms(1000);
    }
}
