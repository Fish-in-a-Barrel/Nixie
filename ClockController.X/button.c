#include "button.h"

static uint8_t gLastRotation = ROTATION_NONE;

void InitButtons(void)
{
    // RC3, RC4, and RC5 are button inputs
    TRISC |= (1 << 3) | (1 << 4) | (1 << 5);
    WPUC |= (1 << 3) | (1 << 4) | (1 << 5);
    
    UpdateButtons();
}

void Debounce(union ButtonState* button)
{
    if ((button->_raw & 0x7) == 0b001) button->_raw = 0x8; // transition from 1 -> 0
    else if ((button->_raw & 0x7) == 0b110) button->_raw = 0xF; // transition from 0 -> 1
    else button->edge = 0;
    
    // Remember the current pin state for debouncing next pass.
    button->_pinLast = button->_pin;
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
        
        // Skipping 11xx because those are dual transition events, which shouldn't be possible.
        ROTATION_NONE,
        ROTATION_NONE,
        ROTATION_NONE,
        ROTATION_NONE,
    };
    
    uint8_t rotation = ROTATION_FROM_STATE[state];
    gButtonState.rotation = ROTATION_NONE;

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

void UpdateButtons(void)
{
    gButtonState.a._pin = RC5;
    gButtonState.b._pin = RC4;
    gButtonState.c._pin = RC3;
    
    Debounce(&gButtonState.a);
    Debounce(&gButtonState.b);
    Debounce(&gButtonState.c);
    
    UpdateRotation();
}
