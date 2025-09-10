/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2015-09-10     WangShun     first version
 */

#include <rtthread.h>
#include <rtdbg.h>
#include <stdlib.h>
#include "drv_common.h"
#include "uart_config.h"

#include "gd32h7xx.h"

#define DBG_TAG "drv.usart"
#ifdef DRV_DEBUG
    #define DBG_LVL DBG_LOG
#else
    #define DBG_LVL DBG_INFO
#endif
#include <rtdbg.h>

#ifdef RT_USING_CONSOLE

/* UART configuration structure */
struct gd32_uart_config
{
    const char *name;
    uint32_t usart_periph;   /* USART0, USART1 ... */
    uint32_t irq_type;

    const char *tx_pin_name;
    const char *rx_pin_name;
};

static struct gd32_uart_config *_uart_config = RT_NULL;

/* Define UART configurations based on BSP settings */
struct gd32_uart_config uart_config[] =
{
#ifdef BSP_USING_UART1
    UART1_CONFIG,
#endif
#ifdef BSP_USING_UART2
    UART2_CONFIG,
#endif
#ifdef BSP_USING_UART3
    UART3_CONFIG,
#endif
#ifdef BSP_USING_UART4
    UART4_CONFIG,
#endif
#ifdef BSP_USING_UART5
    UART5_CONFIG,
#endif
#ifdef BSP_USING_UART6
    UART6_CONFIG,
#endif
#ifdef BSP_USING_UART7
    UART7_CONFIG,
#endif
#ifdef BSP_USING_UART8
    UART8_CONFIG,
#endif
};

/* Enable UART peripheral clock */
static rt_err_t gd32_uart_clk_enable(uint32_t periph)
{
    switch (periph)
    {
    case USART0: rcu_periph_clock_enable(RCU_USART0); break;
    case USART1: rcu_periph_clock_enable(RCU_USART1); break;
    case USART2: rcu_periph_clock_enable(RCU_USART2); break;
    case UART3:  rcu_periph_clock_enable(RCU_UART3);  break;
    case UART4:  rcu_periph_clock_enable(RCU_UART4);  break;
    case USART5: rcu_periph_clock_enable(RCU_USART5); break;
    case UART6:  rcu_periph_clock_enable(RCU_UART6);  break;
    case UART7:  rcu_periph_clock_enable(RCU_UART7);  break;
    default: return -RT_ERROR;
    }
    return RT_EOK;
}

/* Enable GPIO peripheral clock */
static rt_err_t gd32_gpio_clk_enable(uint32_t gpio_periph)
{
    switch (gpio_periph)
    {
    case GPIOA: rcu_periph_clock_enable(RCU_GPIOA); break;
    case GPIOB: rcu_periph_clock_enable(RCU_GPIOB); break;
    case GPIOC: rcu_periph_clock_enable(RCU_GPIOC); break;
    case GPIOD: rcu_periph_clock_enable(RCU_GPIOD); break;
    case GPIOE: rcu_periph_clock_enable(RCU_GPIOE); break;
    case GPIOF: rcu_periph_clock_enable(RCU_GPIOF); break;
    case GPIOG: rcu_periph_clock_enable(RCU_GPIOG); break;
    case GPIOH: rcu_periph_clock_enable(RCU_GPIOH); break;
    default: return -RT_ERROR;
    }
    return RT_EOK;
}

/* Convert pin name string (e.g., "PA9") to GPIO port and pin */
static void get_pin_by_name(const char* pin_name, uint32_t *port, uint32_t *pin)
{
    int pin_num = atoi((char*) &pin_name[2]);
    char port_ch = pin_name[1];
    if (port_ch >= 'a' && port_ch <= 'z') port_ch -= 32;

    *port = GPIOA + (port_ch - 'A') * (GPIOB - GPIOA);
    *pin = BIT(pin_num);
}

/* Configure GPIO pins for UART */
static rt_err_t gd32_gpio_configure(struct gd32_uart_config *config)
{
    uint32_t tx_port, rx_port;
    uint32_t tx_pin, rx_pin;
    get_pin_by_name(config->tx_pin_name, &tx_port, &tx_pin);
    get_pin_by_name(config->rx_pin_name, &rx_port, &rx_pin);

    gd32_gpio_clk_enable(tx_port);
    if (tx_port != rx_port) gd32_gpio_clk_enable(rx_port);

    /* Alternate function selection:
     *   USART0~USART2 (UART1-3) -> AF7
     *   UART3~UART7  (UART4-8) -> AF8
     */
    uint8_t af;
    switch (config->usart_periph)
    {
    case USART0:
    case USART1:
    case USART2:
        af = GPIO_AF_7;
        break;
    case UART3:
    case UART4:
    case USART5:
    case UART6:
    case UART7:
        af = GPIO_AF_8;
        break;
    default:
        return -RT_ERROR;
    }

    gpio_af_set(tx_port, af, tx_pin);
    gpio_mode_set(tx_port, GPIO_MODE_AF, GPIO_PUPD_PULLUP, tx_pin);
    gpio_output_options_set(tx_port, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, tx_pin);

    gpio_af_set(rx_port, af, rx_pin);
    gpio_mode_set(rx_port, GPIO_MODE_AF, GPIO_PUPD_PULLUP, rx_pin);
    gpio_output_options_set(rx_port, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, rx_pin);

    return RT_EOK;
}

/* Configure USART peripheral */
static rt_err_t gd32_configure(struct gd32_uart_config *config)
{
    uint32_t usartx = config->usart_periph;

    gd32_uart_clk_enable(usartx);
    gd32_gpio_configure(config);

    usart_deinit(usartx);
    usart_baudrate_set(usartx, 115200U);
    usart_word_length_set(usartx, USART_WL_8BIT);
    usart_stop_bit_set(usartx, USART_STB_1BIT);
    usart_parity_config(usartx, USART_PM_NONE);
    usart_hardware_flow_rts_config(usartx, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(usartx, USART_CTS_DISABLE);
    usart_transmit_config(usartx, USART_TRANSMIT_ENABLE);
    usart_receive_config(usartx, USART_RECEIVE_ENABLE);
    usart_enable(usartx);

    return RT_EOK;
}

/* Initialize hardware UART */
int rt_hw_usart_init(void)
{
    _uart_config = &uart_config[0];
    if (_uart_config == RT_NULL) return -RT_ERROR;
    return gd32_configure(_uart_config);
}
INIT_BOARD_EXPORT(rt_hw_usart_init);

/* Console output */
void rt_hw_console_output(const char *str)
{
    const char *p = str;
    while (*p)
    {
        if (*p == '\n')
        {
            usart_data_transmit(_uart_config->usart_periph, '\r');
            while (usart_flag_get(_uart_config->usart_periph, USART_FLAG_TBE) == RESET);
        }
        usart_data_transmit(_uart_config->usart_periph, *p++);
        while (usart_flag_get(_uart_config->usart_periph, USART_FLAG_TBE) == RESET);
    }
}

#ifdef RT_USING_FINSH
/* Get a character from console */
char rt_hw_console_getchar(void)
{
    if (usart_flag_get(_uart_config->usart_periph, USART_FLAG_RBNE) != RESET)
    {
        return (char)usart_data_receive(_uart_config->usart_periph);
    }
    return -1;
}
#endif /* RT_USING_FINSH */

#endif /* RT_USING_CONSOLE */

