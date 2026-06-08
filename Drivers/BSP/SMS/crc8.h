#ifndef __CRC8_H
#define __CRC8_H

#include <stdint.h>

#define CRC8_POLY   0x07U
#define CRC8_INIT   0x00U

uint8_t CRC8_Update(uint8_t crc, uint8_t data);
uint8_t CRC8_Calc(const uint8_t *data, uint16_t len);

#endif
