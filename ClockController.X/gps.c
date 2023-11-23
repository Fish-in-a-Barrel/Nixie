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

volatile char** FIELD[5] =
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

char buffer[64];
uint16_t bi = 0;

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
    else if (gCharCounter >= sizeof(HEADER))
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
        gCharCounter = 0;
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

// Updates the passed date with the date of the requested Sunday in the month and year passed
// whichSunday: the 1-based count of the Sunday requested
void FindSunday(char* date, uint8_t whichSunday)
{
    uint8_t sunday = (whichSunday - 1) * 7 + 1;
    sunday += ((7 - GetDayOfWeek(date) + DOW_SUNDAY) % 7);
    BinaryToBcd(sunday, date, 2);
}

int8_t GetTimeZoneOffset()
{
    int8_t lon = BcdToBinary(gpsData.lon, sizeof(gpsData.lon));
    
    return lon / 15;
}

void RollYearBack()
{
    int8_t year = BcdToBinary(gpsData.date + 4, 2) - 1;
    BinaryToBcd(year, gpsData.date + 4, 2);
}

int8_t RollMonthBack()
{
    int8_t month = BcdToBinary(gpsData.date + 2, 2);
    
    if (--month == 0)
    {
        month = 12;
        RollYearBack();
    }
    
    BinaryToBcd(month, gpsData.date + 2, 2);
    return month;
}

void RollDayBack()
{
    int8_t day = BcdToBinary(gpsData.date, 2);
    
    if (--day == 0)
    {
        int8_t month = RollMonthBack();
        
        switch (month)
        {
            // 31 day months
            case 1: // January
            case 3: // March
            case 5: // May
            case 7: // July
            case 8: // August
            case 10: // October
            case 12: // December
                day = 31;
                break;
                
            // 30 day months
            case 4: // April
            case 6: // June
            case 9: // September
            case 11: // November
                day = 30;
                break;
                
            // February - I'm not bothering with anything more than standard leap days
            case 2:
            {
                int8_t year = BcdToBinary(gpsData.date + 4, 2);
                day = (year % 4) ? 29 : 28;
                break;
            }

            default:
                // LOL what?!?
                break;
        }
    }
    
    BinaryToBcd(day, gpsData.date, 2);
}

void RollTimeBack(int8_t tzOffset)
{
    int8_t hour = BcdToBinary(gpsData.time, 2);
    hour -= tzOffset;
    
    // If hour < 0, then UTC is already at tomorrow and we need to roll the date back one day.
    if (hour < 0)
    {
        hour += 24;
        RollDayBack();
    }
    
    BinaryToBcd(hour, gpsData.time, 2);
}

void RollMonthForward()
{
    int8_t month = BcdToBinary(gpsData.date + 2, 2);

    // This code is only triggered by DST=true, which means we don't have to worry about rolling over the year.
    BinaryToBcd(++month, gpsData.date + 2, 2);
}

void RollDayForward()
{
    int8_t day = BcdToBinary(gpsData.date, 2);
    int8_t month = BcdToBinary(gpsData.date + 2, 2);
    
    // This code is only triggered by DST=true, which means we only need to consider March->November
    int8_t dayMax = 31;
    switch (month)
    {
        // 30 day months
        case 4: // April
        case 6: // June
        case 9: // September
        case 11: // November
            day = 30;
            break;
    }
    
    if (++day > dayMax)
    {
        day = 1;
        RollMonthForward();        
    }
    
    BinaryToBcd(day, gpsData.date, 2);
}

void UpdateDST()
{
    // DST starts on the second Sunday in March
    char dstStart[6] = "dd03yy";
    dstStart[4] = gpsData.date[4];
    dstStart[5] = gpsData.date[5];
    FindSunday(dstStart, 2);

    // DST ends on the first Sunday in November
    char dstEnd[6] = "dd11yy";
    dstEnd[4] = gpsData.date[4];
    dstEnd[5] = gpsData.date[5];
    FindSunday(dstEnd, 1);
    
    // Times are in standard time, so the end time is 1AM instead of 2AM.
    gpsData.dst = 
        DateTimeAfter(gpsData.date, gpsData.time, dstStart, "020000") &&
        DateTimeBefore(gpsData.date, gpsData.time, dstEnd, "010000");

    if (gpsData.dst && (BcdToBinary(gpsData.time, 2) + 1 >= 24))
    {
        RollDayForward();
    
        BinaryToBcd(0, gpsData.time, 2);
    }
}

// This is a very crude UTC to local time conversion
void GPS_ConvertToLocalTime()
{
    int8_t tzOffset = GetTimeZoneOffset();
    
    RollTimeBack(tzOffset);
    UpdateDST();
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
    
    // Reset the state machine if there is a framing error.
    if (error)
    {
        gState = STATE_AWAIT_START;
        return;
    }
    
    buffer[bi++] = data;
    if (bi > 64)
    {
        bi = 0;
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
