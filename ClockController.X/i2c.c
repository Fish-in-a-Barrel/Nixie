#include "i2c.h"
#include "i2c_register_bits.h"
#include "clock.h"
#include "pps_outputs.h"

#if _I2C_TRACE
    char gEventTrace[32] = {0};
    uint8_t gEventIndex = 0;

    #define RESET_TRACE() gEventTrace[gEventIndex] = '.'; gEventIndex = 0;
    #define ADD_EVENT(C) gEventTrace[gEventIndex++ % 32] = C;

    char* I2C_GetEventTrace(void) { return gEventTrace; }
#else
    #define RESET_TRACE()
    #define ADD_EVENT(C)

    char* I2C_GetEventTrace(void) { return null; }
#endif

#define I2C_BAUD (400 * 1000ul) // 400 kHz

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
#define OP_READ 3

// States
#define STATE_ERROR 0xFF
#define STATE_OK 0
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
    
    const uint8_t* writeBuffer;
    uint8_t writeBufferLen;
    uint8_t* readBuffer;
    uint8_t readBufferLen;

    WriteCallback* callback;
    struct WriteCallbackContext* callbackContext;
} operation;

static uint8_t gErrorCount = 0;
static uint8_t gResetCount = 0;

void ClearOp()
{
    RESET_TRACE();
    
    operation.type = OP_IDLE;
    if (operation.state != STATE_ERROR) operation.state = STATE_OK;
    
    operation.writeBuffer = NULL;
    operation.writeBufferLen = 0;
    operation.readBuffer = NULL;
    operation.readBufferLen = 0;
    
    operation.callback = NULL;
    operation.callbackContext = NULL;
}

void HandleError()
{
    ADD_EVENT('!');
    
    operation.state = STATE_ERROR;
    ++gErrorCount;
    ClearOp();
}

// This function is used to convince client devices to let go of the bus if they have desynced from the host.
// This usually happens as a result of a controller reset (e.g. during debugging).
void SynchronizeClients(void)
{
    TRISC |= 0x02;  // Set RC1 (SDA) to input
    TRISC &= ~0x01; // Set RC0 (SCL) to output
    
    RC0 = 1;
    uint8_t count = 8 + 1;
    
    // If SDA is being held low, toggle SCL
    while (!RC1 && (--count != 0))
    {
        RC0 = 0;
        __delay_ms(1);
        RC0 = 1;
        __delay_ms(1);
    }
    
    RC0 = 0;
    
    // Reset RC0 & RC1 to digital inputs (§25.2.2.3)
    TRISC |= 0x03;
}

void ResetBus()
{
    if (SSP1STATbits.S || SSP1STATbits.BF)
    {
        ADD_EVENT('*');
        
        // Reset the port
        SSP1CON1bits.SSPEN = 0;
        
        SynchronizeClients();
        
        SSP1CON1bits.SSPEN = 1;
    
        ++gResetCount;
    }
}

uint8_t IsDone()
{
    return OP_IDLE == operation.type;
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
    
    // Set the pad I2C controls (§16.11, §16.14.9)
    RC0I2C = PU_2X | SLEW_FAST | TH_I2C;
    RC1I2C = PU_2X | SLEW_FAST | TH_I2C;
    
    // Enable the port (§25.4.5)
    SSP1CON1bits.SSPEN = 1;
    
    // Do not use ClearOp or function duplication will occur since this is non-interrupt code.
    operation.type = OP_IDLE;
}

void Start(void)
{
    ADD_EVENT('S');
    
    // Set the state first, otherwise it's possible for an interrupt to occur while the state is still "IDLE" because this is non-interrupt code.
    operation.state = STATE_WRITE_ADDRESS;
    SSP1CON2bits.SEN = 1;
}

uint8_t Stop(void)
{
    ADD_EVENT('Q');
    
    SSP1CON2bits.PEN = 1;
    ClearOp();
    
    return STATE_OK;
}

uint8_t Restart(void)
{
    ADD_EVENT('R');
    
    SSP1CON2bits.RSEN = 1;
    
    return STATE_WRITE_ADDRESS;
}

void SendACK(void)
{
    ADD_EVENT('A');
    SSP1CON2bits.ACKDT = 0;
    SSP1CON2bits.ACKEN = 1;
}

void SendNACK(void)
{
    ADD_EVENT('N');
    SSP1CON2bits.ACKDT = 1;
    SSP1CON2bits.ACKEN = 1;
}

uint8_t WriteAddress(uint8_t direction)
{
    ADD_EVENT('W');
    
    SSP1BUF = I2C_ADDRESS(operation.address, direction);
    return (I2C_WRITE == direction) ? STATE_WRITE_DATA : STATE_READ_DATA;
}

uint8_t WriteData()
{
    if (operation.callback && operation.callback(operation.callbackContext))
    {
        ADD_EVENT('c');
    
        SSP1BUF = operation.callbackContext->data;
        ++operation.callbackContext->count;
        
        return STATE_WRITE_DATA;
    }
    else if (operation.writeBuffer && operation.writeBufferLen)
    {
        ADD_EVENT('w');
    
        SSP1BUF = *(operation.writeBuffer++);
        --operation.writeBufferLen;

        return STATE_WRITE_DATA;
    }
    else
    {
        if (OP_WRITE_READ == operation.type)
        {
            return Restart();
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
        ADD_EVENT('~');
        
        SSP1CON2bits.RCEN = 1;
        return STATE_READ_DATA;
    }
    else if (operation.readBuffer && operation.readBufferLen)
    {
        ADD_EVENT('r');

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
    if (SSP1CON2bits.ACKSTAT) HandleError();
    
    switch (operation.state)
    {
        case STATE_WRITE_ADDRESS:
            operation.state = WriteAddress(operation.writeBufferLen || operation.callback ? I2C_WRITE : I2C_READ);
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
            
        case STATE_OK:
        case STATE_ERROR:
            break;

        default:
            ClearOp();
            break;
    }
}

void I2C_Write(uint8_t address, const void* data, uint8_t len)
{
    ResetBus();
    
    operation.type = OP_WRITE;
    operation.address = address;
    operation.writeBuffer = data;
    operation.writeBufferLen = len;
    operation.readBuffer = NULL;
    operation.readBufferLen = 0;
    operation.callback = NULL;

    Start();
    
    while (!IsDone());
}

void I2C_Read(uint8_t address, void* data, uint8_t len)
{
    ResetBus();
    
    operation.type = OP_READ;
    operation.address = address;
    operation.writeBuffer = NULL;
    operation.writeBufferLen = 0;
    operation.readBuffer = data;
    operation.readBufferLen = len;
    operation.callback = NULL;

    Start();
    
    while (!IsDone());
}

void I2C_WriteRead(uint8_t address, const void* writeData, uint8_t writeLen, void* readData, uint8_t readLen)
{
    ResetBus();
    
    operation.type = OP_WRITE_READ;
    operation.address = address;
    operation.writeBuffer = writeData;
    operation.writeBufferLen = writeLen;
    operation.readBuffer = readData;
    operation.readBufferLen = readLen;
    operation.callback = NULL;

    Start();
    
    while (!IsDone());
}

void I2C_WriteWithCallback(uint8_t address, WriteCallback* callback, struct WriteCallbackContext* context)
{
    ResetBus();
    
    operation.type = OP_WRITE;
    operation.address = address;
    operation.callback = callback;
    operation.callbackContext = context;
    operation.callbackContext->count = 0;    
    
    operation.writeBuffer = NULL;
    operation.writeBufferLen = 0;    
    operation.readBuffer = NULL;
    operation.readBufferLen = 0;

    Start();
    
    while (!IsDone());
}

void I2C_HandleInterrupt(void)
{
    if (PIR1bits.SSP1IF)
    {
        ADD_EVENT('I')
        PIR1bits.SSP1IF = 0;
        ExecuteStateMachine();
    }
    else if (PIR1bits.BCL1IF)
    {
        ADD_EVENT('X')
        PIR1bits.BCL1IF = 0;
        HandleError();
    }
}
