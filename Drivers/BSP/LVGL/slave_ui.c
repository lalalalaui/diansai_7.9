#include "./BSP/LVGL/slave_ui.h"

#include "./BSP/WIRELESS/wireless_control.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LV_FONT_DECLARE(ui_font_cn_18);
#define UI_FONT &ui_font_cn_18

typedef enum
{
    UI_ACT_FC_DEC = 0,
    UI_ACT_FC_INC,
    UI_ACT_MODE,
    UI_ACT_SD_MV_DEC,
    UI_ACT_SD_MV_INC,
    UI_ACT_SD_PHASE_DEC,
    UI_ACT_SD_PHASE_INC,
    UI_ACT_AM_DEPTH_DEC,
    UI_ACT_AM_DEPTH_INC,
    UI_ACT_SM_DELAY_DEC,
    UI_ACT_SM_DELAY_INC,
    UI_ACT_SM_PHASE_DEC,
    UI_ACT_SM_PHASE_INC,
    UI_ACT_SM_ATTEN_DEC,
    UI_ACT_SM_ATTEN_INC,
    UI_ACT_SQUARE_DEC,
    UI_ACT_SQUARE_INC,
    UI_ACT_OUT_A_DEC,
    UI_ACT_OUT_A_INC,
    UI_ACT_OUT_B_DEC,
    UI_ACT_OUT_B_INC,
    UI_ACT_RESEND,
    UI_ACT_SAVE,
    UI_ACT_DEFAULTS,
    UI_ACT_AUTO_SEND
} ui_action_t;

typedef enum
{
    UI_FIELD_NONE = 0,
    UI_FIELD_FC,
    UI_FIELD_SD_MV,
    UI_FIELD_SD_PHASE,
    UI_FIELD_AM_DEPTH,
    UI_FIELD_SM_DELAY,
    UI_FIELD_SM_PHASE,
    UI_FIELD_SM_ATTEN,
    UI_FIELD_SQUARE
} ui_field_t;

static uint8_t g_station_id = 3U;
static bool g_group_enabled = true;
static void (*g_command_callback)(const char *command) = NULL;

static lv_obj_t *g_value_fc;
static lv_obj_t *g_value_mode;
static lv_obj_t *g_value_sd_mv;
static lv_obj_t *g_value_sd_phase;
static lv_obj_t *g_value_am_depth;
static lv_obj_t *g_value_sm_delay;
static lv_obj_t *g_value_sm_phase;
static lv_obj_t *g_value_sm_atten;
static lv_obj_t *g_value_square;
static lv_obj_t *g_value_out_a;
static lv_obj_t *g_value_out_b;
static lv_obj_t *g_status_label;
static lv_obj_t *g_cmd_label;
static lv_obj_t *g_auto_label;
static lv_obj_t *g_input_overlay;
static lv_obj_t *g_input_value_label;
static ui_field_t g_input_field = UI_FIELD_NONE;
static char g_input_buf[8];

static lv_style_t g_style_root;
static lv_style_t g_style_panel;
static lv_style_t g_style_btn;
static lv_style_t g_style_btn_primary;
static lv_style_t g_style_label;

static lv_obj_t *ui_make_label(lv_obj_t *parent, const char *text);

static const char *g_keypad_map[] = {
    "1", "2", "3", "\n",
    "4", "5", "6", "\n",
    "7", "8", "9", "\n",
    "CLR", "0", "DEL", "\n",
    "Cancel", "OK", ""
};

static const char *ui_field_name(ui_field_t field)
{
    switch (field)
    {
        case UI_FIELD_FC:
            return "\xE8\xBD\xBD\xE6\xB3\xA2\x20\x4D\x48\x7A";
        case UI_FIELD_SD_MV:
            return "\xE5\xB9\x85\xE5\xBA\xA6\x20\x6D\x56\x72\x6D\x73";
        case UI_FIELD_SD_PHASE:
            return "\xE5\x88\x9D\xE7\x9B\xB8\x20\x64\x65\x67";
        case UI_FIELD_AM_DEPTH:
            return "\xE8\xB0\x83\xE5\x88\xB6\xE5\xBA\xA6\x20\x25";
        case UI_FIELD_SM_DELAY:
            return "\xE6\x97\xB6\xE5\xBB\xB6\x20\x6E\x73";
        case UI_FIELD_SM_PHASE:
            return "\xE5\x88\x9D\xE7\x9B\xB8\x20\x64\x65\x67";
        case UI_FIELD_SM_ATTEN:
            return "\xE8\xA1\xB0\xE5\x87\x8F\x20\x64\x42";
        case UI_FIELD_SQUARE:
            return "\xE6\x96\xB9\xE6\xB3\xA2\x20\x6B\x48\x7A";
        default:
            return "Value";
    }
}

static uint32_t ui_field_current_value(ui_field_t field)
{
    const wireless_config_t *cfg = Wireless_ControlGet();

    switch (field)
    {
        case UI_FIELD_FC:
            return cfg->fc_mhz;
        case UI_FIELD_SD_MV:
            return cfg->sd_mv;
        case UI_FIELD_SD_PHASE:
            return cfg->sd_phase_deg;
        case UI_FIELD_AM_DEPTH:
            return cfg->am_depth_pct;
        case UI_FIELD_SM_DELAY:
            return cfg->sm_delay_ns;
        case UI_FIELD_SM_PHASE:
            return cfg->sm_phase_deg;
        case UI_FIELD_SM_ATTEN:
            return cfg->sm_atten_db;
        case UI_FIELD_SQUARE:
            return cfg->square_khz;
        default:
            return 0U;
    }
}

static void ui_field_set_value(ui_field_t field, uint32_t value)
{
    wireless_config_t cfg = *Wireless_ControlGet();

    switch (field)
    {
        case UI_FIELD_FC:
            cfg.fc_mhz = (uint8_t)value;
            break;
        case UI_FIELD_SD_MV:
            cfg.sd_mv = (uint16_t)value;
            break;
        case UI_FIELD_SD_PHASE:
            cfg.sd_phase_deg = (uint16_t)value;
            break;
        case UI_FIELD_AM_DEPTH:
            cfg.am_depth_pct = (uint8_t)value;
            break;
        case UI_FIELD_SM_DELAY:
            cfg.sm_delay_ns = (uint16_t)value;
            break;
        case UI_FIELD_SM_PHASE:
            cfg.sm_phase_deg = (uint16_t)value;
            break;
        case UI_FIELD_SM_ATTEN:
            cfg.sm_atten_db = (uint8_t)value;
            break;
        case UI_FIELD_SQUARE:
            cfg.square_khz = (uint16_t)value;
            break;
        default:
            return;
    }

    Wireless_ControlSet(&cfg);
}

static void ui_set_text(lv_obj_t *obj, const char *text)
{
    if (obj != NULL)
    {
        lv_label_set_text(obj, text != NULL ? text : "");
    }
}

static const char *ui_auto_text(uint8_t auto_send)
{
    return auto_send ? "\xE4\xB8\x8A\xE7\x94\xB5\xE8\x87\xAA\xE5\x8A\xA8\xE5\x8F\x91\xE9\x80\x81\xEF\xBC\x9A\xE5\xBC\x80" :
                       "\xE4\xB8\x8A\xE7\x94\xB5\xE8\x87\xAA\xE5\x8A\xA8\xE5\x8F\x91\xE9\x80\x81\xEF\xBC\x9A\xE5\x85\xB3";
}

static void ui_refresh(void)
{
    const wireless_config_t *cfg = Wireless_ControlGet();
    char buf[WIRELESS_CMD_MAX_LEN];

    snprintf(buf, sizeof(buf), "%u MHz", cfg->fc_mhz);
    ui_set_text(g_value_fc, buf);

    ui_set_text(g_value_mode, Wireless_ModeName(cfg->mode));

    snprintf(buf, sizeof(buf), "%u mVrms", cfg->sd_mv);
    ui_set_text(g_value_sd_mv, buf);

    snprintf(buf, sizeof(buf), "%u deg", cfg->sd_phase_deg);
    ui_set_text(g_value_sd_phase, buf);

    snprintf(buf, sizeof(buf), "%u %%", cfg->am_depth_pct);
    ui_set_text(g_value_am_depth, buf);

    snprintf(buf, sizeof(buf), "%u ns", cfg->sm_delay_ns);
    ui_set_text(g_value_sm_delay, buf);

    snprintf(buf, sizeof(buf), "%u deg", cfg->sm_phase_deg);
    ui_set_text(g_value_sm_phase, buf);

    snprintf(buf, sizeof(buf), "%u dB", cfg->sm_atten_db);
    ui_set_text(g_value_sm_atten, buf);

    snprintf(buf, sizeof(buf), "%u kHz", cfg->square_khz);
    ui_set_text(g_value_square, buf);

    ui_set_text(g_value_out_a, Wireless_OutputName(cfg->out_a));
    ui_set_text(g_value_out_b, Wireless_OutputName(cfg->out_b));
    ui_set_text(g_auto_label, ui_auto_text(cfg->auto_send));

    Wireless_ControlFormatCommand(cfg, buf, sizeof(buf));
    ui_set_text(g_cmd_label, buf);
}

static void ui_refresh_and_send(const char *status)
{
    ui_refresh();
    Wireless_ControlSendCurrent();
    ui_set_text(g_status_label, status);
}

static void ui_close_input(void)
{
    if (g_input_overlay != NULL)
    {
        lv_obj_delete(g_input_overlay);
        g_input_overlay = NULL;
        g_input_value_label = NULL;
        g_input_field = UI_FIELD_NONE;
        g_input_buf[0] = '\0';
    }
}

static void ui_input_refresh_label(void)
{
    if (g_input_value_label != NULL)
    {
        lv_label_set_text(g_input_value_label, g_input_buf[0] ? g_input_buf : "0");
    }
}

static void ui_input_event(lv_event_t *e)
{
    lv_obj_t *keypad = lv_event_get_target(e);
    uint32_t index;
    const char *text;

    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED)
    {
        return;
    }

    index = lv_buttonmatrix_get_selected_button(keypad);
    text = lv_buttonmatrix_get_button_text(keypad, index);
    if (text == NULL)
    {
        return;
    }

    if (strcmp(text, "Cancel") == 0)
    {
        ui_close_input();
    }
    else if (strcmp(text, "OK") == 0)
    {
        uint32_t value = (uint32_t)strtoul(g_input_buf, NULL, 10);
        ui_field_set_value(g_input_field, value);
        ui_close_input();
        ui_refresh_and_send("\xE6\x95\xB0\xE5\x80\xBC\xE5\xB7\xB2\xE5\x8F\x91\xE9\x80\x81");
    }
    else if (strcmp(text, "CLR") == 0)
    {
        g_input_buf[0] = '\0';
        ui_input_refresh_label();
    }
    else if (strcmp(text, "DEL") == 0)
    {
        size_t len = strlen(g_input_buf);
        if (len > 0U)
        {
            g_input_buf[len - 1U] = '\0';
        }
        ui_input_refresh_label();
    }
    else if (text[0] >= '0' && text[0] <= '9')
    {
        size_t len = strlen(g_input_buf);
        if (len < (sizeof(g_input_buf) - 1U))
        {
            g_input_buf[len] = text[0];
            g_input_buf[len + 1U] = '\0';
        }
        ui_input_refresh_label();
    }
}

static void ui_value_click_event(lv_event_t *e)
{
    ui_field_t field;
    lv_obj_t *panel;
    lv_obj_t *title;
    lv_obj_t *hint;
    lv_obj_t *keypad;

    if (lv_event_get_code(e) != LV_EVENT_CLICKED)
    {
        return;
    }

    field = (ui_field_t)(uintptr_t)lv_event_get_user_data(e);
    if (field == UI_FIELD_NONE)
    {
        return;
    }

    /* Temporary debug text: confirms the value button click event entered this handler. */
    ui_set_text(g_status_label, "VALUE CLICK");

    ui_close_input();
    g_input_field = field;
    snprintf(g_input_buf, sizeof(g_input_buf), "%lu", (unsigned long)ui_field_current_value(field));

    g_input_overlay = lv_obj_create(lv_layer_top());
    lv_obj_move_foreground(g_input_overlay);
    lv_obj_set_size(g_input_overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(g_input_overlay, 0, 0);
    lv_obj_set_style_bg_color(g_input_overlay, lv_color_hex(0xE5E7EB), 0);
    lv_obj_set_style_bg_opa(g_input_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(g_input_overlay, 0, 0);
    lv_obj_set_style_pad_all(g_input_overlay, 10, 0);
    lv_obj_clear_flag(g_input_overlay, LV_OBJ_FLAG_SCROLLABLE);

    panel = lv_obj_create(g_input_overlay);
    lv_obj_set_size(panel, 780, 130);
    lv_obj_set_pos(panel, 0, 0);
    lv_obj_set_style_radius(panel, 6, 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_border_color(panel, lv_color_hex(0xCBD5E1), 0);
    lv_obj_set_style_pad_all(panel, 12, 0);

    title = ui_make_label(panel, ui_field_name(field));
    lv_obj_set_pos(title, 10, 8);

    hint = ui_make_label(panel, "\xE8\xBE\x93\xE5\x85\xA5\xE6\x95\xB0\xE5\x80\xBC\xE5\x90\x8E\xE6\x8C\x89\x4F\x4B");
    lv_obj_set_pos(hint, 10, 36);
    lv_obj_set_style_text_color(hint, lv_color_hex(0x64748B), 0);

    g_input_value_label = ui_make_label(panel, g_input_buf);
    lv_obj_set_pos(g_input_value_label, 10, 70);
    lv_obj_set_size(g_input_value_label, 220, 44);
    lv_obj_set_style_text_align(g_input_value_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(g_input_value_label, UI_FONT, 0);

    keypad = lv_buttonmatrix_create(g_input_overlay);
    lv_buttonmatrix_set_map(keypad, g_keypad_map);
    lv_obj_set_size(keypad, 780, 310);
    lv_obj_set_pos(keypad, 0, 140);
    lv_obj_set_style_text_font(keypad, LV_FONT_DEFAULT, 0);
    lv_obj_set_style_border_width(keypad, 0, 0);
    lv_obj_set_style_radius(keypad, 0, 0);
    lv_obj_add_event_cb(keypad, ui_input_event, LV_EVENT_VALUE_CHANGED, NULL);
}

static void ui_action_event(lv_event_t *e)
{
    ui_action_t action;

    if (lv_event_get_code(e) != LV_EVENT_CLICKED)
    {
        return;
    }

    action = (ui_action_t)(uintptr_t)lv_event_get_user_data(e);

    switch (action)
    {
        case UI_ACT_FC_DEC:
            Wireless_ControlStepFc(-1);
            ui_refresh_and_send("\xE5\xB7\xB2\xE5\x8F\x91\xE9\x80\x81");
            return;
        case UI_ACT_FC_INC:
            Wireless_ControlStepFc(1);
            ui_refresh_and_send("\xE5\xB7\xB2\xE5\x8F\x91\xE9\x80\x81");
            return;
        case UI_ACT_MODE:
            Wireless_ControlStepMode();
            ui_refresh_and_send("\xE5\xB7\xB2\xE5\x8F\x91\xE9\x80\x81");
            return;
        case UI_ACT_SD_MV_DEC:
            Wireless_ControlStepSdMv(-1);
            ui_refresh_and_send("\xE5\xB7\xB2\xE5\x8F\x91\xE9\x80\x81");
            return;
        case UI_ACT_SD_MV_INC:
            Wireless_ControlStepSdMv(1);
            ui_refresh_and_send("\xE5\xB7\xB2\xE5\x8F\x91\xE9\x80\x81");
            return;
        case UI_ACT_SD_PHASE_DEC:
            Wireless_ControlStepSdPhase(-1);
            ui_refresh_and_send("\xE5\xB7\xB2\xE5\x8F\x91\xE9\x80\x81");
            return;
        case UI_ACT_SD_PHASE_INC:
            Wireless_ControlStepSdPhase(1);
            ui_refresh_and_send("\xE5\xB7\xB2\xE5\x8F\x91\xE9\x80\x81");
            return;
        case UI_ACT_AM_DEPTH_DEC:
            Wireless_ControlStepAmDepth(-1);
            ui_refresh_and_send("\xE5\xB7\xB2\xE5\x8F\x91\xE9\x80\x81");
            return;
        case UI_ACT_AM_DEPTH_INC:
            Wireless_ControlStepAmDepth(1);
            ui_refresh_and_send("\xE5\xB7\xB2\xE5\x8F\x91\xE9\x80\x81");
            return;
        case UI_ACT_SM_DELAY_DEC:
            Wireless_ControlStepSmDelay(-1);
            ui_refresh_and_send("\xE5\xB7\xB2\xE5\x8F\x91\xE9\x80\x81");
            return;
        case UI_ACT_SM_DELAY_INC:
            Wireless_ControlStepSmDelay(1);
            ui_refresh_and_send("\xE5\xB7\xB2\xE5\x8F\x91\xE9\x80\x81");
            return;
        case UI_ACT_SM_PHASE_DEC:
            Wireless_ControlStepSmPhase(-1);
            ui_refresh_and_send("\xE5\xB7\xB2\xE5\x8F\x91\xE9\x80\x81");
            return;
        case UI_ACT_SM_PHASE_INC:
            Wireless_ControlStepSmPhase(1);
            ui_refresh_and_send("\xE5\xB7\xB2\xE5\x8F\x91\xE9\x80\x81");
            return;
        case UI_ACT_SM_ATTEN_DEC:
            Wireless_ControlStepSmAtten(-1);
            ui_refresh_and_send("\xE5\xB7\xB2\xE5\x8F\x91\xE9\x80\x81");
            return;
        case UI_ACT_SM_ATTEN_INC:
            Wireless_ControlStepSmAtten(1);
            ui_refresh_and_send("\xE5\xB7\xB2\xE5\x8F\x91\xE9\x80\x81");
            return;
        case UI_ACT_SQUARE_DEC:
            Wireless_ControlStepSquareKHz(-1);
            ui_refresh_and_send("\xE5\xB7\xB2\xE5\x8F\x91\xE9\x80\x81");
            return;
        case UI_ACT_SQUARE_INC:
            Wireless_ControlStepSquareKHz(1);
            ui_refresh_and_send("\xE5\xB7\xB2\xE5\x8F\x91\xE9\x80\x81");
            return;
        case UI_ACT_OUT_A_DEC:
            Wireless_ControlStepOutA(-1);
            ui_refresh_and_send("\xE5\xB7\xB2\xE5\x8F\x91\xE9\x80\x81");
            return;
        case UI_ACT_OUT_A_INC:
            Wireless_ControlStepOutA(1);
            ui_refresh_and_send("\xE5\xB7\xB2\xE5\x8F\x91\xE9\x80\x81");
            return;
        case UI_ACT_OUT_B_DEC:
            Wireless_ControlStepOutB(-1);
            ui_refresh_and_send("\xE5\xB7\xB2\xE5\x8F\x91\xE9\x80\x81");
            return;
        case UI_ACT_OUT_B_INC:
            Wireless_ControlStepOutB(1);
            ui_refresh_and_send("\xE5\xB7\xB2\xE5\x8F\x91\xE9\x80\x81");
            return;
        case UI_ACT_RESEND:
            ui_refresh_and_send("\xE5\xBD\x93\xE5\x89\x8D\xE9\x85\x8D\xE7\xBD\xAE\xE5\xB7\xB2\xE9\x87\x8D\xE5\x8F\x91");
            return;
        case UI_ACT_SAVE:
            if (Wireless_ControlSave())
            {
                ui_set_text(g_status_label, "\xE5\xB7\xB2\xE4\xBF\x9D\xE5\xAD\x98");
            }
            else
            {
                ui_set_text(g_status_label, "\xE5\xAD\x98\xE5\x82\xA8\xE4\xB8\x8D\xE5\x8F\xAF\xE7\x94\xA8");
            }
            ui_refresh();
            return;
        case UI_ACT_DEFAULTS:
            Wireless_ControlResetDefaults();
            ui_refresh_and_send("\xE9\xBB\x98\xE8\xAE\xA4\xE5\x80\xBC\xE5\xB7\xB2\xE5\x8F\x91\xE9\x80\x81");
            return;
        case UI_ACT_AUTO_SEND:
            Wireless_ControlToggleAutoSend();
            (void)Wireless_ControlSave();
            ui_refresh();
            ui_set_text(g_status_label, "\xE4\xB8\x8A\xE7\x94\xB5\xE5\x8F\x91\xE9\x80\x81\xE5\xB7\xB2\xE5\x88\x87\xE6\x8D\xA2");
            return;
        default:
            break;
    }
}

static lv_obj_t *ui_make_label(lv_obj_t *parent, const char *text)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_add_style(label, &g_style_label, 0);
    lv_obj_set_style_text_font(label, UI_FONT, 0);
    lv_label_set_text(label, text);
    return label;
}

static lv_obj_t *ui_make_button(lv_obj_t *parent, const char *text, ui_action_t action, bool primary)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, primary ? 118 : 38, 32);
    lv_obj_add_style(btn, primary ? &g_style_btn_primary : &g_style_btn, 0);
    lv_obj_add_event_cb(btn, ui_action_event, LV_EVENT_CLICKED, (void *)(uintptr_t)action);

    lv_obj_t *label = lv_label_create(btn);
    lv_obj_set_style_text_font(label, UI_FONT, 0);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    return btn;
}

static lv_obj_t *ui_make_row(lv_obj_t *parent,
                             const char *name,
                             const char *unit_hint,
                             ui_action_t dec_action,
                             ui_action_t inc_action,
                             ui_field_t field)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), 40);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *name_label = ui_make_label(row, name);
    lv_obj_set_width(name_label, 76);

    ui_make_button(row, "-", dec_action, false);

    lv_obj_t *value_btn = lv_button_create(row);
    lv_obj_set_size(value_btn, 92, 32);
    lv_obj_add_style(value_btn, &g_style_btn, 0);
    lv_obj_set_style_shadow_width(value_btn, 0, 0);
    if (field != UI_FIELD_NONE)
    {
        lv_obj_add_event_cb(value_btn, ui_value_click_event, LV_EVENT_CLICKED, (void *)(uintptr_t)field);
    }

    lv_obj_t *value = ui_make_label(value_btn, "--");
    if (field != UI_FIELD_NONE)
    {
        lv_obj_add_flag(value, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(value, ui_value_click_event, LV_EVENT_CLICKED, (void *)(uintptr_t)field);
    }
    lv_obj_center(value);

    ui_make_button(row, "+", inc_action, false);

    lv_obj_t *hint = ui_make_label(row, unit_hint);
    lv_obj_set_width(hint, 96);
    lv_obj_set_style_text_color(hint, lv_color_hex(0x718096), 0);
    return value;
}

static lv_obj_t *ui_make_panel(lv_obj_t *parent, const char *title)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_add_style(panel, &g_style_panel, 0);
    lv_obj_set_size(panel, 380, 326);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *title_label = ui_make_label(panel, title);
    lv_obj_set_style_text_color(title_label, lv_color_hex(0x111827), 0);
    return panel;
}

static void ui_init_styles(void)
{
    lv_style_init(&g_style_root);
    lv_style_set_bg_color(&g_style_root, lv_color_hex(0xEEF2F7));
    lv_style_set_pad_all(&g_style_root, 12);

    lv_style_init(&g_style_panel);
    lv_style_set_bg_color(&g_style_panel, lv_color_hex(0xFFFFFF));
    lv_style_set_border_color(&g_style_panel, lv_color_hex(0xD8DEE9));
    lv_style_set_border_width(&g_style_panel, 1);
    lv_style_set_radius(&g_style_panel, 6);
    lv_style_set_pad_all(&g_style_panel, 12);
    lv_style_set_pad_row(&g_style_panel, 8);

    lv_style_init(&g_style_btn);
    lv_style_set_bg_color(&g_style_btn, lv_color_hex(0xE5E7EB));
    lv_style_set_text_color(&g_style_btn, lv_color_hex(0x111827));
    lv_style_set_radius(&g_style_btn, 4);
    lv_style_set_border_width(&g_style_btn, 0);

    lv_style_init(&g_style_btn_primary);
    lv_style_set_bg_color(&g_style_btn_primary, lv_color_hex(0x2563EB));
    lv_style_set_text_color(&g_style_btn_primary, lv_color_hex(0xFFFFFF));
    lv_style_set_radius(&g_style_btn_primary, 4);
    lv_style_set_border_width(&g_style_btn_primary, 0);

    lv_style_init(&g_style_label);
    lv_style_set_text_font(&g_style_label, UI_FONT);
    lv_style_set_text_color(&g_style_label, lv_color_hex(0x1F2937));
}

void slave_ui_create(lv_obj_t *parent)
{
    lv_obj_t *header;
    lv_obj_t *main_row;
    lv_obj_t *panel_left;
    lv_obj_t *panel_right;
    lv_obj_t *bottom;

    if (parent == NULL)
    {
        return;
    }

    ui_init_styles();

    lv_obj_clean(parent);
    lv_obj_add_style(parent, &g_style_root, 0);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    header = lv_obj_create(parent);
    lv_obj_set_size(header, LV_PCT(100), 48);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *title = ui_make_label(header, "\xE6\x97\xA0\xE7\xBA\xBF\xE4\xBF\xA1\xE5\x8F\xB7\xE6\xA8\xA1\xE6\x8B\x9F\xE7\xB3\xBB\xE7\xBB\x9F");
    lv_obj_set_style_text_color(title, lv_color_hex(0x0F172A), 0);

    g_status_label = ui_make_label(header, "\xE5\xB0\xB1\xE7\xBB\xAA");
    lv_obj_set_style_text_color(g_status_label, lv_color_hex(0x2563EB), 0);

    main_row = lv_obj_create(parent);
    lv_obj_set_size(main_row, LV_PCT(100), 326);
    lv_obj_set_style_border_width(main_row, 0, 0);
    lv_obj_set_style_bg_opa(main_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(main_row, 0, 0);
    lv_obj_set_flex_flow(main_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(main_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    panel_left = ui_make_panel(main_row, "\xE7\x9B\xB4\xE8\xBE\xBE\xE4\xBF\xA1\xE5\x8F\xB7\x20\x53\x44");
    g_value_fc = ui_make_row(panel_left, "\xE8\xBD\xBD\xE6\xB3\xA2", "30-40 MHz / 1", UI_ACT_FC_DEC, UI_ACT_FC_INC, UI_FIELD_FC);

    lv_obj_t *mode_row = lv_obj_create(panel_left);
    lv_obj_set_size(mode_row, LV_PCT(100), 40);
    lv_obj_set_style_border_width(mode_row, 0, 0);
    lv_obj_set_style_bg_opa(mode_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(mode_row, 0, 0);
    lv_obj_set_flex_flow(mode_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(mode_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *mode_name = ui_make_label(mode_row, "\xE4\xBF\xA1\xE5\x8F\xB7\xE6\xA8\xA1\xE5\xBC\x8F");
    lv_obj_set_width(mode_name, 76);
    ui_make_button(mode_row, "\xE5\x88\x87\xE6\x8D\xA2", UI_ACT_MODE, false);
    g_value_mode = ui_make_label(mode_row, "--");
    lv_obj_set_width(g_value_mode, 92);
    lv_obj_set_style_text_align(g_value_mode, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_t *mode_hint = ui_make_label(mode_row, "CW / AM");
    lv_obj_set_style_text_color(mode_hint, lv_color_hex(0x718096), 0);

    g_value_sd_mv = ui_make_row(panel_left, "\x53\x44\x20\xE5\xB9\x85\xE5\xBA\xA6", "100mV-1V / 100", UI_ACT_SD_MV_DEC, UI_ACT_SD_MV_INC, UI_FIELD_SD_MV);
    g_value_sd_phase = ui_make_row(panel_left, "\x53\x44\x20\xE5\x88\x9D\xE7\x9B\xB8", "0-330 deg / 30", UI_ACT_SD_PHASE_DEC, UI_ACT_SD_PHASE_INC, UI_FIELD_SD_PHASE);
    g_value_am_depth = ui_make_row(panel_left, "\x41\x4D\x20\xE8\xB0\x83\xE5\x88\xB6\xE5\xBA\xA6", "30%-90% / 10", UI_ACT_AM_DEPTH_DEC, UI_ACT_AM_DEPTH_INC, UI_FIELD_AM_DEPTH);

    panel_right = ui_make_panel(main_row, "\xE5\xA4\x9A\xE5\xBE\x84\xE4\xB8\x8E\xE8\xBE\x93\xE5\x87\xBA");
    g_value_sm_delay = ui_make_row(panel_right, "\x53\x4D\x20\xE6\x97\xB6\xE5\xBB\xB6", "50-200 ns / 30", UI_ACT_SM_DELAY_DEC, UI_ACT_SM_DELAY_INC, UI_FIELD_SM_DELAY);
    g_value_sm_phase = ui_make_row(panel_right, "\x53\x4D\x20\xE5\x88\x9D\xE7\x9B\xB8", "0-180 deg / 30", UI_ACT_SM_PHASE_DEC, UI_ACT_SM_PHASE_INC, UI_FIELD_SM_PHASE);
    g_value_sm_atten = ui_make_row(panel_right, "\x53\x4D\x20\xE8\xA1\xB0\xE5\x87\x8F", "0-20 dB / 2", UI_ACT_SM_ATTEN_DEC, UI_ACT_SM_ATTEN_INC, UI_FIELD_SM_ATTEN);
    g_value_square = ui_make_row(panel_right, "\xE6\x96\xB9\xE6\xB3\xA2", "100-5000 kHz", UI_ACT_SQUARE_DEC, UI_ACT_SQUARE_INC, UI_FIELD_SQUARE);
    g_value_out_a = ui_make_row(panel_right, "DAC A", "SD/SM/SOUT/DC/SQ/MOD", UI_ACT_OUT_A_DEC, UI_ACT_OUT_A_INC, UI_FIELD_NONE);
    g_value_out_b = ui_make_row(panel_right, "DAC B", "SD/SM/SOUT/DC/SQ/MOD", UI_ACT_OUT_B_DEC, UI_ACT_OUT_B_INC, UI_FIELD_NONE);

    bottom = lv_obj_create(parent);
    lv_obj_add_style(bottom, &g_style_panel, 0);
    lv_obj_set_size(bottom, LV_PCT(100), 70);
    lv_obj_set_flex_flow(bottom, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bottom, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    ui_make_button(bottom, "\xE9\x87\x8D\xE5\x8F\x91", UI_ACT_RESEND, true);
    ui_make_button(bottom, "\xE4\xBF\x9D\xE5\xAD\x98", UI_ACT_SAVE, true);
    ui_make_button(bottom, "\xE9\xBB\x98\xE8\xAE\xA4", UI_ACT_DEFAULTS, false);
    ui_make_button(bottom, "\xE8\x87\xAA\xE5\x8A\xA8", UI_ACT_AUTO_SEND, false);

    g_auto_label = ui_make_label(bottom, "--");
    lv_obj_set_width(g_auto_label, 136);

    g_cmd_label = ui_make_label(bottom, "");
    lv_obj_set_width(g_cmd_label, 310);
    lv_label_set_long_mode(g_cmd_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_color(g_cmd_label, lv_color_hex(0x475569), 0);

    ui_refresh();
}

void slave_ui_set_command_callback(void (*callback)(const char *command))
{
    g_command_callback = callback;
    Wireless_ControlSetSendCallback(callback);
}

void slave_ui_set_pynq_status(const char *status)
{
    ui_set_text(g_status_label, status);
}

void slave_ui_set_test_result(const char *result)
{
    ui_set_text(g_status_label, result);
}

void slave_ui_set_capture_state(const char *state)
{
    ui_set_text(g_status_label, state);
}

void slave_ui_append_log(const char *line)
{
    ui_set_text(g_status_label, line);
}

void slave_ui_set_rx_state(bool carrier_detected,
                           bool selected_call,
                           bool group_call,
                           int16_t rssi_dbm,
                           uint8_t af_level)
{
    (void)carrier_detected;
    (void)selected_call;
    (void)group_call;
    (void)rssi_dbm;
    (void)af_level;
}

void slave_ui_set_sms(const char *text, uint8_t sender_id, bool group_call)
{
    (void)sender_id;
    (void)group_call;
    ui_set_text(g_status_label, text);
}

void slave_ui_set_battery(uint16_t millivolts, uint8_t percent)
{
    (void)millivolts;
    (void)percent;
}

void slave_ui_set_waveform(const uint16_t *samples, uint16_t count)
{
    (void)samples;
    (void)count;
}

uint8_t slave_ui_get_station_id(void)
{
    return g_station_id;
}

bool slave_ui_group_enabled(void)
{
    return g_group_enabled;
}

void slave_ui_send_command(const char *command)
{
    if (g_command_callback != NULL)
    {
        g_command_callback(command);
    }
}
