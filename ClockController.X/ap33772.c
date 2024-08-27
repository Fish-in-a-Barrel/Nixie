#include "ap33772.h"
#include "i2c.h"
#include "clock.h"
#include "oled.h"

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

// Least-significant-bit values of different voltage/amperage values
#define REG_V_LSB 80
#define REG_A_LSB 24
#define PDO_V_LSB 50
#define PDO_A_LSB 10
#define RDO_FIXED_A_LSB 10

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
        //
        // The structures are left for reference.
        
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

#define AP33772_MaxPdoCount 7
struct AP33772_PowerDataObject pdos[AP33772_MaxPdoCount];
uint8_t pdoCount = 0;
uint8_t selectedPdo = 0;

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
    return (status.raw != 0xFF) && status.ready && status.newPDO && status.success;
}

void UpdatePDOs(void)
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
}

#define GetVoltage(pdo) ((pdo.raw[1] >> 2) | (((uint16_t)pdo.raw[2] & 0xF) << 6))
#define GetAmperage(pdo) (pdo.raw[0] | (((uint16_t)pdo.raw[1] & 0x3) << 8))

#define VOLTAGE 12
#define PDO_RAW_VOLTAGE ((VOLTAGE * 1000) / PDO_V_LSB)
#define RDO_CURRENT_OP (500 / RDO_FIXED_A_LSB)
#define RDO_CURRENT_MAX (1000 / RDO_FIXED_A_LSB)

uint8_t SelectPDO(void)
{
    // Find a fixed PDO with the desired voltage
    while (selectedPdo < pdoCount)
    {        
        if ((AP33772_PDO_TYPE_FIXED == pdos[selectedPdo].any.type) && 
                (GetVoltage(pdos[selectedPdo]) == PDO_RAW_VOLTAGE)) break;
        ++selectedPdo;
    }
    
    if (selectedPdo >= pdoCount)
    {
        OLED_DrawString(3, 2, "Inadequate power!", 0);
        return 0;
    }
    
    uint8_t rdo[5] = { AP33772_CMD_RDO, 0, 0, 0, 0 };
    
    rdo[1] = RDO_CURRENT_MAX & 0xFF;
    rdo[2] = ((RDO_CURRENT_MAX >> 8) & 0x3) | (RDO_CURRENT_OP << 2);
    rdo[3] = RDO_CURRENT_OP >> 6;
    rdo[4] = ((selectedPdo + 1) << 4) & 0x70; // PDO positions are 1-indexed in the protocol
    
    I2C_Write(AP33772_ADDR, &rdo, sizeof(rdo));
    
    union Status status = { 0 };
    while (!status.ready)
    {
        __delay_ms(10);
        status = GetStatus();
    }
    
    return status.success;
}

uint8_t AP33772_Init(void)
{
    OLED_DrawString(0, 0, "Waiting for USB PD...", 1);
    
    uint8_t waitCounter = 0;
    
    // Wait for the AP33772 to bootstrap
    while (!IsReady())
    {
        // If we've waited 1+ seconds, reset the AP33772
        if (waitCounter++ > 99)
        {
            waitCounter = 0;
            
            uint8_t buffer[5] = { AP33772_CMD_RDO, 0, 0, 0, 0 };
            I2C_Write(AP33772_ADDR, &buffer, sizeof(buffer));
        }
        
        __delay_ms(10);
    }
    
    OLED_DrawString(1, 0, "  Waiting for PDOs...", 1);

    while (pdoCount == 0)
    {
        UpdatePDOs();
        __delay_ms(10);
    }
    
    return SelectPDO();
}

void AP33772_GetStatus(struct AP33772_Status* buffer)
{
    uint8_t command = AP33772_CMD_CURRENT;
    uint8_t rawCurrent = 0;
    I2C_WriteRead(AP33772_ADDR, &command, sizeof(command), &rawCurrent, sizeof(rawCurrent));

    command = AP33772_CMD_VOLTAGE;
    uint8_t rawVoltage = 0;
    I2C_WriteRead(AP33772_ADDR, &command, sizeof(command), &rawVoltage, sizeof(rawVoltage));

    buffer->status.raw = GetStatus().raw;
    buffer->current = (uint16_t)rawCurrent * REG_A_LSB;
    buffer->voltage = (uint16_t)rawVoltage * REG_V_LSB;
    
    buffer->selectedPdoPos = selectedPdo;
    buffer->pdoMaxAmps = (uint8_t)(GetAmperage(pdos[selectedPdo]) / (1000 / PDO_A_LSB));
    buffer->pdoMaxVolts = (uint8_t)(GetVoltage(pdos[selectedPdo]) / (1000 / PDO_V_LSB));
}