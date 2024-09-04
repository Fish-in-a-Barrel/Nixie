#include "nixie.h"
#include "rtc.h"
#include "i2c.h"

uint16_t gNixieStatus = 0;

uint8_t CRC(void* data, uint8_t size)
{
    uint8_t crc = 0;
    for (uint8_t i = 0; i < size; ++i) crc ^= ((uint8_t*)data)[i];
    
    return crc;
}

void UpdateNixieDriver(uint8_t value, uint8_t address)
{
    uint8_t response = 0xFF;
    I2C_WriteRead(address, &value, sizeof(value), &response, sizeof(response));
    
    if (response == value) gNixieStatus |= 1 << address;
    else gNixieStatus &= ~(1 << address);
}

void UpdateNixieDrivers(void)
{
    static uint8_t lastCRC = 0;
    uint8_t crc = CRC(&gRtc, sizeof(gRtc));
    
    if (lastCRC == crc) return;
    lastCRC = crc;    
    
    UpdateNixieDriver(gRtc.second01, 0x06);
    UpdateNixieDriver(gRtc.second10, 0x05);
    UpdateNixieDriver(gRtc.minute01, 0x04);
    UpdateNixieDriver(gRtc.minute10, 0x03);
    UpdateNixieDriver(gRtc.hour01,   0x02);
    UpdateNixieDriver(gRtc.hour10,   0x01);

    UpdateNixieDriver(gRtc.date10,   0x09);
    UpdateNixieDriver(gRtc.date01,   0x0A);
    UpdateNixieDriver(gRtc.month10,  0x0B);
    UpdateNixieDriver(gRtc.month01,  0x0C);
    UpdateNixieDriver(gRtc.year10,   0x0D);
    UpdateNixieDriver(gRtc.year01,   0x0E);
}
