#include "./BSP/WIRELESS/wireless_control.h"

#include "./BSP/24CXX/24cxx.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define WIRELESS_NVS_ADDR       0U
#define WIRELESS_NVS_MAGIC      0x57434647UL /* WCFG */
#define WIRELESS_NVS_VERSION    2U

typedef struct
{
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    wireless_config_t cfg;
    uint32_t checksum;
} wireless_nvs_blob_t;

static wireless_config_t g_wireless_cfg;
static wireless_send_cb_t g_wireless_send_cb = NULL;
static bool g_wireless_nvs_ready = false;

static const wireless_config_t g_wireless_default = {
    .fc_mhz = 35U,
    .mode = WIRELESS_MODE_CW,
    .sd_mv = 500U,
    .sd_phase_deg = 0U,
    .am_depth_pct = 50U,
    .sm_delay_ns = 80U,
    .sm_phase_deg = 30U,
    .sm_atten_db = 6U,
    .square_khz = 1000U,
    .out_a = WIRELESS_OUT_SD,
    .out_b = WIRELESS_OUT_SQUARE,
    .auto_send = 1U,
};

static uint32_t wireless_checksum(const uint8_t *data, uint16_t len)
{
    uint32_t h = 2166136261UL;

    for (uint16_t i = 0; i < len; i++)
    {
        h ^= data[i];
        h *= 16777619UL;
    }

    return h;
}

static uint16_t wireless_clamp_u16(uint16_t v, uint16_t min_v, uint16_t max_v)
{
    if (v < min_v)
    {
        return min_v;
    }
    if (v > max_v)
    {
        return max_v;
    }
    return v;
}

static uint16_t wireless_snap_u16(uint16_t v, uint16_t min_v, uint16_t max_v, uint16_t step)
{
    uint32_t n;

    v = wireless_clamp_u16(v, min_v, max_v);
    n = ((uint32_t)(v - min_v) + (step / 2U)) / step;
    v = (uint16_t)(min_v + (n * step));
    return wireless_clamp_u16(v, min_v, max_v);
}

static uint8_t wireless_snap_u8(uint8_t v, uint8_t min_v, uint8_t max_v, uint8_t step)
{
    uint16_t snapped = wireless_snap_u16(v, min_v, max_v, step);
    return (uint8_t)snapped;
}

static void wireless_normalize(wireless_config_t *cfg)
{
    if (cfg == NULL)
    {
        return;
    }

    cfg->fc_mhz = wireless_snap_u8(cfg->fc_mhz, 30U, 40U, 1U);
    cfg->mode = (cfg->mode == WIRELESS_MODE_AM) ? WIRELESS_MODE_AM : WIRELESS_MODE_CW;
    cfg->sd_mv = wireless_snap_u16(cfg->sd_mv, 100U, 1000U, 100U);
    cfg->sd_phase_deg = wireless_snap_u16(cfg->sd_phase_deg, 0U, 330U, 30U);
    cfg->am_depth_pct = wireless_snap_u8(cfg->am_depth_pct, 30U, 90U, 10U);
    cfg->sm_delay_ns = wireless_snap_u16(cfg->sm_delay_ns, 50U, 200U, 30U);
    cfg->sm_phase_deg = wireless_snap_u16(cfg->sm_phase_deg, 0U, 180U, 30U);
    cfg->sm_atten_db = wireless_snap_u8(cfg->sm_atten_db, 0U, 20U, 2U);
    cfg->square_khz = wireless_snap_u16(cfg->square_khz, 100U, 5000U, 100U);
    if (cfg->out_a >= WIRELESS_OUT_COUNT)
    {
        cfg->out_a = WIRELESS_OUT_SD;
    }
    if (cfg->out_b >= WIRELESS_OUT_COUNT)
    {
        cfg->out_b = WIRELESS_OUT_SM;
    }
    cfg->auto_send = cfg->auto_send ? 1U : 0U;
}

static void wireless_step_u8(uint8_t *value, uint8_t min_v, uint8_t max_v, uint8_t step, int8_t dir)
{
    int16_t v;

    if (value == NULL || dir == 0)
    {
        return;
    }

    v = (int16_t)*value + ((dir > 0) ? step : -(int16_t)step);
    if (v < (int16_t)min_v)
    {
        v = (int16_t)max_v;
    }
    else if (v > (int16_t)max_v)
    {
        v = (int16_t)min_v;
    }
    *value = (uint8_t)v;
}

static void wireless_step_u16(uint16_t *value, uint16_t min_v, uint16_t max_v, uint16_t step, int8_t dir)
{
    int32_t v;

    if (value == NULL || dir == 0)
    {
        return;
    }

    v = (int32_t)*value + ((dir > 0) ? step : -(int32_t)step);
    if (v < (int32_t)min_v)
    {
        v = (int32_t)max_v;
    }
    else if (v > (int32_t)max_v)
    {
        v = (int32_t)min_v;
    }
    *value = (uint16_t)v;
}

static wireless_output_t wireless_step_output(wireless_output_t value, int8_t dir)
{
    int16_t v;

    if (dir == 0)
    {
        return value;
    }

    v = (int16_t)value + ((dir > 0) ? 1 : -1);
    if (v < 0)
    {
        v = WIRELESS_OUT_COUNT - 1;
    }
    else if (v >= WIRELESS_OUT_COUNT)
    {
        v = 0;
    }

    return (wireless_output_t)v;
}

void Wireless_ControlInit(void)
{
    at24cxx_init();
    g_wireless_nvs_ready = (at24cxx_check() == 0U);
    if (!Wireless_ControlLoad())
    {
        Wireless_ControlResetDefaults();
        (void)Wireless_ControlSave();
    }
}

void Wireless_ControlSetSendCallback(wireless_send_cb_t callback)
{
    g_wireless_send_cb = callback;
}

const wireless_config_t *Wireless_ControlGet(void)
{
    return &g_wireless_cfg;
}

void Wireless_ControlSet(const wireless_config_t *cfg)
{
    if (cfg == NULL)
    {
        return;
    }

    g_wireless_cfg = *cfg;
    wireless_normalize(&g_wireless_cfg);
}

void Wireless_ControlResetDefaults(void)
{
    g_wireless_cfg = g_wireless_default;
}

bool Wireless_ControlSave(void)
{
    wireless_nvs_blob_t blob;

    if (!g_wireless_nvs_ready)
    {
        return false;
    }

    memset(&blob, 0, sizeof(blob));
    blob.magic = WIRELESS_NVS_MAGIC;
    blob.version = WIRELESS_NVS_VERSION;
    blob.size = sizeof(blob.cfg);
    blob.cfg = g_wireless_cfg;
    wireless_normalize(&blob.cfg);
    blob.checksum = wireless_checksum((const uint8_t *)&blob.cfg, sizeof(blob.cfg));

    at24cxx_write(WIRELESS_NVS_ADDR, (uint8_t *)&blob, sizeof(blob));
    return true;
}

bool Wireless_ControlLoad(void)
{
    wireless_nvs_blob_t blob;

    if (!g_wireless_nvs_ready)
    {
        return false;
    }

    memset(&blob, 0, sizeof(blob));
    at24cxx_read(WIRELESS_NVS_ADDR, (uint8_t *)&blob, sizeof(blob));

    if (blob.magic != WIRELESS_NVS_MAGIC ||
        blob.version != WIRELESS_NVS_VERSION ||
        blob.size != sizeof(blob.cfg))
    {
        return false;
    }

    if (blob.checksum != wireless_checksum((const uint8_t *)&blob.cfg, sizeof(blob.cfg)))
    {
        return false;
    }

    g_wireless_cfg = blob.cfg;
    wireless_normalize(&g_wireless_cfg);
    return true;
}

void Wireless_ControlFormatCommand(const wireless_config_t *cfg, char *buf, uint16_t buf_size)
{
    wireless_config_t tmp;

    if (buf == NULL || buf_size == 0U)
    {
        return;
    }

    tmp = (cfg != NULL) ? *cfg : g_wireless_cfg;
    wireless_normalize(&tmp);

    snprintf(buf,
             buf_size,
             "WCFG,%lu,%s,%u,%u,%u,%u,%u,%u,%s,%s,%lu,%lu",
             (unsigned long)tmp.fc_mhz * 1000000UL,
             Wireless_ModeName(tmp.mode),
             tmp.sd_mv,
             tmp.sd_phase_deg,
             tmp.am_depth_pct,
             tmp.sm_delay_ns,
             tmp.sm_phase_deg,
             tmp.sm_atten_db,
             Wireless_OutputName(tmp.out_a),
             Wireless_OutputName(tmp.out_b),
             (unsigned long)WIRELESS_MOD_HZ,
             (unsigned long)tmp.square_khz * 1000UL);
}

void Wireless_ControlSendCurrent(void)
{
    char line[WIRELESS_CMD_MAX_LEN];

    Wireless_ControlFormatCommand(&g_wireless_cfg, line, sizeof(line));
    if (g_wireless_send_cb != NULL)
    {
        g_wireless_send_cb(line);
    }
}

void Wireless_ControlSendBootDefault(void)
{
    if (g_wireless_cfg.auto_send != 0U)
    {
        Wireless_ControlSendCurrent();
    }
}

void Wireless_ControlStepFc(int8_t dir)
{
    wireless_step_u8(&g_wireless_cfg.fc_mhz, 30U, 40U, 1U, dir);
}

void Wireless_ControlStepMode(void)
{
    g_wireless_cfg.mode = (g_wireless_cfg.mode == WIRELESS_MODE_AM) ? WIRELESS_MODE_CW : WIRELESS_MODE_AM;
}

void Wireless_ControlStepSdMv(int8_t dir)
{
    wireless_step_u16(&g_wireless_cfg.sd_mv, 100U, 1000U, 100U, dir);
}

void Wireless_ControlStepSdPhase(int8_t dir)
{
    wireless_step_u16(&g_wireless_cfg.sd_phase_deg, 0U, 330U, 30U, dir);
}

void Wireless_ControlStepAmDepth(int8_t dir)
{
    wireless_step_u8(&g_wireless_cfg.am_depth_pct, 30U, 90U, 10U, dir);
}

void Wireless_ControlStepSmDelay(int8_t dir)
{
    wireless_step_u16(&g_wireless_cfg.sm_delay_ns, 50U, 200U, 30U, dir);
}

void Wireless_ControlStepSmPhase(int8_t dir)
{
    wireless_step_u16(&g_wireless_cfg.sm_phase_deg, 0U, 180U, 30U, dir);
}

void Wireless_ControlStepSmAtten(int8_t dir)
{
    wireless_step_u8(&g_wireless_cfg.sm_atten_db, 0U, 20U, 2U, dir);
}

void Wireless_ControlStepSquareKHz(int8_t dir)
{
    wireless_step_u16(&g_wireless_cfg.square_khz, 100U, 5000U, 100U, dir);
}

void Wireless_ControlStepOutA(int8_t dir)
{
    g_wireless_cfg.out_a = wireless_step_output(g_wireless_cfg.out_a, dir);
}

void Wireless_ControlStepOutB(int8_t dir)
{
    g_wireless_cfg.out_b = wireless_step_output(g_wireless_cfg.out_b, dir);
}

void Wireless_ControlToggleAutoSend(void)
{
    g_wireless_cfg.auto_send = g_wireless_cfg.auto_send ? 0U : 1U;
}

const char *Wireless_ModeName(wireless_mode_t mode)
{
    return (mode == WIRELESS_MODE_AM) ? "AM" : "CW";
}

const char *Wireless_OutputName(wireless_output_t out)
{
    switch (out)
    {
        case WIRELESS_OUT_SD:
            return "SD";
        case WIRELESS_OUT_SM:
            return "SM";
        case WIRELESS_OUT_SOUT:
            return "SOUT";
        case WIRELESS_OUT_DC:
            return "DC";
        case WIRELESS_OUT_SQUARE:
            return "SQUARE";
        case WIRELESS_OUT_MOD_SINE:
            return "MOD_SINE";
        default:
            return "SD";
    }
}
