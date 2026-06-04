/**
 ****************************************************************************************************
 * @file        main.c
 * @brief       Touch screen dashboard demo for STM32H743.
 ****************************************************************************************************
 */

#include <stdio.h>
#include <string.h>

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/SDRAM/sdram.h"
#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/KEY/key.h"
#include "./BSP/MPU/mpu.h"
#include "./BSP/TOUCH/touch.h"
#include "./BSP/ADC/adc.h"
#include "./BSP/LVGL/lvgl_port.h"

#define RGB565(r, g, b)   (uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xF8) << 3) | (((b) & 0xF8) >> 3))

#define ADC_DMA_BUF_SIZE  50U
#define ADC_DMA_BUF_BYTES (((ADC_DMA_BUF_SIZE * sizeof(uint16_t)) + 31U) & ~31U)
#define ADC_VREF_MV       3300U
#define ADC_FULL_SCALE    65535U
#define UI_CARD_COUNT     8U

#define UI_BG             RGB565(22, 26, 32)
#define UI_PANEL          RGB565(34, 42, 52)
#define UI_PANEL_2        RGB565(44, 54, 66)
#define UI_ACCENT         RGB565(0, 180, 180)
#define UI_ACCENT_2       RGB565(255, 178, 42)
#define UI_TEXT           WHITE
#define UI_MUTED          RGB565(168, 180, 190)
#define UI_DRAW_BG        RGB565(248, 248, 244)
#define UI_GRID           RGB565(222, 226, 226)
#define UI_DANGER         RGB565(232, 82, 74)
#define UI_OK             RGB565(46, 190, 112)

typedef struct
{
    uint16_t top_h;
    uint16_t metrics_y;
    uint16_t metrics_h;
    uint16_t draw_x;
    uint16_t draw_y;
    uint16_t draw_w;
    uint16_t draw_h;
    uint16_t bottom_y;
    uint16_t clear_x;
    uint16_t clear_y;
    uint16_t clear_w;
    uint16_t clear_h;
    uint16_t cal_x;
    uint16_t cal_y;
    uint16_t cal_w;
    uint16_t cal_h;
} ui_layout_t;

static ui_layout_t g_ui;
static uint32_t g_fps = 0;
static uint32_t g_frame_count = 0;
static uint32_t g_last_fps_tick = 0;
static uint32_t g_last_panel_tick = 0;
static uint32_t g_touch_events = 0;
uint16_t g_adc_raw = 0;
uint32_t g_adc_mv = 0;
uint32_t g_adc_samples = 0;
uint32_t g_ui_fps = 0;
static uint8_t g_led_div = 0;
static uint16_t g_adc_dma_buf[ADC_DMA_BUF_SIZE] __attribute__((section(".dma_buffer"), aligned(32)));

static const uint16_t POINT_COLOR_TBL[10] =
{
    RED, GREEN, BLUE, BROWN, YELLOW, MAGENTA, CYAN, LIGHTBLUE, BRRED, GRAY
};

static void ui_layout_calc(void);
static void ui_draw_page(void);
static void ui_draw_draw_area(void);
static void ui_update_metrics(uint8_t points, uint16_t x, uint16_t y, uint8_t key);
static void adc_dma_service(void);
static uint16_t ui_metric_cols(void);
static void ui_button(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const char *text, uint16_t bg, uint16_t fg);
static void ui_card(uint8_t index, const char *label);
static void ui_card_value(uint8_t index, const char *value, uint16_t color);
static void lcd_draw_bline(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t size, uint16_t color);
static uint8_t ui_in_rect(uint16_t x, uint16_t y, uint16_t rx, uint16_t ry, uint16_t rw, uint16_t rh);
static uint8_t ui_in_draw_area(uint16_t x, uint16_t y);
static uint8_t touch_points_count(uint8_t maxp);
static void touch_dashboard(void) __attribute__((unused));

static void ui_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    uint32_t ex;
    uint32_t ey;

    if (x >= lcddev.width || y >= lcddev.height || w == 0 || h == 0)
    {
        return;
    }

    ex = (uint32_t)x + w - 1;
    ey = (uint32_t)y + h - 1;

    if (ex >= lcddev.width)
    {
        ex = lcddev.width - 1;
    }

    if (ey >= lcddev.height)
    {
        ey = lcddev.height - 1;
    }

    lcd_fill(x, y, (uint16_t)ex, (uint16_t)ey, color);
}

static void ui_text(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t size, const char *text, uint16_t color)
{
    if (x >= lcddev.width || y >= lcddev.height)
    {
        return;
    }

    lcd_show_string(x, y, w, h, size, (char *)text, color);
}

static void ui_layout_calc(void)
{
    uint16_t margin = 8;
    uint16_t bottom_h = 24;
    uint16_t cols = ui_metric_cols();
    uint16_t rows = (UI_CARD_COUNT + cols - 1U) / cols;
    uint16_t card_h = (lcddev.height < 320) ? 34 : 42;

    g_ui.top_h = 44;
    g_ui.metrics_y = g_ui.top_h + 8;
    g_ui.metrics_h = rows * card_h + (rows - 1U) * 6U;
    g_ui.draw_x = margin;
    g_ui.draw_y = g_ui.metrics_y + g_ui.metrics_h + 8;
    g_ui.bottom_y = (lcddev.height > bottom_h) ? (lcddev.height - bottom_h) : 0;
    g_ui.draw_w = (lcddev.width > margin * 2) ? (lcddev.width - margin * 2) : lcddev.width;

    if (g_ui.bottom_y > g_ui.draw_y + 8)
    {
        g_ui.draw_h = g_ui.bottom_y - g_ui.draw_y - 8;
    }
    else
    {
        g_ui.draw_h = (lcddev.height > g_ui.draw_y + 4) ? (lcddev.height - g_ui.draw_y - 4) : 1;
    }

    g_ui.clear_w = (lcddev.width < 360) ? 54 : 72;
    g_ui.clear_h = 30;
    g_ui.clear_x = lcddev.width - g_ui.clear_w - 8;
    g_ui.clear_y = 7;
    g_ui.cal_w = g_ui.clear_w;
    g_ui.cal_h = g_ui.clear_h;
    g_ui.cal_x = (g_ui.clear_x > g_ui.cal_w + 8) ? (g_ui.clear_x - g_ui.cal_w - 8) : 8;
    g_ui.cal_y = g_ui.clear_y;
}

static uint16_t ui_metric_cols(void)
{
    if (lcddev.width >= 360)
    {
        return 4;
    }

    return 2;
}

static void ui_button(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const char *text, uint16_t bg, uint16_t fg)
{
    uint16_t text_x = x + 8;
    uint16_t text_y = y + ((h > 16) ? ((h - 16) / 2) : 0);

    ui_fill_rect(x, y, w, h, bg);
    lcd_draw_rectangle(x, y, x + w - 1, y + h - 1, UI_TEXT);
    ui_text(text_x, text_y, w - 12, 16, 16, text, fg);
}

static void ui_card(uint8_t index, const char *label)
{
    uint16_t cols = ui_metric_cols();
    uint16_t rows = (UI_CARD_COUNT + cols - 1U) / cols;
    uint16_t gap = 6;
    uint16_t x0 = 8;
    uint16_t y0 = g_ui.metrics_y;
    uint16_t card_w = (lcddev.width - 16 - gap * (cols - 1)) / cols;
    uint16_t card_h = (g_ui.metrics_h - gap * (rows - 1U)) / rows;
    uint16_t col = index % cols;
    uint16_t row = index / cols;
    uint16_t x = x0 + col * (card_w + gap);
    uint16_t y = y0 + row * (card_h + gap);

    ui_fill_rect(x, y, card_w, card_h, UI_PANEL);
    lcd_draw_rectangle(x, y, x + card_w - 1, y + card_h - 1, UI_PANEL_2);
    ui_text(x + 8, y + 5, card_w - 16, 12, 12, label, UI_MUTED);
}

static void ui_card_value(uint8_t index, const char *value, uint16_t color)
{
    uint16_t cols = ui_metric_cols();
    uint16_t rows = (UI_CARD_COUNT + cols - 1U) / cols;
    uint16_t gap = 6;
    uint16_t x0 = 8;
    uint16_t y0 = g_ui.metrics_y;
    uint16_t card_w = (lcddev.width - 16 - gap * (cols - 1)) / cols;
    uint16_t card_h = (g_ui.metrics_h - gap * (rows - 1U)) / rows;
    uint16_t col = index % cols;
    uint16_t row = index / cols;
    uint16_t x = x0 + col * (card_w + gap);
    uint16_t y = y0 + row * (card_h + gap);
    uint16_t value_y = y + ((card_h > 42) ? 30 : 20);
    uint16_t value_h = (card_h > 42) ? 18 : 14;
    uint8_t font = (card_h > 42) ? 16 : 12;

    ui_fill_rect(x + 7, value_y, card_w - 14, value_h, UI_PANEL);
    ui_text(x + 8, value_y, card_w - 16, value_h, font, value, color);
}

static void ui_draw_draw_area(void)
{
    uint16_t i;
    uint16_t gx;
    uint16_t gy;

    ui_fill_rect(g_ui.draw_x, g_ui.draw_y, g_ui.draw_w, g_ui.draw_h, UI_DRAW_BG);
    lcd_draw_rectangle(g_ui.draw_x, g_ui.draw_y,
                       g_ui.draw_x + g_ui.draw_w - 1,
                       g_ui.draw_y + g_ui.draw_h - 1,
                       UI_ACCENT);

    for (i = 1; i < 4; i++)
    {
        gx = g_ui.draw_x + (g_ui.draw_w * i) / 4;
        lcd_draw_line(gx, g_ui.draw_y + 1, gx, g_ui.draw_y + g_ui.draw_h - 2, UI_GRID);
    }

    for (i = 1; i < 3; i++)
    {
        gy = g_ui.draw_y + (g_ui.draw_h * i) / 3;
        lcd_draw_hline(g_ui.draw_x + 1, gy, g_ui.draw_w - 2, UI_GRID);
    }

    ui_text(g_ui.draw_x + 8, g_ui.draw_y + 6, g_ui.draw_w - 16, 16, 16, "TOUCH TRACE AREA", GRAY);
}

static void ui_draw_page(void)
{
    char info[64];

    ui_layout_calc();
    lcd_clear(UI_BG);

    ui_fill_rect(0, 0, lcddev.width, g_ui.top_h, UI_PANEL);
    ui_text(10, 9, lcddev.width / 2, 24, 24, "H743 TOUCH DASHBOARD", UI_TEXT);

    snprintf(info, sizeof(info), "%ux%u  LCD:0x%04X", lcddev.width, lcddev.height, lcddev.id);
    ui_text(10, 31, lcddev.width / 2, 12, 12, info, UI_MUTED);

    ui_button(g_ui.clear_x, g_ui.clear_y, g_ui.clear_w, g_ui.clear_h, "CLR", UI_DANGER, WHITE);
    ui_button(g_ui.cal_x, g_ui.cal_y, g_ui.cal_w, g_ui.cal_h, "CAL", UI_ACCENT_2, BLACK);

    ui_card(0, "FPS");
    ui_card(1, "TOUCH");
    ui_card(2, "POINTS");
    ui_card(3, "XY");
    ui_card(4, "EVENTS");
    ui_card(5, "UPTIME");
    ui_card(6, "ADC RAW");
    ui_card(7, "ADC VOLT");

    ui_draw_draw_area();

    ui_fill_rect(0, g_ui.bottom_y, lcddev.width, lcddev.height - g_ui.bottom_y, UI_PANEL);
    ui_text(8, g_ui.bottom_y + 6, lcddev.width - 16, 12, 12,
            "Tap CLR to clear, CAL or KEY0 to calibrate resistive touch.", UI_MUTED);
}

static void ui_update_metrics(uint8_t points, uint16_t x, uint16_t y, uint8_t key)
{
    char buf[48];
    uint32_t now = HAL_GetTick();
    const char *type = (tp_dev.touchtype & 0x80) ? "CAP" : "RES";

    snprintf(buf, sizeof(buf), "%lu", (unsigned long)g_fps);
    ui_card_value(0, buf, UI_OK);

    snprintf(buf, sizeof(buf), "%s %s", type, points ? "DOWN" : "UP");
    ui_card_value(1, buf, points ? UI_ACCENT_2 : UI_MUTED);

    snprintf(buf, sizeof(buf), "%u  K:%u", points, key);
    ui_card_value(2, buf, UI_TEXT);

    if (points)
    {
        snprintf(buf, sizeof(buf), "%u,%u", x, y);
    }
    else
    {
        snprintf(buf, sizeof(buf), "--,--");
    }
    ui_card_value(3, buf, UI_TEXT);

    snprintf(buf, sizeof(buf), "%lu", (unsigned long)g_touch_events);
    ui_card_value(4, buf, UI_ACCENT);

    snprintf(buf, sizeof(buf), "%lus", (unsigned long)(now / 1000U));
    ui_card_value(5, buf, UI_TEXT);

    snprintf(buf, sizeof(buf), "%u", g_adc_raw);
    ui_card_value(6, buf, UI_ACCENT_2);

    snprintf(buf, sizeof(buf), "%lu.%03luV",
             (unsigned long)(g_adc_mv / 1000U),
             (unsigned long)(g_adc_mv % 1000U));
    ui_card_value(7, buf, UI_OK);
}

static void adc_dma_service(void)
{
    uint16_t i;
    uint32_t sum = 0;

    if (g_adc_dma_sta == 0)
    {
        return;
    }

    SCB_InvalidateDCache_by_Addr((uint32_t *)g_adc_dma_buf, ADC_DMA_BUF_BYTES);

    for (i = 0; i < ADC_DMA_BUF_SIZE; i++)
    {
        sum += g_adc_dma_buf[i];
    }

    g_adc_raw = (uint16_t)(sum / ADC_DMA_BUF_SIZE);
    g_adc_mv = (uint32_t)(((uint64_t)g_adc_raw * ADC_VREF_MV + (ADC_FULL_SCALE / 2U)) / ADC_FULL_SCALE);
    g_adc_samples++;

    g_adc_dma_sta = 0;
    adc_dma_enable(ADC_DMA_BUF_SIZE);
}

static uint8_t ui_in_rect(uint16_t x, uint16_t y, uint16_t rx, uint16_t ry, uint16_t rw, uint16_t rh)
{
    return (x >= rx && y >= ry && x < (uint16_t)(rx + rw) && y < (uint16_t)(ry + rh));
}

static uint8_t ui_in_draw_area(uint16_t x, uint16_t y)
{
    return ui_in_rect(x, y, g_ui.draw_x, g_ui.draw_y, g_ui.draw_w, g_ui.draw_h);
}

static uint8_t touch_points_count(uint8_t maxp)
{
    uint8_t t;
    uint8_t count = 0;

    if (tp_dev.touchtype & 0x80)
    {
        for (t = 0; t < maxp; t++)
        {
            if (tp_dev.sta & (1 << t))
            {
                count++;
            }
        }
    }
    else if (tp_dev.sta & TP_PRES_DOWN)
    {
        count = 1;
    }

    return count;
}

static void clear_trace(uint16_t lastpos[10][2])
{
    uint8_t i;

    ui_draw_draw_area();
    for (i = 0; i < 10; i++)
    {
        lastpos[i][0] = 0xFFFF;
        lastpos[i][1] = 0xFFFF;
    }
}

static void draw_touch_point(uint8_t index, uint16_t x, uint16_t y, uint16_t lastpos[10][2])
{
    uint16_t color = POINT_COLOR_TBL[index % 10];

    if (!ui_in_draw_area(x, y))
    {
        return;
    }

    if (lastpos[index][0] == 0xFFFF || !ui_in_draw_area(lastpos[index][0], lastpos[index][1]))
    {
        lastpos[index][0] = x;
        lastpos[index][1] = y;
    }

    lcd_draw_bline(lastpos[index][0], lastpos[index][1], x, y, 2, color);
    lcd_fill_circle(x, y, 4, color);
    lastpos[index][0] = x;
    lastpos[index][1] = y;
}

static void touch_dashboard(void)
{
    uint8_t t;
    uint8_t key;
    uint8_t maxp = (lcddev.id == 0x1018) ? 10 : 5;
    uint8_t points = 0;
    uint8_t was_down = 0;
    uint16_t first_x = 0;
    uint16_t first_y = 0;
    uint16_t lastpos[10][2];

    memset(lastpos, 0xFF, sizeof(lastpos));
    ui_draw_page();
    g_last_fps_tick = HAL_GetTick();
    g_last_panel_tick = g_last_fps_tick;
    ui_update_metrics(0, 0, 0, 0);

    while (1)
    {
        adc_dma_service();

        key = key_scan(0);
        tp_dev.scan(0);
        points = touch_points_count(maxp);

        if (points)
        {
            first_x = tp_dev.x[0];
            first_y = tp_dev.y[0];

            if (!was_down)
            {
                g_touch_events++;
                was_down = 1;
            }

            if (ui_in_rect(first_x, first_y, g_ui.clear_x, g_ui.clear_y, g_ui.clear_w, g_ui.clear_h))
            {
                clear_trace(lastpos);
            }
            else if (ui_in_rect(first_x, first_y, g_ui.cal_x, g_ui.cal_y, g_ui.cal_w, g_ui.cal_h)
                     && (tp_dev.touchtype & 0x80) == 0)
            {
                lcd_clear(WHITE);
                tp_adjust();
                tp_save_adjust_data();
                ui_draw_page();
                clear_trace(lastpos);
            }
            else if (tp_dev.touchtype & 0x80)
            {
                for (t = 0; t < maxp; t++)
                {
                    if (tp_dev.sta & (1 << t))
                    {
                        if (tp_dev.x[t] < lcddev.width && tp_dev.y[t] < lcddev.height)
                        {
                            draw_touch_point(t, tp_dev.x[t], tp_dev.y[t], lastpos);
                        }
                    }
                    else
                    {
                        lastpos[t][0] = 0xFFFF;
                    }
                }
            }
            else
            {
                if (first_x < lcddev.width && first_y < lcddev.height)
                {
                    draw_touch_point(0, first_x, first_y, lastpos);
                }
            }
        }
        else
        {
            was_down = 0;
            for (t = 0; t < 10; t++)
            {
                lastpos[t][0] = 0xFFFF;
            }
        }

        if (key == KEY0_PRES && (tp_dev.touchtype & 0x80) == 0)
        {
            lcd_clear(WHITE);
            tp_adjust();
            tp_save_adjust_data();
            ui_draw_page();
            clear_trace(lastpos);
        }

        g_frame_count++;
        if (HAL_GetTick() - g_last_fps_tick >= 1000U)
        {
            g_fps = g_frame_count;
            g_frame_count = 0;
            g_last_fps_tick = HAL_GetTick();
        }

        if (HAL_GetTick() - g_last_panel_tick >= 120U)
        {
            ui_update_metrics(points, first_x, first_y, key);
            g_last_panel_tick = HAL_GetTick();
        }

        if (++g_led_div >= 20)
        {
            LED0_TOGGLE();
            g_led_div = 0;
        }

        delay_ms(2);
    }
}

static void lcd_draw_bline(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t size, uint16_t color)
{
    uint16_t t;
    int xerr = 0;
    int yerr = 0;
    int delta_x;
    int delta_y;
    int distance;
    int incx;
    int incy;
    int row;
    int col;

    if (x1 < size || x2 < size || y1 < size || y2 < size)
    {
        return;
    }

    delta_x = x2 - x1;
    delta_y = y2 - y1;
    row = x1;
    col = y1;

    if (delta_x > 0)
    {
        incx = 1;
    }
    else if (delta_x == 0)
    {
        incx = 0;
    }
    else
    {
        incx = -1;
        delta_x = -delta_x;
    }

    if (delta_y > 0)
    {
        incy = 1;
    }
    else if (delta_y == 0)
    {
        incy = 0;
    }
    else
    {
        incy = -1;
        delta_y = -delta_y;
    }

    distance = (delta_x > delta_y) ? delta_x : delta_y;

    for (t = 0; t <= distance + 1; t++)
    {
        lcd_fill_circle(row, col, size, color);
        xerr += delta_x;
        yerr += delta_y;

        if (xerr > distance)
        {
            xerr -= distance;
            row += incx;
        }

        if (yerr > distance)
        {
            yerr -= distance;
            col += incy;
        }
    }
}

int main(void)
{
    sys_cache_enable();
    HAL_Init();
    sys_stm32_clock_init(160, 5, 2, 4);
    delay_init(400);
    usart_init(115200);
    mpu_memory_protection();
    led_init();
    sdram_init();
    lcd_init();
    key_init();
    tp_dev.init();
    adc_dma_init((uint32_t)&ADC1->DR, (uint32_t)g_adc_dma_buf);
    adc_dma_enable(ADC_DMA_BUF_SIZE);
    lvgl_port_init();
    lvgl_port_demo_create();

    while (1)
    {
        static uint32_t frame_cnt = 0;
        static uint32_t last_fps_tick = 0;
        uint32_t now = HAL_GetTick();

        adc_dma_service();
        lv_timer_handler();

        /* FPS counting */
        frame_cnt++;
        if (now - last_fps_tick >= 1000U)
        {
            g_ui_fps = frame_cnt;
            frame_cnt = 0;
            last_fps_tick = now;
        }

        delay_ms(5);
    }
}
