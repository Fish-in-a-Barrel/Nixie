#include <xc.H>

#include "gps.h"
#include "bcd_utils.h"
#include "time_utils.h"

volatile struct GpsData gpsData;

#define STATE_AWAIT_START 0
#define STATE_CONSUME_HEADER 1
#define STATE_AWAIT_FIELD 2
#define STATE_CONSUME_FIELD 3
#define STATE_END 4

#define FIELD_TIME 0
#define FIELD_STATUS 1
#define FIELD_LAT 2
#define FIELD_LON 3
#define FIELD_DATE 4
#define LAST_FIELD FIELD_DATE

volatile uint8_t* FIELD[] =
{
    &(gpsData.time),
    &(gpsData.status),
    &(gpsData.lat),
    &(gpsData.lon),
    &(gpsData.date)
};

const uint8_t FIELD_SIZE[5] =
{
    sizeof(gpsData.time),
    sizeof(gpsData.status),
    sizeof(gpsData.lat),
    sizeof(gpsData.lon),
    sizeof(gpsData.date)
};

const uint8_t FIELD_OFFSET[5] =
{
    1, // (1) time
    1, // (2) status
    1, // (3) lat
    2, // (5) lon
    4  // (9) date
};

uint8_t gState = STATE_AWAIT_START;
uint8_t gField = FIELD_TIME;
uint8_t gCharCounter = 0;

void AwaitStart(char data)
{
    // Start character is '$'
    if ('$' == data)
    {
        gState = STATE_CONSUME_HEADER;
        gCharCounter = 0;
    }
}

void ConsumeHeader(char data)
{
    const char HEADER[] = "GPRMC";
    
    if (HEADER[gCharCounter++] != data)
    {
        // The header does not match the message we want.
        gState = STATE_AWAIT_START;
    }
    
    if (gCharCounter >= sizeof(HEADER) - 1)
    {
        // The header matches the message we want: start consuming fields;
        gState = STATE_AWAIT_FIELD;
        gField = FIELD_TIME;
        gCharCounter = 1;
    }
}

void AwaitField(char data)
{
    if ((',' == data) && (--gCharCounter == 0))
    {
        gState = STATE_CONSUME_FIELD;
    }
    else if ('*' == data)
    {
        // We have unexpectedly read the start of the checksum.
        // This would only happen if we somehow missed the start of the field. Restart to recover the state machine.
        gState = STATE_AWAIT_START;
    }
}

void AwaitNextField()
{
    if (LAST_FIELD <= gField)
    {
        gState = STATE_END;
    }
    else
    {
        gState = STATE_AWAIT_FIELD;
        ++gField;
        gCharCounter = FIELD_OFFSET[gField];
    }
}

void ConsumeField(char data)
{
    if (',' == data)
    {
        // The field ended prematurely. The GPS probably doesn't have a fix yet.
        for (uint8_t i = 0; i < FIELD_SIZE[gField]; ++i) FIELD[gField][i] = 0;
        AwaitNextField();
    }
    else
    {
        FIELD[gField][gCharCounter++] = data;
        if (FIELD_SIZE[gField] <= gCharCounter) AwaitNextField();
    }
}

static char buffer[64];
static uint8_t index = 0;
void ClearBuffer()
{
    for (uint8_t i = 0; i < sizeof(buffer); ++i) buffer[i] = 0;
    index = 0;
}

void GPS_HandleInterrupt(void)
{
    // Clear the interrupt
    PIR1bits.RC1IF = 0;
    
    if (RC1STAbits.OERR)
    {
        RC1STAbits.CREN = 0;
        RC1STAbits.CREN = 1;
    }
    
    uint8_t error = RC1STAbits.FERR;
    char data = RC1REG;
    
    if (error || (gState == STATE_AWAIT_START)) ClearBuffer();
    
    buffer[index++] = data;
    
    // Reset the state machine if there is a framing error.
    if (error)
    {
        gState = STATE_AWAIT_START;
        return;
    }
    
    // Run the state machine.
    switch (gState)
    {
        case STATE_AWAIT_START: AwaitStart(data); break;
        case STATE_CONSUME_HEADER: ConsumeHeader(data); break;
        case STATE_AWAIT_FIELD: AwaitField(data); break;
        case STATE_CONSUME_FIELD: ConsumeField(data); break;
        
        // We should never get here.
        default: AwaitStart(data);
    }
    
    // Handle the state machine reaching the end state.
    if (STATE_END == gState)
    {
        gpsData.updated = 1;
        gState = STATE_AWAIT_START;
    }
}
