#ifndef AP33772_H
#define	AP33772_H

#include <xc.h>

union Status
{
    uint8_t raw;
    
    struct
    {
        uint8_t ready:1;
        uint8_t success:1;
        uint8_t newPDO:1;
        uint8_t _:1;
        uint8_t overVoltageProtection:1;
        uint8_t overCurrentProtection:1;
        uint8_t overTemperaturProtection:1;
        uint8_t derating:1;
    };
};

struct AP33772_Status
{
    union Status status;
    uint16_t current; // In mA
    uint16_t voltage; // in mV
    
    uint8_t selectedPdoPos;
    uint8_t pdoMaxAmps;
    uint8_t pdoMaxVolts;
} AP33772_StatusAndPower;

uint8_t AP33772Init(void);

void AP33772_GetStatus(struct AP33772_Status* buffer);

#endif	/* AP33772_H */

