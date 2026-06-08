#ifndef __SMS_FRAME_H
#define __SMS_FRAME_H

#include <stdbool.h>
#include <stdint.h>

#define SMS_MAX_PAYLOAD_LEN       96U
#define SMS_PREAMBLE_BYTE         0xAAU
#define SMS_PREAMBLE_COUNT        4U
#define SMS_FLAG_BYTE             0x7EU
#define SMS_GROUP_ADDRESS         0xFFU
#define SMS_MAX_STATION_ID        7U

typedef enum
{
    SMS_STATE_SEARCH_PREAMBLE = 0,
    SMS_STATE_SEARCH_FLAG,
    SMS_STATE_READ_ADDRESS,
    SMS_STATE_READ_LENGTH,
    SMS_STATE_READ_PAYLOAD,
    SMS_STATE_READ_CRC,
    SMS_STATE_CHECK_FRAME
} sms_frame_state_t;

typedef enum
{
    SMS_PUSH_NONE = 0,
    SMS_PUSH_PREAMBLE_OK,
    SMS_PUSH_FLAG_OK,
    SMS_PUSH_FRAME_OK,
    SMS_PUSH_CRC_FAIL,
    SMS_PUSH_ADDR_DROP,
    SMS_PUSH_CHAR_INVALID,
    SMS_PUSH_LEN_INVALID
} sms_push_result_t;

typedef struct
{
    sms_frame_state_t state;
    uint8_t station_id;
    uint8_t preamble_count;
    uint8_t bit_count;
    uint8_t current_byte;
    uint8_t address;
    uint8_t length;
    uint8_t payload_index;
    uint8_t payload[SMS_MAX_PAYLOAD_LEN];
    uint8_t rx_crc;
    uint8_t calc_crc;
    uint8_t last_address;
    uint8_t last_fail_address;
    uint8_t last_fail_length;
    uint8_t last_fail_rx_crc;
    uint8_t last_fail_calc_crc;
} sms_frame_t;

void SMS_FrameInit(uint8_t station_id);
sms_push_result_t SMS_FrameContextPushBit(sms_frame_t *ctx, uint8_t bit, char *out_text, uint8_t *out_len);
bool SMS_FramePushBit(uint8_t bit, char *out_text, uint8_t *out_len);
uint8_t SMS_FrameGetLastAddress(void);
uint8_t SMS_FrameContextGetLastAddress(const sms_frame_t *ctx);

uint8_t SMS_FrameContextGetState(const sms_frame_t *ctx);
uint8_t SMS_FrameContextGetAddress(const sms_frame_t *ctx);
uint8_t SMS_FrameContextGetLength(const sms_frame_t *ctx);
uint8_t SMS_FrameContextGetRxCrc(const sms_frame_t *ctx);
uint8_t SMS_FrameContextGetCalcCrc(const sms_frame_t *ctx);
uint8_t SMS_FrameContextGetLastFailAddress(const sms_frame_t *ctx);
uint8_t SMS_FrameContextGetLastFailLength(const sms_frame_t *ctx);
uint8_t SMS_FrameContextGetLastFailRxCrc(const sms_frame_t *ctx);
uint8_t SMS_FrameContextGetLastFailCalcCrc(const sms_frame_t *ctx);

void SMS_FrameContextInit(sms_frame_t *ctx, uint8_t station_id);

#endif
