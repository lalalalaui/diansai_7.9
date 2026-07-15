#include "./BSP/CALIBRATION/calibration_table.h"

#include <stddef.h>

#define CAL_POINT(output_uv_value, gain_a_value, gain_b_value) \
    { (output_uv_value), (gain_a_value), (gain_b_value) }

const uint16_t g_calibration_amplitude_mv[CALIBRATION_AMPLITUDE_POINT_COUNT] = {
    1000U, 900U, 800U, 700U, 600U, 500U, 400U, 300U, 200U, 100U
};

static const calibration_frequency_t g_calibration_table[CALIBRATION_FREQUENCY_COUNT] = {
    {
        30U, 1600,
        {
            CAL_POINT(755000U, 1000U, 1000U), CAL_POINT(683000U, 1000U, 1000U),
            CAL_POINT(612000U, 1000U, 1000U), CAL_POINT(540000U, 1000U, 1000U),
            CAL_POINT(466800U, 1000U, 1000U), CAL_POINT(393000U, 1000U, 1000U),
            CAL_POINT(320000U, 1000U, 1000U), CAL_POINT(243000U, 1000U, 1000U),
            CAL_POINT(164000U, 1000U, 1000U), CAL_POINT( 86000U, 1000U, 1000U)
        }
    },
    {
        31U, 1600,
        {
            CAL_POINT(754000U, 1000U,  990U), CAL_POINT(680000U, 1000U,  990U),
            CAL_POINT(615000U, 1000U,  990U), CAL_POINT(542000U, 1000U,  990U),
            CAL_POINT(471000U, 1000U,  990U), CAL_POINT(397000U, 1000U,  990U),
            CAL_POINT(321000U, 1000U,  990U), CAL_POINT(245000U, 1000U,  990U),
            CAL_POINT(165000U, 1000U,  990U), CAL_POINT( 84000U, 1000U,  990U)
        }
    },
    {
        32U, 1500,
        {
            CAL_POINT(745000U, 1000U, 1000U), CAL_POINT(674000U, 1000U, 1000U),
            CAL_POINT(605000U, 1000U, 1000U), CAL_POINT(534000U, 1000U, 1000U),
            CAL_POINT(464000U, 1000U, 1000U), CAL_POINT(392000U, 1000U, 1000U),
            CAL_POINT(317000U, 1000U, 1000U), CAL_POINT(242000U, 1000U, 1000U),
            CAL_POINT(164000U, 1000U, 1000U), CAL_POINT( 83000U, 1000U, 1000U)
        }
    },
    {
        33U, 1700,
        {
            CAL_POINT(742000U, 987U, 1000U), CAL_POINT(672000U, 987U, 1000U),
            CAL_POINT(603000U, 987U, 1000U), CAL_POINT(533000U, 987U, 1000U),
            CAL_POINT(460000U, 987U, 1000U), CAL_POINT(389000U, 987U, 1000U),
            CAL_POINT(315000U, 987U, 1000U), CAL_POINT(241000U, 987U, 1000U),
            CAL_POINT(163000U, 987U, 1000U), CAL_POINT( 83000U, 987U, 1000U)
        }
    },
    {
        34U, 975,
        {
            CAL_POINT(743000U, 984U, 1000U), CAL_POINT(673000U, 984U, 1000U),
            CAL_POINT(606000U, 984U, 1000U), CAL_POINT(535000U, 984U, 1000U),
            CAL_POINT(464000U, 984U, 1000U), CAL_POINT(393000U, 984U, 1000U),
            CAL_POINT(318000U, 984U, 1000U), CAL_POINT(241000U, 984U, 1000U),
            CAL_POINT(163000U, 984U, 1000U), CAL_POINT( 86000U, 984U, 1000U)
        }
    },
    {
        35U, 2100,
        {
            CAL_POINT(750000U, 950U, 1000U), CAL_POINT(680000U, 950U, 1000U),
            CAL_POINT(610000U, 950U, 1000U), CAL_POINT(540000U, 950U, 1000U),
            CAL_POINT(468000U, 950U, 1000U), CAL_POINT(395000U, 950U, 1000U),
            CAL_POINT(320000U, 950U, 1000U), CAL_POINT(245000U, 950U, 1000U),
            CAL_POINT(165000U, 950U, 1000U), CAL_POINT( 83000U, 950U, 1000U)
        }
    },
    {
        36U, 2500,
        {
            CAL_POINT(770000U, 937U, 1000U), CAL_POINT(688000U, 937U, 1000U),
            CAL_POINT(613000U, 937U, 1000U), CAL_POINT(546000U, 937U, 1000U),
            CAL_POINT(474000U, 937U, 1000U), CAL_POINT(400000U, 937U, 1000U),
            CAL_POINT(324000U, 937U, 1000U), CAL_POINT(246000U, 937U, 1000U),
            CAL_POINT(167000U, 937U, 1000U), CAL_POINT( 87000U, 937U, 1000U)
        }
    },
    {
        37U, 2400,
        {
            CAL_POINT(774000U, 916U, 1000U), CAL_POINT(700000U, 916U, 1000U),
            CAL_POINT(627000U, 916U, 1000U), CAL_POINT(557000U, 916U, 1000U),
            CAL_POINT(481000U, 916U, 1000U), CAL_POINT(408000U, 916U, 1000U),
            CAL_POINT(330000U, 916U, 1000U), CAL_POINT(252000U, 916U, 1000U),
            CAL_POINT(169000U, 916U, 1000U), CAL_POINT( 89000U, 916U, 1000U)
        }
    },
    {
        38U, 2600,
        {
            CAL_POINT(790000U, 905U, 1000U), CAL_POINT(718000U, 905U, 1000U),
            CAL_POINT(637000U, 905U, 1000U), CAL_POINT(566000U, 905U, 1000U),
            CAL_POINT(491000U, 905U, 1000U), CAL_POINT(414000U, 905U, 1000U),
            CAL_POINT(337000U, 905U, 1000U), CAL_POINT(255000U, 905U, 1000U),
            CAL_POINT(173000U, 905U, 1000U), CAL_POINT( 90000U, 905U, 1000U)
        }
    },
    {
        39U, 3000,
        {
            CAL_POINT(802000U, 905U, 1000U), CAL_POINT(725000U, 905U, 1000U),
            CAL_POINT(652000U, 905U, 1000U), CAL_POINT(574000U, 905U, 1000U),
            CAL_POINT(497000U, 905U, 1000U), CAL_POINT(418000U, 905U, 1000U),
            CAL_POINT(340000U, 905U, 1000U), CAL_POINT(257000U, 905U, 1000U),
            CAL_POINT(176000U, 905U, 1000U), CAL_POINT( 92000U, 905U, 1000U)
        }
    },
    {
        40U, 3200,
        {
            CAL_POINT(820000U, 930U, 1000U), CAL_POINT(740000U, 930U, 1000U),
            CAL_POINT(660000U, 930U, 1000U), CAL_POINT(580000U, 930U, 1000U),
            CAL_POINT(500000U, 930U, 1000U), CAL_POINT(430000U, 900U, 1000U),
            CAL_POINT(350000U, 890U, 1000U), CAL_POINT(265000U, 880U, 1000U),
            CAL_POINT(183000U, 850U, 1000U), CAL_POINT( 95000U, 850U, 1000U)
        }
    }
};

const calibration_frequency_t *CalibrationTable_Get(void)
{
    return g_calibration_table;
}

const calibration_frequency_t *CalibrationTable_FindByCarrierMhz(uint32_t carrier_mhz)
{
    uint32_t i;

    for (i = 0U; i < CALIBRATION_FREQUENCY_COUNT; i++)
    {
        if (g_calibration_table[i].carrier_mhz == carrier_mhz)
        {
            return &g_calibration_table[i];
        }
    }

    return NULL;
}

const calibration_amplitude_point_t *CalibrationTable_GetPoint(uint32_t carrier_mhz,
                                                                uint8_t amplitude_index)
{
    const calibration_frequency_t *frequency = CalibrationTable_FindByCarrierMhz(carrier_mhz);

    if (frequency == NULL || amplitude_index >= CALIBRATION_AMPLITUDE_POINT_COUNT)
    {
        return NULL;
    }

    return &frequency->amplitude[amplitude_index];
}
