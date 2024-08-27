#ifndef AP33772_H
#define	AP33772_H

#include <xc.h>

union Status
{
    uint8_t raw; ///< The full status byte.
    
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
    union Status status; ///< The status bits.
    uint16_t current; ///< In mA
    uint16_t voltage; ///< in mV
    
    uint8_t selectedPdoPos; ///< The position of the selected PDO (1-indexed).
    uint8_t pdoMaxAmps; ///< In Amps
    uint8_t pdoMaxVolts; ///< In Volts
} AP33772_StatusAndPower;

/// Initializes the AP337772 USB PD client.
///
/// @result A PDO will be selected from the PD host, powering the system.
/// @remark Status messages will be displayed while initializing and on error.
/// @return 1 on success, 0 if an error is encountered.
uint8_t AP33772_Init(void);

void AP33772_GetStatus(struct AP33772_Status* buffer);

#endif	/* AP33772_H */

