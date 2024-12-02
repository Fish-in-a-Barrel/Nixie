#include "button.h"

static uint8_t gLastRotation = ROTATION_NONE;

void Buttons_Init(void)
{
    // RC3, RC4, and RC5 are button inputs
    TRISC |= (1 << 3) | (1 << 4) | (1 << 5);
    WPUC |= (1 << 3) | (1 << 4) | (1 << 5);
    
    // Set up Interrupt On Change (IOC) for the pins (§17.3)
    IOCCP |= (1 << 3) | (1 << 4) | (1 << 5);
    IOCCN |= (1 << 3) | (1 << 4) | (1 << 5);
    
    gButtonState.deltaR = 0;
}

void UpdateRotation(void)
{
    int state = 
            gButtonState.b.state << 0 |
            gButtonState.a.state << 1 |
            gButtonState.b.edge  << 2 |
            gButtonState.a.edge  << 3;
    
    // From the PEC11S datasheet
    static const uint8_t ROTATION_FROM_STATE[] =
    {
        // 00xx - non-transition events.
        ROTATION_NONE,
        ROTATION_NONE,
        ROTATION_NONE,
        ROTATION_NONE,
        
        ROTATION_CW,    // 0100 - A = 0, B -> 0
        ROTATION_CCW,   // 0101 - A = 0, B -> 1
        ROTATION_CCW,   // 0110 - A = 1, B -> 0
        ROTATION_CW,    // 0111 - A = 1, B -> 1
        ROTATION_CCW,   // 1000 - A -> 0, B = 0
        ROTATION_CW,    // 1001 - A -> 0, B = 1
        ROTATION_CW,    // 1010 - A -> 1, B = 0
        ROTATION_CCW,   // 1011 - A -> 1, B = 1
        
        // 11xx dual transition events, which shouldn't be possible.
        ROTATION_NONE,
        ROTATION_NONE,
        ROTATION_NONE,
        ROTATION_NONE,
    };
    
    uint8_t rotation = ROTATION_FROM_STATE[state];
    gButtonState.rotation = ROTATION_NONE;
    if (ROTATION_CW == rotation) ++gButtonState.deltaR;
    else if (ROTATION_CCW == rotation) --gButtonState.deltaR;

    // Each click generates two rotation events, so don't set the rotation state until two events of the same type.
    if (rotation != ROTATION_NONE)
    {
        if (gLastRotation == rotation)
        {
            gButtonState.rotation = rotation;
            gLastRotation = ROTATION_NONE;
        }
        else
        {
            gLastRotation = rotation;
        }
    }
}

void Buttons_HandleInterrupt(void)
{
    // IOCxF is a set of flags, so care must be taken to only clear flags we handle in this call.
    // If IOCxF is cleared by this operation, IOCIF is automatically cleared. (§17.5)
    uint8_t iocif = IOCCF;
    IOCCF &= ~iocif;

    gButtonState.a.state = RC5;
    gButtonState.b.state = RC4;
    gButtonState.c.state = RC3;
    
    // Detect the edges (§17.4)
    gButtonState.a.edge = (iocif >> 5) & 1;
    gButtonState.b.edge = (iocif >> 4) & 1;
    gButtonState.c.edge = (iocif >> 3) & 1;

    UpdateRotation();
}

void Button_ResetEdges(void)
{
    gButtonState.a.edge = 0;
    gButtonState.b.edge = 0;
    gButtonState.c.edge = 0;
    gButtonState.deltaR = 0;
    gButtonState.rotation = 0;
}