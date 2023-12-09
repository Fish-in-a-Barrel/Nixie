#include "i2c.h"
#include "i2c_register_bits.h"
#include "clock.h"
#include "pps_outputs.h"

#define I2C_BAUD (400 * 1000ul) // 100 kHz

#define I2C_ADDRESS(address, rw) (uint8_t)((address << 1) | rw)

#define I2C_READ 1
#define I2C_WRITE 0

//
// Operation states
//

#define OP_IDLE 0

// Operations
#define OP_WRITE 1
#define OP_WRITE_READ 2

// States
#define STATE_ERROR 0xFF
#define STATE_IDLE 0
#define STATE_WRITE_ADDRESS 2
#define STATE_WRITE_DATA 3
#define STATE_READ_START 4
#define STATE_READ_DATA 5
#define STATE_READ_COMPLETE 6

static struct
{
    uint8_t type;
    uint8_t state;
    
    uint8_t address;
    
    WriteCallback* callback;
    struct WriteCallbackContext* callbackContext;
    
    const uint8_t* writeBuffer;
    uint8_t writeBufferLen;
    uint8_t* readBuffer;
    uint8_t readBufferLen;
} operation;

void ClearOp()
{
    if (operation.state == STATE_ERROR) 
    {
        SSP1CON1bits.SSPEN = 0;
        SSP1CON1bits.SSPEN = 1;        
    }

    operation.type = OP_IDLE;
    operation.state = STATE_IDLE;
    operation.callback = NULL;
    operation.callbackContext = NULL;
    operation.writeBuffer = NULL;
    operation.writeBufferLen = 0;
    operation.readBuffer = NULL;
    operation.readBufferLen = 0;
}

uint8_t IsBusy()
{
    return operation.type != OP_IDLE || SSP1STATbits.S || SSP1STATbits.BF;
}

void I2C_Host_Init(void)
{
    // disable slew rate control for standard speed
    SSP1STAT |= 
              SSPxSTAT_SLEW_RATE_CTL_ENABLED // 100 kHz
            | SSPxSTAT_CKE_SMBUS_DISABLED;
    
    // enable I2C pins SCL and SDA for serial communication (§25.2.4)
    SSP1CON1 = SSPxCON1_SSMP_HOST;
    SSP1CON2 = 0;
    SSP1CON3 = SSPxCON3_PCIE_ENABLED | SSPxCON3_SCIE_ENABLED;
    
    // set the baud rate (§25.3)
    SSP1ADD = _XTAL_FREQ / (4 * I2C_BAUD + 1);
    
    // Enable the port (§25.4.5)
    SSP1CON1bits.SSPEN = 1;
    
    // Do not use ClearOp or function duplication will occur since this is non-interrupt code.
    operation.type = OP_IDLE;
}

uint8_t Start()
{
    SSP1CON2bits.SEN = 1;
    return STATE_WRITE_ADDRESS;
}

uint8_t Stop()
{
    SSP1CON2bits.PEN = 1;
    ClearOp();
    
    return STATE_IDLE;
}

uint8_t Restart()
{
    SSP1CON2bits.RSEN = 1;
    
    return STATE_WRITE_ADDRESS;
}

void SendACK(void)
{
    SSP1CON2bits.ACKDT = 0;
    SSP1CON2bits.ACKEN = 1;
}

void SendNACK(void)
{
    SSP1CON2bits.ACKDT = 1;
    SSP1CON2bits.ACKEN = 1;
}

uint8_t WriteAddress(uint8_t direction)
{
    SSP1BUF = I2C_ADDRESS(operation.address, direction);
    return (I2C_WRITE == direction) ? STATE_WRITE_DATA : STATE_READ_DATA;
}

uint8_t WriteData()
{
    if (operation.callback && operation.callback(operation.callbackContext))
    {
        SSP1BUF = operation.callbackContext->data;
        ++operation.callbackContext->count;
        
        return STATE_WRITE_DATA;
    }
    else if (operation.writeBuffer && operation.writeBufferLen)
    {
        SSP1BUF = *(operation.writeBuffer++);
        --operation.writeBufferLen;

        return STATE_WRITE_DATA;
    }
    else
    {
        if (OP_WRITE_READ == operation.type)
        {
            Restart();
            return WriteAddress(I2C_READ);
        }
        else
        {
            return Stop();
        }
    }
}

uint8_t ReadData()
{
    if (!SSP1STATbits.BF)
    {
        SSP1CON2bits.RCEN = 1;
        return STATE_READ_DATA;
    }
    else if (operation.readBuffer && operation.readBufferLen)
    {
        *(operation.readBuffer++) = SSP1BUF;
        --operation.readBufferLen;
        
        if (operation.readBufferLen > 0)
        {
            SendACK();
            return STATE_READ_DATA;
        }
        else
        {
            SendNACK();
            return STATE_READ_COMPLETE;            
        }
    }
    else
    {
        // We shouldn't actually reach this point.
        
        SendNACK();
        return STATE_READ_COMPLETE;            
    }
}

void ExecuteStateMachine()
{   
    if (SSP1CON2bits.ACKSTAT) operation.state = STATE_ERROR;
    
    switch (operation.state)
    {
        case STATE_WRITE_ADDRESS:
            operation.state = WriteAddress(
                    (operation.writeBufferLen || operation.callback) ? I2C_WRITE : I2C_READ);
            break;
        case STATE_WRITE_DATA:
            operation.state = WriteData();
            break;
        case STATE_READ_DATA:
            operation.state = ReadData();
            break;
        case STATE_READ_COMPLETE:
            Stop();
            break;
            
        case STATE_IDLE:
            break;

        default:
            ClearOp();
            break;
    }
}

void I2C_Write(uint8_t address, const void* data, uint8_t len)
{
    if (IsBusy()) return;
    
    operation.type = OP_WRITE;
    operation.address = address;
    operation.writeBuffer = data;
    operation.writeBufferLen = len;
    
    operation.callback = NULL;
    operation.callbackContext = NULL;    
    operation.readBuffer = NULL;
    operation.readBufferLen = 0;
    
    operation.state = STATE_WRITE_ADDRESS;
    Start();
    
    while (IsBusy());
}

void I2C_WriteWithCallback(uint8_t address, WriteCallback* callback, struct WriteCallbackContext* context)
{
    if (IsBusy()) return;
    
    operation.type = OP_WRITE;
    operation.address = address;
    operation.callback = callback;
    operation.callbackContext = context;
    operation.callbackContext->count = 0;    
    
    operation.writeBuffer = 0;
    operation.writeBufferLen = 0;
    operation.readBuffer = 0;
    operation.readBufferLen = 0;
    
    operation.state = STATE_WRITE_ADDRESS;
    Start();
    
    while (IsBusy());
}

/*
 * not needed for this application
void I2C_WriteRead(uint8_t address, const void* writeData, uint8_t writeLen, void* readData, uint8_t readLen)
{
    if (IsBusy()) return;
    
    operation.type = OP_WRITE_READ;
    operation.address = address;
    operation.writeBuffer = writeData;
    operation.writeBufferLen = writeLen;
    operation.readBuffer = readData;
    operation.readBufferLen = readLen;
    
    operation.state = Start();
    
    while (IsBusy());
}
*/

void I2C_HandleInterrupt(void)
{
    if (PIR1bits.SSP1IF)
    {
        PIR1bits.SSP1IF = 0;
        ExecuteStateMachine();
    }
    else if (PIR1bits.BCL1IF)
    {
        PIR1bits.BCL1IF = 0;
        operation.state = STATE_ERROR;
        ClearOp();
    }
}
