#include "./BSP/SMS/sms_frame.h"
#include "./BSP/SMS/crc8.h"

#include <stddef.h>
#include <string.h>

static sms_frame_t g_sms_frame;

static void sms_frame_reset_rx(sms_frame_t *ctx)
{
    ctx->state = SMS_STATE_SEARCH_PREAMBLE;
    ctx->preamble_count = 0U;
    ctx->bit_count = 0U;
    ctx->current_byte = 0U;
    ctx->address = 0U;
    ctx->length = 0U;
    ctx->payload_index = 0U;
    ctx->rx_crc = 0U;
    ctx->calc_crc = CRC8_INIT;
}

static bool sms_frame_char_valid(uint8_t ch)
{
    if (ch >= 'A' && ch <= 'Z')
    {
        return true;
    }

    if (ch >= '0' && ch <= '9')
    {
        return true;
    }

    switch (ch)
    {
        case ' ':
        case '.':
        case ',':
        case '?':
        case '!':
        case '-':
        case '/':
            return true;

        default:
            return false;
    }
}

static bool sms_frame_address_valid(const sms_frame_t *ctx)
{
    return (ctx->address == SMS_GROUP_ADDRESS || ctx->address == ctx->station_id);
}

static bool sms_frame_get_byte(sms_frame_t *ctx, uint8_t bit, uint8_t *out_byte)
{
    bit &= 0x01U;

    if (bit != 0U)
    {
        ctx->current_byte |= (uint8_t)(1U << ctx->bit_count);
    }

    ctx->bit_count++;
    if (ctx->bit_count < 8U)
    {
        return false;
    }

    *out_byte = ctx->current_byte;
    ctx->current_byte = 0U;
    ctx->bit_count = 0U;
    return true;
}

static bool sms_frame_accept(sms_frame_t *ctx, char *out_text, uint8_t *out_len)
{
    /* save debug info before reset can clear it */
    ctx->last_fail_address = ctx->address;
    ctx->last_fail_length  = ctx->length;
    ctx->last_fail_rx_crc  = ctx->rx_crc;
    ctx->last_fail_calc_crc = ctx->calc_crc;

    if (!sms_frame_address_valid(ctx))
    {
        return false;
    }

    if (ctx->rx_crc != ctx->calc_crc)
    {
        return false;
    }

    if (out_text != NULL)
    {
        memcpy(out_text, ctx->payload, ctx->length);
        out_text[ctx->length] = '\0';
    }

    if (out_len != NULL)
    {
        *out_len = ctx->length;
    }

    ctx->last_address = ctx->address;
    return true;
}

void SMS_FrameContextInit(sms_frame_t *ctx, uint8_t station_id)
{
    if (ctx == NULL)
    {
        return;
    }

    if (station_id > SMS_MAX_STATION_ID)
    {
        station_id = 0U;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->station_id = station_id;
    sms_frame_reset_rx(ctx);
}

sms_push_result_t SMS_FrameContextPushBit(sms_frame_t *ctx, uint8_t bit, char *out_text, uint8_t *out_len)
{
    uint8_t byte;

    if (ctx == NULL)
    {
        return SMS_PUSH_NONE;
    }

    if (!sms_frame_get_byte(ctx, bit, &byte))
    {
        return SMS_PUSH_NONE;
    }

    switch (ctx->state)
    {
        case SMS_STATE_SEARCH_PREAMBLE:
            if (byte == SMS_PREAMBLE_BYTE)
            {
                if (ctx->preamble_count < SMS_PREAMBLE_COUNT)
                {
                    ctx->preamble_count++;
                }

                if (ctx->preamble_count >= SMS_PREAMBLE_COUNT)
                {
                    ctx->state = SMS_STATE_SEARCH_FLAG;
                    return SMS_PUSH_PREAMBLE_OK;
                }
            }
            else
            {
                ctx->preamble_count = 0U;
            }
            break;

        case SMS_STATE_SEARCH_FLAG:
            if (byte == SMS_FLAG_BYTE)
            {
                ctx->state = SMS_STATE_READ_ADDRESS;
                ctx->calc_crc = CRC8_INIT;
                return SMS_PUSH_FLAG_OK;
            }
            else if (byte == SMS_PREAMBLE_BYTE)
            {
                ctx->preamble_count = SMS_PREAMBLE_COUNT;
            }
            else
            {
                sms_frame_reset_rx(ctx);
            }
            break;

        case SMS_STATE_READ_ADDRESS:
            ctx->address = byte;
            ctx->calc_crc = CRC8_Update(ctx->calc_crc, byte);
            ctx->state = SMS_STATE_READ_LENGTH;
            break;

        case SMS_STATE_READ_LENGTH:
            ctx->length = byte;
            ctx->payload_index = 0U;
            ctx->calc_crc = CRC8_Update(ctx->calc_crc, byte);

            if (ctx->length > SMS_MAX_PAYLOAD_LEN)
            {
                sms_frame_reset_rx(ctx);
                return SMS_PUSH_LEN_INVALID;
            }
            else if (ctx->length == 0U)
            {
                ctx->state = SMS_STATE_READ_CRC;
            }
            else
            {
                ctx->state = SMS_STATE_READ_PAYLOAD;
            }
            break;

        case SMS_STATE_READ_PAYLOAD:
            if (!sms_frame_char_valid(byte))
            {
                sms_frame_reset_rx(ctx);
                return SMS_PUSH_CHAR_INVALID;
            }

            ctx->payload[ctx->payload_index++] = byte;
            ctx->calc_crc = CRC8_Update(ctx->calc_crc, byte);

            if (ctx->payload_index >= ctx->length)
            {
                ctx->state = SMS_STATE_READ_CRC;
            }
            break;

        case SMS_STATE_READ_CRC:
            ctx->rx_crc = byte;
            ctx->state = SMS_STATE_CHECK_FRAME;
            if (sms_frame_accept(ctx, out_text, out_len))
            {
                sms_frame_reset_rx(ctx);
                return SMS_PUSH_FRAME_OK;
            }
            else
            {
                sms_push_result_t ret = sms_frame_address_valid(ctx)
                                        ? SMS_PUSH_CRC_FAIL
                                        : SMS_PUSH_ADDR_DROP;
                sms_frame_reset_rx(ctx);
                return ret;
            }

        case SMS_STATE_CHECK_FRAME:
        default:
            sms_frame_reset_rx(ctx);
            break;
    }

    return SMS_PUSH_NONE;
}

uint8_t SMS_FrameContextGetLastAddress(const sms_frame_t *ctx)
{
    if (ctx == NULL)
    {
        return 0U;
    }

    return ctx->last_address;
}

void SMS_FrameInit(uint8_t station_id)
{
    SMS_FrameContextInit(&g_sms_frame, station_id);
}

bool SMS_FramePushBit(uint8_t bit, char *out_text, uint8_t *out_len)
{
    sms_push_result_t r = SMS_FrameContextPushBit(&g_sms_frame, bit, out_text, out_len);
    return (r == SMS_PUSH_FRAME_OK);
}

uint8_t SMS_FrameGetLastAddress(void)
{
    return SMS_FrameContextGetLastAddress(&g_sms_frame);
}

uint8_t SMS_FrameContextGetState(const sms_frame_t *ctx)
{
    if (ctx == NULL) { return 0U; }
    return (uint8_t)ctx->state;
}

uint8_t SMS_FrameContextGetAddress(const sms_frame_t *ctx)
{
    if (ctx == NULL) { return 0U; }
    return ctx->address;
}

uint8_t SMS_FrameContextGetLength(const sms_frame_t *ctx)
{
    if (ctx == NULL) { return 0U; }
    return ctx->length;
}

uint8_t SMS_FrameContextGetRxCrc(const sms_frame_t *ctx)
{
    if (ctx == NULL) { return 0U; }
    return ctx->rx_crc;
}

uint8_t SMS_FrameContextGetCalcCrc(const sms_frame_t *ctx)
{
    if (ctx == NULL) { return 0U; }
    return ctx->calc_crc;
}

uint8_t SMS_FrameContextGetLastFailAddress(const sms_frame_t *ctx)
{
    if (ctx == NULL) { return 0U; }
    return ctx->last_fail_address;
}

uint8_t SMS_FrameContextGetLastFailLength(const sms_frame_t *ctx)
{
    if (ctx == NULL) { return 0U; }
    return ctx->last_fail_length;
}

uint8_t SMS_FrameContextGetLastFailRxCrc(const sms_frame_t *ctx)
{
    if (ctx == NULL) { return 0U; }
    return ctx->last_fail_rx_crc;
}

uint8_t SMS_FrameContextGetLastFailCalcCrc(const sms_frame_t *ctx)
{
    if (ctx == NULL) { return 0U; }
    return ctx->last_fail_calc_crc;
}
