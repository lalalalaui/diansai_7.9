#include "./BSP/SMS/crc8.h"

#include <stddef.h>

uint8_t CRC8_Update(uint8_t crc, uint8_t data)
{
    crc ^= data;

    for (uint8_t i = 0; i < 8U; i++)
    {
        if ((crc & 0x80U) != 0U)
        {
            crc = (uint8_t)((crc << 1U) ^ CRC8_POLY);
        }
        else
        {
            crc = (uint8_t)(crc << 1U);
        }
    }

    return crc;
}

uint8_t CRC8_Calc(const uint8_t *data, uint16_t len)
{
    uint8_t crc = CRC8_INIT;

    if (data == NULL)
    {
        return crc;
    }

    for (uint16_t i = 0; i < len; i++)
    {
        crc = CRC8_Update(crc, data[i]);
    }

    return crc;
}
