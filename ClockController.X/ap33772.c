#include "ap33772.h"
#include "i2c.h"
#include "clock.h"

#include <xc.h>

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

static const uint8_t AP33772_ADDR = 0x51;

#define AP33772_PDO_TYPE_FIXED 0
#define AP33772_PDO_TYPE_AUG 0b11

struct AP33772_PowerDataObject
{
  union
  {
    struct
    {
      uint8_t _1;
      uint8_t _2;
      uint8_t _3:6;
      uint8_t type:2;
    } any;

    struct // any.type = PowerDataObjectType::FIXED
    {
      uint8_t maxI_lo:8; // LSB = 10mA
      uint8_t maxI_hi:2;
      uint8_t maxV_lo:8; // LSB = 50mV
      uint8_t maxV_hi:2;
    } fixed;

    struct // any.type = PowerDataObjectType::AUGMENTED
    {
      uint8_t maxI:7; // LSB = 50mA
      uint8_t _1:1;
      uint8_t minV:8; // LSB = 100mV
      uint8_t _2:1;
      uint8_t maxV:8; // LSB = 100mV
      uint8_t _3:3;
      uint8_t pps:2;
    } augmented;
  };
};

struct AP33772_RequestDataObject
{
  uint8_t maxI_lo:8;        // LSB = 10mA
  uint8_t maxI_hi:2;
  uint8_t operatingI_lo:8;  // LSB = 10mA
  uint8_t operatingI_hi:2;
  uint8_t _1:8;
  uint8_t objectPosition:3; // NOTE: PDO position is 1-based
  uint8_t _2:1;
};

struct AP33772_Status
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

#define AP33772_MaxPdoCount 7
struct AP33772_PowerDataObject pdos[AP33772_MaxPdoCount];
uint8_t pdoCount = 0;

struct AP33772_Status AP33772_GetStatus(void)
{
    uint8_t command = AP33772_CMD_STATUS;
    struct AP33772_Status status;
    
    I2C_WriteRead(AP33772_ADDR, &command, sizeof(command), &status, sizeof(status));
    
    return status;
}

uint8_t AP33772_IsReady(void)
{
    struct AP33772_Status status = AP33772_GetStatus();
    return *(uint8_t*)(&status) != 0xFF;
}

uint8_t AP33772_UpdatePDOs(void)
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

// We'll be looking for a PDO with a voltage of 20,000mV. With an LSB value of 50mV, that means we're looking for a value of 400, or 0x0190.
#define V_20_HI 0x01
#define V_20_LO 0x90

uint8_t AP33772_SelectPDO(void)
{
    uint8_t id = 0;
    while (id < AP33772_MaxPdoCount)
    {        
        if ((AP33772_PDO_TYPE_FIXED == pdos[id].any.type) && (V_20_HI == pdos[id].fixed.maxV_hi) && (V_20_LO == pdos[id].fixed.maxV_lo)) break;
        ++id;
    }
    
    if (id >= AP33772_MaxPdoCount) return 0;
    
    uint8_t buffer[sizeof(struct AP33772_RequestDataObject) + 1];
    buffer[0] = AP33772_CMD_RDO;
    
    struct AP33772_RequestDataObject* rdo = (struct AP33772_RequestDataObject*)(buffer + 1);
    rdo->objectPosition = id + 1; // PDO positions are 1-index in the protocol.
    rdo->operatingI_lo = 50;      // 500mA operating current.
    rdo->operatingI_hi = 0;
    rdo->maxI_lo = 100;           // 1,000mA max current.
    rdo->maxI_hi = 0;
    
    uint8_t command = AP33772_CMD_RDO;
    I2C_Write(AP33772_ADDR, &buffer, sizeof(buffer));
    
    struct AP33772_Status status = { 0 };
    while (!status.ready)
    {
        __delay_ms(10);
        status = AP33772_GetStatus();
    }
    
    return status.success;
}

void AP33772Init(void)
{
    // Wait for the AP33772 to bootstrap
    while (!AP33772_IsReady()) __delay_ms(10);
    
    AP33772_UpdatePDOs();
    AP33772_SelectPDO();
}