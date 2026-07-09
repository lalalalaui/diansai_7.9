#include "./BSP/LVGL/lvgl_port.h"

#include "./BSP/LCD/lcd.h"
#include "./BSP/LVGL/slave_ui.h"
#include "./BSP/TOUCH/touch.h"
#include "stm32h7xx_hal.h"

#define LVGL_DRAW_BUF_LINES 20U
#define LVGL_DRAW_BUF_PIXELS (800U * LVGL_DRAW_BUF_LINES)

static lv_color_t g_lvgl_draw_buf[LVGL_DRAW_BUF_PIXELS] __attribute__((section(".dma_buffer"), aligned(4)));
static lv_display_t *g_lvgl_disp;
static lv_indev_t *g_lvgl_touch;

static uint32_t lvgl_tick_get_cb(void)
{
    return HAL_GetTick();
}

static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    int32_t x1 = area->x1;
    int32_t y1 = area->y1;
    int32_t x2 = area->x2;
    int32_t y2 = area->y2;

    if (x1 < 0) {
        x1 = 0;
    }
    if (y1 < 0) {
        y1 = 0;
    }
    if (x2 >= lcddev.width) {
        x2 = lcddev.width - 1;
    }
    if (y2 >= lcddev.height) {
        y2 = lcddev.height - 1;
    }

    if (x1 <= x2 && y1 <= y2) {
        lcd_color_fill((uint16_t)x1,
                       (uint16_t)y1,
                       (uint16_t)x2,
                       (uint16_t)y2,
                       (uint16_t *)px_map);
    }

    lv_display_flush_ready(disp);
}

static void lvgl_touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    static lv_point_t last_point;
    uint8_t pressed;

    (void)indev;

    tp_dev.scan(0);
    if (tp_dev.touchtype & 0x80) {
        pressed = (tp_dev.sta & 0x01) ? 1U : 0U;
    } else {
        pressed = (tp_dev.sta & TP_PRES_DOWN) ? 1U : 0U;
    }

    if (pressed && tp_dev.x[0] < lcddev.width && tp_dev.y[0] < lcddev.height) {
        last_point.x = tp_dev.x[0];
        last_point.y = tp_dev.y[0];
    }

    data->point = last_point;
    data->state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

void lvgl_port_init(void)
{
    lcd_display_dir(1);
    if (lcddev.dir & 0x01) {
        tp_dev.touchtype |= 0x01;
    } else {
        tp_dev.touchtype &= (uint8_t)~0x01;
    }

    lv_init();
    lv_tick_set_cb(lvgl_tick_get_cb);

    g_lvgl_disp = lv_display_create(lcddev.width, lcddev.height);
    lv_display_set_color_format(g_lvgl_disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(g_lvgl_disp, lvgl_flush_cb);
    lv_display_set_buffers(g_lvgl_disp,
                           g_lvgl_draw_buf,
                           NULL,
                           sizeof(g_lvgl_draw_buf),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_default(g_lvgl_disp);

    g_lvgl_touch = lv_indev_create();
    lv_indev_set_type(g_lvgl_touch, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(g_lvgl_touch, lvgl_touch_read_cb);
}

void lvgl_port_demo_create(void)
{
    lv_obj_t *screen = lv_screen_active();

    slave_ui_create(screen);
}
