#include "bcd_utils.h"

int8_t BcdToBinary(volatile const char* bcd, uint8_t len)
{
    int8_t binary = 0;
    while (len > 0)
        binary += binary * 10 + bcd[--len];

    return binary;
}

void BinaryToBcd(int8_t data, volatile char* bcd, uint8_t len)
{
    while (len > 0)
    {
        bcd[--len] = data % 10;
        data /= 10;
    }
}
