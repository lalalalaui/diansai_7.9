#include "./BSP/LVGL/lvgl_port.h"
#include "./BSP/LVGL/slave_ui.h"

#include "./BSP/LCD/lcd.h"
#include "./BSP/TOUCH/touch.h"
#include "./BSP/LED/led.h"
#include "stm32h7xx_hal.h"
#include <stdio.h>

#define LVGL_DRAW_BUF_LINES 20U
#define LVGL_DRAW_BUF_PIXELS (800U * LVGL_DRAW_BUF_LINES)

static lv_color_t g_lvgl_draw_buf[LVGL_DRAW_BUF_PIXELS] __attribute__((section(".dma_buffer"), aligned(4)));
static lv_display_t *g_lvgl_disp;
static lv_indev_t *g_lvgl_touch;

#if 0
/* ---- Extern variables from main.c ---- */
extern uint16_t g_adc_raw;
extern uint32_t g_adc_mv;
extern uint32_t g_adc_samples;
extern uint32_t g_ui_fps;

/* ---- UI object pointers for periodic update ---- */
static lv_obj_t *g_arc_adc;
static lv_obj_t *g_label_arc_val;
static lv_obj_t *g_label_adc_raw;
static lv_obj_t *g_label_adc_mv;
static lv_obj_t *g_label_samples;
static lv_obj_t *g_label_fps;
static lv_obj_t *g_label_uptime;
static lv_obj_t *g_label_touch;
static lv_obj_t *g_label_status;
static lv_obj_t *g_label_counter;
static lv_obj_t *g_label_led_state;
static lv_obj_t *g_slider;

static int32_t g_counter = 0;
static uint8_t g_led_on = 0;
static const char *g_status_msg = "Ready";

/* ---- Color palette ---- */
#define C_BG        lv_color_hex(0x161A20)
#define C_PANEL     lv_color_hex(0x1E2834)
#define C_ACCENT    lv_color_hex(0x00C8C8)
#define C_ACCENT2   lv_color_hex(0xFFA040)
#define C_TEXT      lv_color_hex(0xFFFFFF)
#define C_MUTED     lv_color_hex(0x8899AA)
#define C_GREEN     lv_color_hex(0x2ECC71)
#define C_RED       lv_color_hex(0xE74C3C)
#define C_YELLOW    lv_color_hex(0xF1C40F)

/* ---- Helpers ---- */
static lv_obj_t *make_panel(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h)
{
    lv_obj_t *p = lv_obj_create(parent);
    lv_obj_set_pos(p, x, y);
    lv_obj_set_size(p, w, h);
    lv_obj_set_style_bg_color(p, C_PANEL, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_set_style_pad_all(p, 0, 0);
    lv_obj_set_style_radius(p, 6, 0);
    lv_obj_set_scrollbar_mode(p, LV_SCROLLBAR_MODE_OFF);
    return p;
}

static lv_obj_t *make_label(lv_obj_t *parent, const char *text, lv_color_t color,
                             int32_t x, int32_t y, const lv_font_t *font)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_color(l, color, 0);
    lv_obj_set_pos(l, x, y);
    if (font) lv_obj_set_style_text_font(l, font, 0);
    return l;
}

static lv_obj_t *make_btn(lv_obj_t *parent, const char *text, int32_t x, int32_t y, int32_t w, int32_t h)
{
    lv_obj_t *b = lv_button_create(parent);
    lv_obj_set_pos(b, x, y);
    lv_obj_set_size(b, w, h);
    lv_obj_set_style_bg_color(b, C_ACCENT, 0);
    lv_obj_set_style_radius(b, 4, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);

    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_color(l, lv_color_hex(0x0A0E12), 0);
    lv_obj_center(l);
    return b;
}

/* ---- Button event handlers ---- */
static void btn_led_cb(lv_event_t *e)
{
    (void)e;
    g_led_on = !g_led_on;
    if (g_led_on)
        LED0(0);   /* LED on (active low) */
    else
        LED0(1);   /* LED off */
    lv_label_set_text(g_label_led_state, g_led_on ? "ON" : "OFF");
    lv_obj_set_style_text_color(g_label_led_state, g_led_on ? C_GREEN : C_RED, 0);
    g_status_msg = g_led_on ? "LED turned ON" : "LED turned OFF";
    lv_label_set_text(g_label_status, g_status_msg);
}

static void btn_counter_inc_cb(lv_event_t *e)
{
    (void)e;
    g_counter++;
    char buf[16];
    snprintf(buf, sizeof(buf), "%ld", (long)g_counter);
    lv_label_set_text(g_label_counter, buf);
    g_status_msg = "Counter incremented";
    lv_label_set_text(g_label_status, g_status_msg);
}

static void btn_counter_dec_cb(lv_event_t *e)
{
    (void)e;
    g_counter--;
    char buf[16];
    snprintf(buf, sizeof(buf), "%ld", (long)g_counter);
    lv_label_set_text(g_label_counter, buf);
    g_status_msg = "Counter decremented";
    lv_label_set_text(g_label_status, g_status_msg);
}

static void btn_counter_reset_cb(lv_event_t *e)
{
    (void)e;
    g_counter = 0;
    lv_label_set_text(g_label_counter, "0");
    g_status_msg = "Counter reset to zero";
    lv_label_set_text(g_label_status, g_status_msg);
}

static void slider_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int32_t val = lv_slider_get_value(slider);
    char buf[32];
    snprintf(buf, sizeof(buf), "Slider: %ld%%", (long)val);
    g_status_msg = buf;
    lv_label_set_text(g_label_status, g_status_msg);
}

/* ---- Periodic UI update timer (100ms) ---- */
static void ui_update_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    char buf[48];
    uint32_t now = lv_tick_get();
    uint32_t sec = now / 1000U;
    uint32_t h = sec / 3600U;
    uint32_t m = (sec % 3600U) / 60U;
    uint32_t s = sec % 60U;

    /* ADC gauge */
    int32_t mv = (int32_t)g_adc_mv;
    if (mv > 3300) mv = 3300;
    lv_arc_set_value(g_arc_adc, mv);
    snprintf(buf, sizeof(buf), "%ld mV", (long)mv);
    lv_label_set_text(g_label_arc_val, buf);

    /* ADC numbers */
    snprintf(buf, sizeof(buf), "%u", g_adc_raw);
    lv_label_set_text(g_label_adc_raw, buf);

    snprintf(buf, sizeof(buf), "%lu mV", (unsigned long)g_adc_mv);
    lv_label_set_text(g_label_adc_mv, buf);

    snprintf(buf, sizeof(buf), "%lu", (unsigned long)g_adc_samples);
    lv_label_set_text(g_label_samples, buf);

    /* FPS */
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)g_ui_fps);
    lv_label_set_text(g_label_fps, buf);

    /* Touch */
    if (tp_dev.sta & TP_PRES_DOWN)
    {
        snprintf(buf, sizeof(buf), "X:%u  Y:%u", tp_dev.x[0], tp_dev.y[0]);
    }
    else
    {
        snprintf(buf, sizeof(buf), "X:---  Y:---");
    }
    lv_label_set_text(g_label_touch, buf);

    /* Uptime */
    snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", (unsigned long)h, (unsigned long)m, (unsigned long)s);
    lv_label_set_text(g_label_uptime, buf);
}
#endif

/* ========== LVGL Tick, Flush, Touch ========== */

static uint32_t lvgl_tick_get_cb(void)
{
    return HAL_GetTick();
}

static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    int32_t x1 = area->x1, y1 = area->y1, x2 = area->x2, y2 = area->y2;
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= lcddev.width)  x2 = lcddev.width - 1;
    if (y2 >= lcddev.height) y2 = lcddev.height - 1;
    if (x1 <= x2 && y1 <= y2)
        lcd_color_fill((uint16_t)x1, (uint16_t)y1, (uint16_t)x2, (uint16_t)y2, (uint16_t *)px_map);
    lv_display_flush_ready(disp);
}

static void lvgl_touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    static lv_point_t last_point;
    uint8_t pressed = 0;
    (void)indev;
    tp_dev.scan(0);
    if (tp_dev.touchtype & 0x80)
        pressed = (tp_dev.sta & 0x01) ? 1U : 0U;
    else
        pressed = (tp_dev.sta & TP_PRES_DOWN) ? 1U : 0U;
    if (pressed && tp_dev.x[0] < lcddev.width && tp_dev.y[0] < lcddev.height) {
        last_point.x = tp_dev.x[0];
        last_point.y = tp_dev.y[0];
    }
    data->point = last_point;
    data->state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

/* ========== Display & Touch Init ========== */

void lvgl_port_init(void)
{
    lcd_display_dir(1);  /* 横屏 */
    if (lcddev.dir & 0x01) tp_dev.touchtype |= 0x01;
    else tp_dev.touchtype &= ~0x01;

    lv_init();
    lv_tick_set_cb(lvgl_tick_get_cb);

    g_lvgl_disp = lv_display_create(lcddev.width, lcddev.height);
    lv_display_set_color_format(g_lvgl_disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(g_lvgl_disp, lvgl_flush_cb);
    lv_display_set_buffers(g_lvgl_disp, g_lvgl_draw_buf, NULL, sizeof(g_lvgl_draw_buf),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_default(g_lvgl_disp);

    g_lvgl_touch = lv_indev_create();
    lv_indev_set_type(g_lvgl_touch, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(g_lvgl_touch, lvgl_touch_read_cb);
}

/* ========== Dashboard UI ========== */

void lvgl_port_demo_create(void)
{
    slave_ui_create(lv_screen_active());
#if 0
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, C_BG, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);

    int32_t sw = lcddev.width;   /* display width (landscape) */
    int32_t sh = lcddev.height;  /* display height */

    /* ==================== TOP BAR ==================== */
    lv_obj_t *top_bar = make_panel(screen, 0, 0, sw, 42);
    lv_obj_set_style_radius(top_bar, 0, 0);

    make_label(top_bar, "H743 TOUCH DASHBOARD", C_TEXT, 16, 9, &lv_font_montserrat_18);
    make_label(top_bar, "FPS:", C_MUTED, sw - 128, 13, &lv_font_montserrat_14);
    g_label_fps = make_label(top_bar, "--", C_GREEN, sw - 80, 13, &lv_font_montserrat_14);

    /* ==================== LEFT: ADC Arc Gauge ==================== */
    int32_t left_w = 240;
    int32_t left_x = 6;
    int32_t top_y = 48;
    int32_t main_h = sh - top_y - 34;

    lv_obj_t *left_panel = make_panel(screen, left_x, top_y, left_w, main_h);

    /* Title */
    make_label(left_panel, "ADC GAUGE", C_ACCENT, 8, 8, &lv_font_montserrat_14);

    /* Arc (range 0–3300 mV) */
    g_arc_adc = lv_arc_create(left_panel);
    lv_obj_set_size(g_arc_adc, 190, 190);
    lv_obj_align(g_arc_adc, LV_ALIGN_CENTER, 0, -16);
    lv_arc_set_range(g_arc_adc, 0, 3300);
    lv_arc_set_bg_angles(g_arc_adc, 135, 45);
    lv_arc_set_rotation(g_arc_adc, 135);
    lv_arc_set_value(g_arc_adc, 0);
    lv_obj_set_style_arc_width(g_arc_adc, 12, LV_PART_MAIN);
    lv_obj_set_style_arc_width(g_arc_adc, 12, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(g_arc_adc, lv_color_hex(0x2A3440), LV_PART_MAIN);
    lv_obj_set_style_arc_color(g_arc_adc, C_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(g_arc_adc, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(g_arc_adc, LV_OPA_TRANSP, 0);
    lv_obj_remove_style(g_arc_adc, NULL, LV_PART_KNOB);  /* remove knob */

    /* Arc center label */
    g_label_arc_val = make_label(left_panel, "0 mV", C_TEXT, 0, 0, &lv_font_montserrat_18);
    lv_obj_align(g_label_arc_val, LV_ALIGN_CENTER, 0, -16);

    /* Min / Max labels */
    make_label(left_panel, "0", C_MUTED, 28, 165, &lv_font_montserrat_14);
    make_label(left_panel, "3300mV", C_MUTED, 175, 165, &lv_font_montserrat_14);

    /* ==================== MIDDLE: ADC Values + Touch ==================== */
    int32_t mid_x = left_x + left_w + 6;
    int32_t mid_w = 240;

    lv_obj_t *mid_panel = make_panel(screen, mid_x, top_y, mid_w, main_h);

    make_label(mid_panel, "ADC VALUES", C_ACCENT, 8, 8, &lv_font_montserrat_14);

    /* ADC Raw */
    make_label(mid_panel, "RAW:", C_MUTED, 12, 40, &lv_font_montserrat_14);
    g_label_adc_raw = make_label(mid_panel, "--", C_TEXT, 70, 40, &lv_font_montserrat_14);

    /* ADC mV */
    make_label(mid_panel, "Voltage:", C_MUTED, 12, 68, &lv_font_montserrat_14);
    g_label_adc_mv = make_label(mid_panel, "--", C_TEXT, 85, 68, &lv_font_montserrat_14);

    /* ADC Samples */
    make_label(mid_panel, "Samples:", C_MUTED, 12, 96, &lv_font_montserrat_14);
    g_label_samples = make_label(mid_panel, "--", C_TEXT, 85, 96, &lv_font_montserrat_14);

    /* ---- Touch Coordinates ---- */
    lv_obj_t *touch_line = lv_obj_create(mid_panel);
    lv_obj_set_pos(touch_line, 8, 128);
    lv_obj_set_size(touch_line, mid_w - 16, 1);
    lv_obj_set_style_bg_color(touch_line, lv_color_hex(0x2A3440), 0);
    lv_obj_set_style_border_width(touch_line, 0, 0);

    make_label(mid_panel, "TOUCH", C_ACCENT2, 8, 138, &lv_font_montserrat_14);
    g_label_touch = make_label(mid_panel, "X:---  Y:---", C_TEXT, 12, 164, &lv_font_montserrat_14);

    /* ==================== RIGHT: Controls ==================== */
    int32_t right_x = mid_x + mid_w + 6;
    int32_t right_w = sw - right_x - 6;

    lv_obj_t *right_panel = make_panel(screen, right_x, top_y, right_w, main_h);

    make_label(right_panel, "CONTROLS", C_ACCENT, 8, 8, &lv_font_montserrat_14);

    /* ---- LED Toggle ---- */
    make_label(right_panel, "LED:", C_MUTED, 12, 40, &lv_font_montserrat_14);

    lv_obj_t *btn_led = make_btn(right_panel, "TOGGLE", 62, 36, 100, 32);
    lv_obj_add_event_cb(btn_led, btn_led_cb, LV_EVENT_CLICKED, NULL);

    g_label_led_state = make_label(right_panel, "OFF", C_RED, 172, 42, &lv_font_montserrat_14);

    /* ---- Counter ---- */
    lv_obj_t *sep1 = lv_obj_create(right_panel);
    lv_obj_set_pos(sep1, 8, 80);
    lv_obj_set_size(sep1, right_w - 16, 1);
    lv_obj_set_style_bg_color(sep1, lv_color_hex(0x2A3440), 0);
    lv_obj_set_style_border_width(sep1, 0, 0);

    make_label(right_panel, "COUNTER", C_ACCENT2, 8, 90, &lv_font_montserrat_14);

    lv_obj_t *btn_dec = make_btn(right_panel, "-", 12, 120, 44, 32);
    lv_obj_add_event_cb(btn_dec, btn_counter_dec_cb, LV_EVENT_CLICKED, NULL);

    g_label_counter = make_label(right_panel, "0", C_TEXT, 0, 120, &lv_font_montserrat_18);
    lv_obj_align(g_label_counter, LV_ALIGN_TOP_MID, 0, 122);

    lv_obj_t *btn_inc = make_btn(right_panel, "+", right_w - 56, 120, 44, 32);
    lv_obj_add_event_cb(btn_inc, btn_counter_inc_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_reset = make_btn(right_panel, "RESET", 0, 162, 80, 28);
    lv_obj_align(btn_reset, LV_ALIGN_TOP_MID, 0, 162);
    lv_obj_add_event_cb(btn_reset, btn_counter_reset_cb, LV_EVENT_CLICKED, NULL);

    /* ---- Slider ---- */
    lv_obj_t *sep2 = lv_obj_create(right_panel);
    lv_obj_set_pos(sep2, 8, 200);
    lv_obj_set_size(sep2, right_w - 16, 1);
    lv_obj_set_style_bg_color(sep2, lv_color_hex(0x2A3440), 0);
    lv_obj_set_style_border_width(sep2, 0, 0);

    make_label(right_panel, "SLIDER", C_MUTED, 8, 210, &lv_font_montserrat_14);
    g_slider = lv_slider_create(right_panel);
    lv_obj_set_size(g_slider, right_w - 24, 16);
    lv_obj_set_pos(g_slider, 12, 234);
    lv_slider_set_range(g_slider, 0, 100);
    lv_slider_set_value(g_slider, 50, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(g_slider, lv_color_hex(0x2A3440), LV_PART_MAIN);
    lv_obj_set_style_bg_color(g_slider, C_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(g_slider, C_TEXT, LV_PART_KNOB);
    lv_obj_add_event_cb(g_slider, slider_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* ---- Status message ---- */
    make_label(right_panel, "STATUS", C_MUTED, 8, 260, &lv_font_montserrat_14);
    g_label_status = make_label(right_panel, g_status_msg, C_GREEN, 12, 282, &lv_font_montserrat_14);

    /* ==================== BOTTOM BAR ==================== */
    lv_obj_t *bottom_bar = make_panel(screen, 0, sh - 30, sw, 30);
    lv_obj_set_style_radius(bottom_bar, 0, 0);

    /* Uptime — left */
    lv_obj_t *lbl = make_label(bottom_bar, "Uptime:", C_MUTED, 12, 7, &lv_font_montserrat_14);
    g_label_uptime = make_label(bottom_bar, "00:00:00", C_TEXT, 80, 7, &lv_font_montserrat_14);

    /* Info — center and right */
    lv_obj_t *info = make_label(bottom_bar, "STM32H743 | LTDC RGB565 | LVGL v9", C_MUTED, 0, 0, &lv_font_montserrat_14);
    lv_obj_align(info, LV_ALIGN_CENTER, 0, 0);
    (void)lbl;

    /* ==================== Periodic update timer ==================== */
    lv_timer_create(ui_update_timer_cb, 100, NULL);
#endif
}
