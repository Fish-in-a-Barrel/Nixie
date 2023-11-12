#include "i2c.h"
#include "i2c_register_bits.h"
#include "clock.h"
#include "pps_outputs.h"

#define I2C_BAUD (100 * 1000ul) // 100 kHz

#define I2C_ADDRESS(address, rw) (uint8_t)((address << 1) | rw)

//
// Operation states
//

#define OP_IDLE 0
#define STATE_DEFAULT 0

// OP_WRITE
#define OP_WRITE 1
#define STATE_WRITE_START 1
#define STATE_WRITE_ADDRESS 2
#define STATE_WRITE_DATA 3

typedef void (*StateHandler)(uint8_t nextState);

static struct
{
    uint8_t type;
    uint8_t state;
    uint8_t address;
    uint8_t* buffer;
    uint8_t bufferLen;
    
    StateHandler stateHandler;
    StateHandler startHandler;
    StateHandler stopHandler;
    StateHandler addressHandler;
    StateHandler dataAckHandler;
    StateHandler dataNackHandler;
} operation;

void ClearOp()
{
    operation.type = OP_IDLE;
    operation.state = 0;
    operation.buffer = NULL;
    operation.bufferLen = 0;
    
    operation.stateHandler = NULL;
    operation.startHandler = operation.stopHandler = operation.addressHandler = operation.dataAckHandler = operation.dataNackHandler = NULL;
}

uint8_t IsBusy()
{
    return operation.type != OP_IDLE || SSP1STATbits.S;
}

void I2C_Host_Init(void)
{
    // Remap the SDA/SLC pins to 4/5 (§18.2, Table 18-1 / §18.8.2)
    SSP1DATPPS = 4;
    SSP1CLKPPS = 5;
    RA4PPS = PPS_OUT_SDA1;
    RA5PPS = PPS_OUT_SCL1;
    
    // set RA4 & RA5 as digital inputs (§25.2.2.3)
    TRISA |= 0x30;
    
    // Clear the analog registers (§16.5)
    ANSELA = 0x00;

    // Clear the GPIO pins
    PORTA = 0;
    
    // disable slew rate control for standard speed
    SSP1STAT |= 
              SSPxSTAT_SLEW_RATE_CTL_ENABLED // 100 kHz
            | SSPxSTAT_CKE_SMBUS_DISABLED;
    
    // enable I2C pins SCL and SDA for serial communication (§25.2.4)
    SSP1CON1 = SSPxCON1_SSMP_HOST;
    SSP1CON2 = 0;
    SSP1CON3 = 0;
    
    // set the baud rate (§25.3)
    SSP1ADD = _XTAL_FREQ / (4 * I2C_BAUD + 1);
    
    // Enable the port (§25.4.5)
    SSP1CON1bits.SSPEN = 1;
    
    // Do not use ClearOp or function duplication will occur since this is non-interrupt code.
    operation.type = OP_IDLE;
}

void I2C_HandleInterrupt(void)
{
//    if (SSP1STATbits.S) // Start
//    {
//        if (operation.startHandler) operation.startHandler(STATE_DEFAULT);
//    }
//    else if (SSP1STATbits.P) // Stop
//    {
//        if (operation.stopHandler) operation.stopHandler(STATE_DEFAULT);
//    }
//    else if (!SSP1STATbits.D_nA) // Address
//    {
//        if (operation.addressHandler) operation.addressHandler(STATE_DEFAULT);
//    }
//    else if (SSP1CON2bits.ACKSTAT) // Data + ACK
//    {
//        if (operation.dataAckHandler) operation.dataAckHandler(STATE_DEFAULT);
//    }
//    else // Data + NACK
//    {
//        if (operation.dataNackHandler) operation.dataNackHandler(STATE_DEFAULT);
//    }
    
    operation.stateHandler(STATE_DEFAULT);
    
    // Clear the interrupt flag
    PIR1bits.SSP1IF = 0;
}

void I2C_HandleCollisionInterrupt(void)
{
    // Clear the interrupt flag
    PIR1bits.BCL1IF = 0;
}

void I2C_Start()
{
    SSP1CON2bits.SEN = 1;
}

void I2C_Stop()
{
    SSP1CON2bits.PEN = 1;
}

void I2C_Restart()
{
    SSP1CON2bits.SEN = 1;
}

void I2C_SendACK(void)
{
    SSP1CON2bits.ACKDT = 0;
    SSP1CON2bits.ACKEN = 1;
}

void I2C_SendNACK(void)
{
    SSP1CON2bits.ACKDT = 1;
    SSP1CON2bits.ACKEN = 1;
}

void WriteAddress(uint8_t nextState)
{
    SSP1BUF = operation.address;
    operation.state = STATE_WRITE_ADDRESS;
}

void WriteData(uint8_t nextState)
{
    if ((NULL == operation.buffer) || (0 == operation.bufferLen))
    {
        I2C_Stop();
        ClearOp();
    }
    else
    {
        --operation.bufferLen;
        SSP1BUF = *(operation.buffer++);
        operation.state = STATE_WRITE_DATA;
    }
}

void WriteHandler(uint8_t nextState)
{
    switch (operation.state)
    {
        case STATE_WRITE_START:
            WriteAddress(STATE_DEFAULT);
            break;
        case STATE_WRITE_ADDRESS:
        case STATE_WRITE_DATA:
            WriteData(STATE_DEFAULT);
            break;
        default:
            ClearOp();
            break;
    }
}

uint8_t I2C_Write(uint8_t address, uint8_t direction, uint8_t* data, uint8_t len)
{
    if (IsBusy()) return 0;
    
    operation.type = OP_WRITE;
    operation.state = STATE_WRITE_START;
    operation.address = I2C_ADDRESS(address, direction);
    operation.buffer = data;
    operation.bufferLen = len;
    
    operation.stateHandler = WriteHandler;
    operation.startHandler = WriteAddress;
    operation.addressHandler = NULL;
    operation.dataAckHandler = WriteData;
    operation.dataNackHandler = NULL;
    
    I2C_Start();
    
    return ACKSTAT;
}
