#ifndef __AFSK_DEMOD_H
#define __AFSK_DEMOD_H

#include <stdbool.h>
#include <stdint.h>

#define AFSK_MARK_HZ                 2200U
#define AFSK_SPACE_HZ                1200U
#define AFSK_BIT_RATE                100U
#define AFSK_DEFAULT_MIN_RMS         20.0f
#define AFSK_DEFAULT_MIN_RATIO       1.10f

void AFSK_Init(uint32_t sample_rate);
bool AFSK_DecodeBit(const int16_t *samples, uint16_t count, uint8_t *bit);
bool AFSK_DecodeBitEx(const int16_t *samples, uint16_t count, uint8_t *bit,
                       float *out_e1200, float *out_e2200);

uint16_t AFSK_GetSamplesPerBit(void);
void AFSK_SetThresholds(float min_rms, float min_ratio);

#endif
