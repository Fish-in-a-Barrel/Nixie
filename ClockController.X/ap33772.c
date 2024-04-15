#include "ap33772.h"
#include "i2c.h"
#include "clock.h"

#include <xc.h>

static const uint8_t AP33772_ADDR = 0x51;

#define AP33772_CMD_SRCPDO  0x00 // Get source PDOs
#define AP33772_CMD_PDONUM  0x1C // Get source PDO count
#define AP33772_CMD_STATUS  0x1D // Get/Clear AP33772 status
#define AP33772_CMD_MASK    0x1E // Get/Set interrupt mask
#define AP33772_CMD_VOLTAGE 0x20 // Get voltage, LSB 80mV
#define AP33772_CMD_CURRENT 0x21 // Get current, LSB 24mA
#define AP33772_CMD_TEMP    0x22 // Get tempurature, °C
#define AP33772_CMD_OCPTHR  0x23 // Get/Set Over-current protection threshold, LSB 50mA
#define AP33772_CMD_OTPTRR  0x24 // Get/Set Over-temperature protection threshold, °C
#define AP33772_CMD_DRTHR   0x25 // Get/Set derating threshold, °C
#define AP33772_CMD_TR25    0x28 // Get/Set thermal resistance @25°C, ?
#define AP33772_CMD_TR50    0x2A // Get/Set thermal resistance @50°C, ?
#define AP33772_CMD_TR75    0x2C // Get/Set thermal resistance @75°C, ?
#define AP33772_CMD_TR100   0x2E // Get/Set thermal resistance @100°C, ?
#define AP33772_CMD_RDO     0x30 // Set Request Data Object
#define AP33772_CMD_VID     0x34 // Get/Set vendor ID (reserved)
#define AP33772_CMD_PID     0x36 // Get/Set product ID (reserved)

#define AP33772_PDO_TYPE_FIXED 0
#define AP33772_PDO_TYPE_AUG 0b11

struct AP33772_PowerDataObject
{
  union
  {
        uint8_t raw[4];
        
        struct {
            uint8_t _1[3];
            uint8_t _2 : 6;
            uint8_t type : 2;
        } any;

        // These structures don't pack correctly because some of the fields span byte boundaries.
        // XC8 will not span a field across a byte, and instead leaves empty space, which breaks the field
        // alignment and causes the union to be oversized.
        
        /*
        struct // any.type = PowerDataObjectType::FIXED
        {
            uint8_t maxI_hi : 8; // LSB = 10mA
            uint8_t maxI_lo : 2;
            uint8_t maxV_hi : 8; // LSB = 50mV
            uint8_t maxV_lo : 2;
            uint8_t _1 : 8;
            uint8_t _2 : 2;
            uint8_t type : 2;
        } fixed;

        struct // any.type = PowerDataObjectType::AUGMENTED
        {
            uint8_t maxI : 7; // LSB = 50mA
            uint8_t _1 : 1;
            uint8_t minV : 8; // LSB = 100mV
            uint8_t _2 : 1;
            uint8_t maxV : 8; // LSB = 100mV
            uint8_t _3 : 3;
            uint8_t pps : 2;
            uint8_t type : 2;
        } augmented;
 */
    };
};

/*
struct AP33772_RequestDataObject {
    uint8_t maxI_hi : 8; // LSB = 10mA
    uint8_t maxI_lo : 2;
    uint8_t operatingI_hi : 8; // LSB = 10mA
    uint8_t operatingI_lo : 2;
    uint8_t _1 : 8;
    uint8_t objectPosition : 3; // NOTE: PDO position is 1-based
    uint8_t _2 : 1;
};
*/

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

#define AP33772_MaxPdoCount 7
struct AP33772_PowerDataObject pdos[AP33772_MaxPdoCount];
uint8_t pdoCount = 0;
uint8_t current24mA = 0;
uint8_t voltage80mV = 0;

union Status GetStatus(void)
{
    uint8_t command = AP33772_CMD_STATUS;
    union Status status;
    
    I2C_WriteRead(AP33772_ADDR, &command, sizeof(command), &status, sizeof(status));
    
    return status;
}

uint8_t IsReady(void)
{
    union Status status = GetStatus();
    return status.raw != 0xFF;
}

uint8_t UpdatePDOs(void)
{
    uint8_t command = AP33772_CMD_PDONUM;
    I2C_WriteRead(AP33772_ADDR, &command, sizeof(command), &pdoCount, sizeof(pdoCount));
    
    if (pdoCount > AP33772_MaxPdoCount)
    {
        pdoCount = 0;
    }
    else
    {
        command = AP33772_CMD_SRCPDO;
        I2C_WriteRead(AP33772_ADDR, &command, sizeof(command), &pdos, sizeof(struct AP33772_PowerDataObject) * pdoCount);
    }
    
    return pdoCount;
}

#define GetVoltage(pdo) ((pdo.raw[1] >> 2) | (((uint16_t)pdo.raw[2] & 0xF) << 6))

// We're looking for a 20V supply. The LSB of the voltage is 50mV, so the value we're looking for is 400.
// Operating current is 500mA -> 50
// Max current is 1000mA -> 100
#define VOLTAGE_OP 400
#define CURRENT_OP 50
#define CURRENT_MAX 100

uint8_t SelectPDO(void)
{
    uint8_t pos = 0;
    while (pos < AP33772_MaxPdoCount)
    {        
        if ((AP33772_PDO_TYPE_FIXED == pdos[pos].any.type) && (GetVoltage(pdos[pos]) == VOLTAGE_OP)) break;
        ++pos;
    }
    
    if (pos >= AP33772_MaxPdoCount) return 0;
    
    uint8_t buffer[5] = { AP33772_CMD_RDO, 0, 0, 0, 0 };
    
    buffer[1] = CURRENT_MAX & 0xFF;
    buffer[2] = ((CURRENT_MAX >> 8) & 0x3) | (CURRENT_OP << 2);
    buffer[3] = CURRENT_OP >> 6;
    buffer[4] = ((pos + 1) << 4) & 0x70;
    
    I2C_Write(AP33772_ADDR, &buffer, sizeof(buffer));
    
    union Status status = { 0 };
    while (!status.ready)
    {
        __delay_ms(10);
        status = GetStatus();
    }
    
    return status.success;
}

void AP33772Init(void)
{
    // Wait for the AP33772 to bootstrap
    while (!IsReady()) __delay_ms(10);
    
    UpdatePDOs();
    SelectPDO();
}