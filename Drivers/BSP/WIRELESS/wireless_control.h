#ifndef __WIRELESS_CONTROL_H
#define __WIRELESS_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

#define WIRELESS_OUT_NAME_LEN  11U
#define WIRELESS_CMD_MAX_LEN   192U
#define WIRELESS_MOD_HZ        2000000UL

typedef enum
{
    WIRELESS_MODE_CW = 0,
    WIRELESS_MODE_AM = 1
} wireless_mode_t;

typedef enum
{
    WIRELESS_OUT_SD = 0,
    WIRELESS_OUT_SM,
    WIRELESS_OUT_SOUT,
    WIRELESS_OUT_DC,
    WIRELESS_OUT_SQUARE,
    WIRELESS_OUT_MOD_SINE,
    WIRELESS_OUT_COUNT
} wireless_output_t;

typedef struct
{
    uint32_t fc_mhz;            /* MHz, step 1 MHz */
    wireless_mode_t mode;       /* CW/AM */
    uint32_t sd_mv;             /* mVrms, step 100 mV */
    uint32_t sd_phase_deg;      /* deg, step 30 deg */
    uint32_t am_depth_pct;      /* %, step 10 % */
    uint32_t sm_delay_ns;       /* ns, step 30 ns */
    uint32_t sm_phase_deg;      /* deg, step 30 deg */
    uint32_t sm_atten_db;       /* dB, step 2 dB */
    uint32_t square_khz;        /* kHz, step 100 kHz */
    wireless_output_t out_a;    /* DAC A output select */
    wireless_output_t out_b;    /* DAC B output select */
    uint8_t auto_send;          /* send current params on boot */
} wireless_config_t;

typedef void (*wireless_send_cb_t)(const char *line);

void Wireless_ControlInit(void);
void Wireless_ControlSetSendCallback(wireless_send_cb_t callback);
const wireless_config_t *Wireless_ControlGet(void);
void Wireless_ControlSet(const wireless_config_t *cfg);
void Wireless_ControlResetDefaults(void);
bool Wireless_ControlSave(void);
bool Wireless_ControlLoad(void);
void Wireless_ControlSendCurrent(void);
void Wireless_ControlSendBootDefault(void);
void Wireless_ControlFormatCommand(const wireless_config_t *cfg, char *buf, uint16_t buf_size);

void Wireless_ControlStepFc(int8_t dir);
void Wireless_ControlStepMode(void);
void Wireless_ControlStepSdMv(int8_t dir);
void Wireless_ControlStepSdPhase(int8_t dir);
void Wireless_ControlStepAmDepth(int8_t dir);
void Wireless_ControlStepSmDelay(int8_t dir);
void Wireless_ControlStepSmPhase(int8_t dir);
void Wireless_ControlStepSmAtten(int8_t dir);
void Wireless_ControlStepSquareKHz(int8_t dir);
void Wireless_ControlStepOutA(int8_t dir);
void Wireless_ControlStepOutB(int8_t dir);
void Wireless_ControlToggleAutoSend(void);

const char *Wireless_ModeName(wireless_mode_t mode);
const char *Wireless_OutputName(wireless_output_t out);

#endif
