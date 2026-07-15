#ifndef __CALIBRATION_TABLE_H
#define __CALIBRATION_TABLE_H

#include <stdint.h>

#define CALIBRATION_FREQUENCY_COUNT        11U
#define CALIBRATION_AMPLITUDE_POINT_COUNT  10U
#define CALIBRATION_GAIN_SCALE             1000U
#define CALIBRATION_PHASE_SCALE            1000U

/*
 * The table is const data stored in the application Flash, so its default
 * calibration values remain available after power loss.
 */
typedef struct
{
    uint32_t output_uv;
    uint16_t gain_a_x1000;
    uint16_t gain_b_x1000;
} calibration_amplitude_point_t;

typedef struct
{
    uint32_t carrier_mhz;
    int32_t initial_phase_diff_x1000;
    calibration_amplitude_point_t amplitude[CALIBRATION_AMPLITUDE_POINT_COUNT];
} calibration_frequency_t;

extern const uint16_t g_calibration_amplitude_mv[CALIBRATION_AMPLITUDE_POINT_COUNT];

const calibration_frequency_t *CalibrationTable_Get(void);
const calibration_frequency_t *CalibrationTable_FindByCarrierMhz(uint32_t carrier_mhz);
const calibration_amplitude_point_t *CalibrationTable_GetPoint(uint32_t carrier_mhz,
                                                                uint8_t amplitude_index);

#endif
