#include "button.h"

void InitButtons(void)
{
    // RC3, RC4, and RC5 are button inputs
    TRISC |= 0x38;
    
    UpdateButtons();
}

void Debounce(union ButtonState* button)
{
    switch (button->_raw & 0x7)
    {
        // no change in state
        case 0:
        case 7:
            break;
        
        // ignoring transient state change
        case 2:
        case 5:
            break;
            
        // holding state change for debouncing
        case 3:
        case 4:

        // transition from 1 -> 0
        case 1: button->_raw = 0x8; break;
        
        // transition from 0 -> 1
        case 6: button->_raw = 0xF; break;
    }
}

void UpdateRotation(void)
{
    int state = 
            gButtonState.a.edge | 
            gButtonState.b.edge << 1 |
            gButtonState.a.state << 2 |
            gButtonState.b.state << 3;
    
    if ((state & 0x3) == 0)
    {
        // No edge events
        gButtonState.rotation = ROTATION_NONE;
    }
    else
    {
        // From the PEC11S datasheet
        switch (state)
        {
            case 0b1001: // A -> 0, B = 1
            case 0b0100: // A = 0, B -> 0
            case 0b1010: // A -> 1, B = 0 
            case 0b0111: // A = 1, B -> 1
                gButtonState.rotation = ROTATION_CW; break;

            // 0b0101, A = 1, B -> 0
            // 0b1000, A -> 0, B = 0
            // 0b0110, A = 0, B -> 1
            // 0b1011, A -> 1, B = 0
            default:
                gButtonState.rotation = ROTATION_CCW; break;
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
    
    gButtonState.a._pinLast = gButtonState.a._pin;
    gButtonState.b._pinLast = gButtonState.b._pin;
    gButtonState.c._pinLast = gButtonState.c._pin;
    
    UpdateRotation();
}
