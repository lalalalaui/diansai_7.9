#include "./BSP/AFSK/afsk_demod.h"

#include <math.h>
#include <stddef.h>

#define AFSK_PI                     3.14159265358979323846f
#define AFSK_MIN_SAMPLE_RATE_HZ     4000U
#define AFSK_MAX_SAMPLE_RATE_HZ     48000U
#define AFSK_MIN_SAMPLES_PER_BIT    8U
#define AFSK_MAX_COUNT_ERROR_DIV    4U

typedef struct
{
    uint32_t sample_rate;
    uint16_t samples_per_bit;
    float coeff_1200;
    float coeff_2200;
    float min_rms;
    float min_ratio;
} afsk_demod_t;

static afsk_demod_t g_afsk;

static float afsk_calc_coeff(uint32_t sample_rate, uint32_t freq_hz)
{
    float omega = (2.0f * AFSK_PI * (float)freq_hz) / (float)sample_rate;

    return 2.0f * cosf(omega);
}

static float afsk_goertzel_energy(const int16_t *samples, uint16_t count, float coeff, float mean)
{
    float q0;
    float q1 = 0.0f;
    float q2 = 0.0f;

    for (uint16_t i = 0; i < count; i++)
    {
        float x = (float)samples[i] - mean;

        q0 = x + coeff * q1 - q2;
        q2 = q1;
        q1 = q0;
    }

    return (q1 * q1) + (q2 * q2) - (coeff * q1 * q2);
}

static bool afsk_count_valid(uint16_t count)
{
    uint16_t tolerance;
    uint16_t min_count;

    if (g_afsk.samples_per_bit < AFSK_MIN_SAMPLES_PER_BIT || count < AFSK_MIN_SAMPLES_PER_BIT)
    {
        return false;
    }

    tolerance = g_afsk.samples_per_bit / AFSK_MAX_COUNT_ERROR_DIV;
    if (tolerance == 0U)
    {
        tolerance = 1U;
    }

    /* allow count down to 2/3 of samples_per_bit for edge guard */
    min_count = (g_afsk.samples_per_bit * 2U) / 3U;
    if (count < min_count)
    {
        return false;
    }

    if (count > g_afsk.samples_per_bit + tolerance)
    {
        return false;
    }

    return true;
}

void AFSK_Init(uint32_t sample_rate)
{
    if (sample_rate < AFSK_MIN_SAMPLE_RATE_HZ)
    {
        sample_rate = 8000U;
    }
    else if (sample_rate > AFSK_MAX_SAMPLE_RATE_HZ)
    {
        sample_rate = AFSK_MAX_SAMPLE_RATE_HZ;
    }

    g_afsk.sample_rate = sample_rate;
    g_afsk.samples_per_bit = (uint16_t)((sample_rate + (AFSK_BIT_RATE / 2U)) / AFSK_BIT_RATE);
    if (g_afsk.samples_per_bit < AFSK_MIN_SAMPLES_PER_BIT)
    {
        g_afsk.samples_per_bit = AFSK_MIN_SAMPLES_PER_BIT;
    }

    g_afsk.coeff_1200 = afsk_calc_coeff(sample_rate, AFSK_SPACE_HZ);
    g_afsk.coeff_2200 = afsk_calc_coeff(sample_rate, AFSK_MARK_HZ);
    g_afsk.min_rms = AFSK_DEFAULT_MIN_RMS;
    g_afsk.min_ratio = AFSK_DEFAULT_MIN_RATIO;
}

bool AFSK_DecodeBit(const int16_t *samples, uint16_t count, uint8_t *bit)
{
    float mean;
    float signal_energy = 0.0f;
    float min_signal_energy;
    float energy_1200;
    float energy_2200;
    float strong;
    float weak;
    int64_t sum = 0;

    if (samples == NULL || bit == NULL || !afsk_count_valid(count))
    {
        return false;
    }

    for (uint16_t i = 0; i < count; i++)
    {
        sum += samples[i];
    }

    mean = (float)sum / (float)count;

    for (uint16_t i = 0; i < count; i++)
    {
        float x = (float)samples[i] - mean;

        signal_energy += x * x;
    }

    min_signal_energy = g_afsk.min_rms * g_afsk.min_rms * (float)count;
    if (signal_energy < min_signal_energy)
    {
        return false;
    }

    energy_1200 = afsk_goertzel_energy(samples, count, g_afsk.coeff_1200, mean);
    energy_2200 = afsk_goertzel_energy(samples, count, g_afsk.coeff_2200, mean);

    if (energy_1200 >= energy_2200)
    {
        strong = energy_1200;
        weak = energy_2200;
        *bit = 0U;
    }
    else
    {
        strong = energy_2200;
        weak = energy_1200;
        *bit = 1U;
    }

    if (strong <= 0.0f || strong < (weak * g_afsk.min_ratio))
    {
        return false;
    }

    return true;
}

bool AFSK_DecodeBitEx(const int16_t *samples, uint16_t count, uint8_t *bit,
                       float *out_e1200, float *out_e2200)
{
    float mean;
    float signal_energy = 0.0f;
    float min_signal_energy;
    float energy_1200;
    float energy_2200;
    float strong;
    float weak;
    int64_t sum = 0;

    if (out_e1200 != NULL) { *out_e1200 = 0.0f; }
    if (out_e2200 != NULL) { *out_e2200 = 0.0f; }

    if (samples == NULL || bit == NULL || !afsk_count_valid(count))
    {
        return false;
    }

    for (uint16_t i = 0; i < count; i++)
    {
        sum += samples[i];
    }

    mean = (float)sum / (float)count;

    for (uint16_t i = 0; i < count; i++)
    {
        float x = (float)samples[i] - mean;

        signal_energy += x * x;
    }

    min_signal_energy = g_afsk.min_rms * g_afsk.min_rms * (float)count;
    if (signal_energy < min_signal_energy)
    {
        return false;
    }

    energy_1200 = afsk_goertzel_energy(samples, count, g_afsk.coeff_1200, mean);
    energy_2200 = afsk_goertzel_energy(samples, count, g_afsk.coeff_2200, mean);

    if (out_e1200 != NULL) { *out_e1200 = energy_1200; }
    if (out_e2200 != NULL) { *out_e2200 = energy_2200; }

    if (energy_1200 >= energy_2200)
    {
        strong = energy_1200;
        weak = energy_2200;
        *bit = 0U;
    }
    else
    {
        strong = energy_2200;
        weak = energy_1200;
        *bit = 1U;
    }

    if (strong <= 0.0f || strong < (weak * g_afsk.min_ratio))
    {
        return false;
    }

    return true;
}

uint16_t AFSK_GetSamplesPerBit(void)
{
    return g_afsk.samples_per_bit;
}

void AFSK_SetThresholds(float min_rms, float min_ratio)
{
    if (min_rms >= 0.0f)
    {
        g_afsk.min_rms = min_rms;
    }

    if (min_ratio >= 1.0f)
    {
        g_afsk.min_ratio = min_ratio;
    }
}
