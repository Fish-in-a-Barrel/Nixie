#include "serial.h"
#include "clock.h"
#include <xc.h>

#define SER_BAUD (9600ul)

void SerialInit(void)
{
    //TX9D 0x0; BRGH hi_speed; SENDB sync_break_complete; SYNC asynchronous; TX9 8-bit; CSRC client; (§24.6.1)
    TX1STA = 0x06;
    //ABDEN disabled; WUE disabled; BRG16 16bit_generator; SCKP Non-Inverted;  (§24.6.3)
    BAUD1CON = 0x48;
    //ADDEN disabled; CREN enabled; SREN disabled; RX9 8-bit; SPEN enabled; (§24.6.2)
    RC1STA = 0x90;
    //SPBRGL 103; SPBRGH 0; (§24.6.6)
    SP1BRGL = _XTAL_FREQ / (4 * SER_BAUD) - 1; 
    SP1BRGH = 0x0; 
}
