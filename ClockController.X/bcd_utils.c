#include "bcd_utils.h"

int8_t BcdToBinary(volatile const char bcd[], uint8_t len)
{
    int8_t binary = 0;
    for (uint8_t i = 0; i < len; ++i)
        binary = binary * 10 + (bcd[i] - '0');

    return binary;
}

void BinaryToBcd(int8_t data, volatile char bcd[], uint8_t len)
{
    for (uint8_t i = 1; i <= len; ++i)
    {
        bcd[len - i] = (data % 10) + '0';
        data /= 10;
    }
}
