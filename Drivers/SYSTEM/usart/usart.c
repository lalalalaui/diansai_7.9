/**
 ****************************************************************************************************
 * @file        usart.c
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.1
 * @date        2023-03-02
 * @brief       ���ڳ�ʼ������(һ���Ǵ���1)��֧��printf
 * @license     Copyright (c) 2020-2032, �������������ӿƼ����޹�˾
 ****************************************************************************************************
 * @attention
 *
 * ʵ��ƽ̨:����ԭ�� STM32������
 * ������Ƶ:www.yuanzige.com
 * ������̳:www.openedv.com
 * ��˾��ַ:www.alientek.com
 * ������?openedv.taobao.com
 *
 * �޸�˵��
 * V1.0 20220420
 * ��һ�η���
 * V1.1 20230607
 * �޸�SYS_SUPPORT_OS���ִ���, ����ͷ�ļ��ĳ�:"os.h"
 * ɾ��USART_UX_IRQHandler()�����ĳ�ʱ�������޸�HAL_UART_RxCpltCallback()
 *
 ****************************************************************************************************
 */

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"

#include <stdbool.h>
#include <string.h>


/* ���ʹ��os,����������ͷ�ļ�����. */
#if SYS_SUPPORT_OS
#include "os.h"   /* os ʹ�� */
#endif

/******************************************************************************************/
/* �������´���, ֧��printf����, ������Ҫѡ��use MicroLIB */

#if defined(__GNUC__) && !defined(__ARMCC_VERSION)
int fputc(int ch, FILE *f)
{
    (void)f;
    return ch;
}

int __io_putchar(int ch)
{
    return ch;
}
#else
#if (__ARMCC_VERSION >= 6010050)
__asm(".global __use_no_semihosting\n\t");
__asm(".global __ARM_use_no_argv \n\t");
#else
#pragma import(__use_no_semihosting)

struct __FILE
{
    int handle;
};
#endif

int _ttywrch(int ch)
{
    ch = ch;
    return ch;
}

void _sys_exit(int x)
{
    x = x;
}

char *_sys_command_string(char *cmd, int len)
{
    (void)cmd;
    (void)len;
    return NULL;
}

FILE __stdout;

int fputc(int ch, FILE *f)
{
    (void)f;
    return ch;
}
#endif
/******************************************************************************************/

#if USART_EN_RX     /* ���ʹ���˽���?*/

/* ���ջ���, ���USART_REC_LEN���ֽ�. */
uint8_t g_usart_rx_buf[USART_REC_LEN];

/*  ����״̬
 *  bit15��      ������ɱ��?
 *  bit14��      ���յ�0x0d
 *  bit13~0��    ���յ�����Ч�ֽ���Ŀ
*/
uint16_t g_usart_rx_sta = 0;

uint8_t g_rx_buffer[RXBUFFERSIZE];          /* HAL��ʹ�õĴ��ڽ��ջ��� */

UART_HandleTypeDef g_uart1_handle;          /* UART���?*/

static uint8_t s_usart_rx_line[USART_REC_LEN];
static uint16_t s_usart_rx_len = 0U;
static bool s_usart_rx_cr_seen = false;

static uint8_t s_usart_line_queue[USART_LINE_QUEUE_DEPTH][USART_REC_LEN];
static volatile uint16_t s_usart_line_len[USART_LINE_QUEUE_DEPTH];
static volatile uint8_t s_usart_line_head = 0U;
static volatile uint8_t s_usart_line_tail = 0U;
static volatile uint8_t s_usart_line_count = 0U;
static volatile uint32_t s_usart_line_overflow = 0U;

static void usart_queue_current_line(void)
{
    uint8_t head;

    if (s_usart_rx_len == 0U)
    {
        return;
    }

    if (s_usart_line_count >= USART_LINE_QUEUE_DEPTH)
    {
        s_usart_line_tail = (uint8_t)((s_usart_line_tail + 1U) % USART_LINE_QUEUE_DEPTH);
        s_usart_line_count--;
        s_usart_line_overflow++;
    }

    head = s_usart_line_head;
    memcpy(s_usart_line_queue[head], s_usart_rx_line, s_usart_rx_len);
    s_usart_line_len[head] = s_usart_rx_len;
    s_usart_line_head = (uint8_t)((s_usart_line_head + 1U) % USART_LINE_QUEUE_DEPTH);
    s_usart_line_count++;
}

uint16_t usart_read_line(uint8_t *buf, uint16_t buf_size)
{
    uint16_t len;
    uint8_t tail;

    if (buf == NULL || buf_size == 0U)
    {
        return 0U;
    }

    __disable_irq();
    if (s_usart_line_count == 0U)
    {
        __enable_irq();
        buf[0] = '\0';
        return 0U;
    }

    tail = s_usart_line_tail;
    len = s_usart_line_len[tail];
    if (len >= buf_size)
    {
        len = (uint16_t)(buf_size - 1U);
    }
    memcpy(buf, s_usart_line_queue[tail], len);
    buf[len] = '\0';

    s_usart_line_tail = (uint8_t)((s_usart_line_tail + 1U) % USART_LINE_QUEUE_DEPTH);
    s_usart_line_count--;
    __enable_irq();

    return len;
}


/**
 * @brief       ����X��ʼ������
 * @param       baudrate: ������, �����Լ���Ҫ���ò�����ֵ
 * @note        ע��: ����������ȷ��ʱ��Դ, ���򴮿ڲ����ʾͻ������쳣.
 *              �����USART��ʱ��Դ��sys_stm32_clock_init()�������Ѿ����ù���.
 * @retval      ��
 */
void usart_init(uint32_t baudrate)
{
    g_uart1_handle.Instance = USART_UX;                    /* USART1 */
    g_uart1_handle.Init.BaudRate = baudrate;               /* ������ */
    g_uart1_handle.Init.WordLength = UART_WORDLENGTH_8B;   /* �ֳ�Ϊ8λ���ݸ�ʽ */
    g_uart1_handle.Init.StopBits = UART_STOPBITS_1;        /* һ��ֹͣλ */
    g_uart1_handle.Init.Parity = UART_PARITY_NONE;         /* ����żУ��λ */
    g_uart1_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;   /* ��Ӳ������ */
    g_uart1_handle.Init.Mode = UART_MODE_TX_RX;            /* �շ�ģʽ */
    HAL_UART_Init(&g_uart1_handle);                        /* HAL_UART_Init()��ʹ��UART1 */
    
    /* �ú����Ὺ�������жϣ���־λUART_IT_RXNE���������ý��ջ����Լ����ջ��������������� */
    HAL_UART_Receive_IT(&g_uart1_handle, (uint8_t *)g_rx_buffer, RXBUFFERSIZE);
}

/**
 * @brief       UART�ײ��ʼ������?
 * @param       huart: UART�������ָ��?
 * @note        �˺����ᱻHAL_UART_Init()����
 *              ���ʱ��ʹ�ܣ��������ã��ж�����?
 * @retval      ��
 */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef gpio_init_struct;
    if (huart->Instance == USART_UX)                                /* ����Ǵ���?�����д���1 MSP��ʼ�� */
    {
        USART_UX_CLK_ENABLE();                                      /* USART1 ʱ��ʹ�� */
        USART_TX_GPIO_CLK_ENABLE();                                 /* ��������ʱ��ʹ�� */
        USART_RX_GPIO_CLK_ENABLE();                                 /* ��������ʱ��ʹ�� */

        gpio_init_struct.Pin = USART_TX_GPIO_PIN;                   /* TX���� */
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;                    /* �������� */
        gpio_init_struct.Pull = GPIO_PULLUP;                        /* ���� */
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;              /* ���� */
        gpio_init_struct.Alternate = USART_TX_GPIO_AF;              /* ����ΪUSART1 */
        HAL_GPIO_Init(USART_TX_GPIO_PORT, &gpio_init_struct);       /* ��ʼ���������� */

        gpio_init_struct.Pin = USART_RX_GPIO_PIN;                   /* RX���� */
        gpio_init_struct.Alternate = USART_RX_GPIO_AF;              /* ����ΪUSART1 */
        HAL_GPIO_Init(USART_RX_GPIO_PORT, &gpio_init_struct);       /* ��ʼ���������� */

#if USART_EN_RX
        HAL_NVIC_EnableIRQ(USART_UX_IRQn);                          /* ʹ��USART1�ж�ͨ�� */
        HAL_NVIC_SetPriority(USART_UX_IRQn, 3, 3);                  /* ��ռ���ȼ�3�������ȼ�3 */
#endif
    }
}

/**
 * @brief       Rx����ص�����?
 * @param       huart: UART�������ָ��?
 * @retval      ��
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        uint8_t ch = g_rx_buffer[0];

        if (ch == '\r')
        {
            s_usart_rx_cr_seen = true;
        }
        else if (ch == '\n')
        {
            usart_queue_current_line();
            s_usart_rx_len = 0U;
            s_usart_rx_cr_seen = false;
            g_usart_rx_sta = 0U;
        }
        else
        {
            if (s_usart_rx_cr_seen)
            {
                usart_queue_current_line();
                s_usart_rx_len = 0U;
                s_usart_rx_cr_seen = false;
            }

            if (s_usart_rx_len < (USART_REC_LEN - 1U))
            {
                s_usart_rx_line[s_usart_rx_len++] = ch;
                g_usart_rx_buf[g_usart_rx_sta & 0x3FFFU] = ch;
                g_usart_rx_sta = s_usart_rx_len;
            }
            else
            {
                s_usart_rx_len = 0U;
                g_usart_rx_sta = 0U;
            }
        }
        HAL_UART_Receive_IT(&g_uart1_handle, (uint8_t *)g_rx_buffer, RXBUFFERSIZE);
    }
}
/**
 * @brief       ����1�жϷ�����
 * @param       ��
 * @retval      ��
 */
void USART_UX_IRQHandler(void)
{ 
#if SYS_SUPPORT_OS                        /* ʹ��OS */
    OSIntEnter();    
#endif

    HAL_UART_IRQHandler(&g_uart1_handle); /* ����HAL���жϴ������ú��� */

#if SYS_SUPPORT_OS                      /* ʹ��OS */
    OSIntExit();
#endif

}

#endif


 

 



 

 





