/**
 ****************************************************************************************************
 * @file        main.c
 * @brief       STM32H743 application entry with LCD/LVGL driver kept blank.
 ****************************************************************************************************
 */

#include <stdint.h>

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
#include "./BSP/FPGA/fpga_display.h"
#include "./BSP/LVGL/lvgl_port.h"
#include "./BSP/WIRELESS/wireless_control.h"

#define ADC_DMA_BUF_SIZE  80U
#define ADC_VREF_MV       3300U
#define ADC_FULL_SCALE    65535U

uint16_t g_adc_raw = 0;
uint32_t g_adc_mv = 0;
uint32_t g_adc_samples = 0;
uint32_t g_ui_fps = 0;

static uint16_t g_adc_dma_buf[ADC_DMA_BUF_SIZE] __attribute__((section(".dma_buffer"), aligned(32)));
static uint16_t g_adc_proc_buf[ADC_DMA_BUF_SIZE] __attribute__((aligned(32)));

static void adc_dma_service(void)
{
    uint16_t count;
    uint32_t sum = 0;

    count = adc_dma_read_snapshot(g_adc_proc_buf, ADC_DMA_BUF_SIZE);

    if (count == 0U)
    {
        return;
    }

    for (uint16_t i = 0; i < count; i++)
    {
        sum += g_adc_proc_buf[i];
    }

    g_adc_raw = (uint16_t)(sum / count);
    g_adc_mv = (uint32_t)(((uint64_t)g_adc_raw * ADC_VREF_MV + (ADC_FULL_SCALE / 2U)) / ADC_FULL_SCALE);
    g_adc_samples++;
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
    Wireless_ControlInit();
    lvgl_port_init();
    lvgl_port_demo_create();
    FPGA_DisplayInit();
    Wireless_ControlSendBootDefault();

    while (1)
    {
        static uint32_t frame_cnt = 0;
        static uint32_t last_fps_tick = 0;
        uint32_t now = HAL_GetTick();

        adc_dma_service();
        FPGA_DisplayPollUsart1();
        lv_timer_handler();

        frame_cnt++;
        if (now - last_fps_tick >= 1000U)
        {
            g_ui_fps = frame_cnt;
            frame_cnt = 0;
            last_fps_tick = now;
        }

        delay_ms(1);
    }
}
