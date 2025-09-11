/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-11-09     xiangxistu   first version
 * 2020-05-18     chenyaxing   modify uart_config struct
 * 2025-09-11     WangShun     fully adapted for GD32H7xx with SPL
 */

#include <string.h>
#include <stdlib.h>
#include <gd32h7xx.h>
#include "uart_config.h"
#include <board.h>

/* GD32 UART config structure */
struct gd32_uart_config
{
    const char *name;
    uint32_t instance; /* USART peripheral address */
    IRQn_Type irq_type;
    const char *tx_pin_name;
    const char *rx_pin_name;
};

static struct gd32_uart_config *_uart_config = NULL;

/* UART configuration array */
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
};

static long gd32_uart_clk_enable(struct gd32_uart_config *config)
{
    /* Enable UART peripheral clock */
    switch (config->instance)
    {
#ifdef BSP_USING_UART1
    case USART0:
        rcu_periph_clock_enable(RCU_USART0);
        break;
#endif
#ifdef BSP_USING_UART2
    case USART1:
        rcu_periph_clock_enable(RCU_USART1);
        break;
#endif
#ifdef BSP_USING_UART3
    case USART2:
        rcu_periph_clock_enable(RCU_USART2);
        break;
#endif
#ifdef BSP_USING_UART4
    case UART3:
        rcu_periph_clock_enable(RCU_UART3);
        break;
#endif
#ifdef BSP_USING_UART5
    case UART4:
        rcu_periph_clock_enable(RCU_UART4);
        break;
#endif
    default:
        return -1;
    }

    return 0;
}

static long gd32_gpio_clk_enable(uint32_t gpiox)
{
    /* Enable GPIO peripheral clock */
    switch (gpiox)
    {
    case GPIOA:
        rcu_periph_clock_enable(RCU_GPIOA);
        break;
    case GPIOB:
        rcu_periph_clock_enable(RCU_GPIOB);
        break;
    case GPIOC:
        rcu_periph_clock_enable(RCU_GPIOC);
        break;
    case GPIOD:
        rcu_periph_clock_enable(RCU_GPIOD);
        break;
    case GPIOE:
        rcu_periph_clock_enable(RCU_GPIOE);
        break;
    case GPIOF:
        rcu_periph_clock_enable(RCU_GPIOF);
        break;
    case GPIOG:
        rcu_periph_clock_enable(RCU_GPIOG);
        break;
    case GPIOH:
        rcu_periph_clock_enable(RCU_GPIOH);
        break;
    default:
        return -1;
    }

    return 0;
}

static int up_char(char *c)
{
    if ((*c >= 'a') && (*c <= 'z'))
    {
        *c = *c - 32;
    }
    return 0;
}

static void get_pin_by_name(const char *pin_name, uint32_t *port, uint32_t *pin)
{
    int pin_num = atoi((char *)&pin_name[2]);
    char port_name = pin_name[1];
    up_char(&port_name);
    *port = (GPIOA + (uint32_t)(port_name - 'A') * (GPIOB - GPIOA));
    *pin = (1U << pin_num); /* GD32 uses 1 << n for pin */
}

static long gd32_gpio_configure(struct gd32_uart_config *config)
{
    int uart_num = 0;
    uint32_t tx_port, rx_port;
    uint32_t tx_pin, rx_pin;

    /* Extract UART number from name (e.g., "uart1" -> 1) */
    uart_num = config->name[4] - '0';
    get_pin_by_name(config->tx_pin_name, &tx_port, &tx_pin);
    get_pin_by_name(config->rx_pin_name, &rx_port, &rx_pin);

    /* Enable GPIO clocks */
    gd32_gpio_clk_enable(tx_port);
    if (tx_port != rx_port)
    {
        gd32_gpio_clk_enable(rx_port);
    }

    /* Configure TX pin */
    gpio_mode_set(tx_port, GPIO_MODE_AF, GPIO_PUPD_PULLUP, tx_pin);
    gpio_output_options_set(tx_port, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, tx_pin);

    /* Configure RX pin */
    gpio_mode_set(rx_port, GPIO_MODE_AF, GPIO_PUPD_PULLUP, rx_pin);
    gpio_output_options_set(rx_port, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, rx_pin);

    /* Configure alternate function (AF) */
    /* GD32F4xx: USART0-2 use AF7, UART3-4 use AF8 */
    uint8_t af = (uart_num <= 3) ? GPIO_AF_7 : GPIO_AF_8;
    gpio_af_set(tx_port, af, tx_pin);
    gpio_af_set(rx_port, af, rx_pin);

    return 0;
}

static long gd32_configure(struct gd32_uart_config *config)
{
    /* Enable UART clock */
    if (gd32_uart_clk_enable(config) != 0)
        return -1;

    /* Initialize UART */
    usart_deinit(config->instance);
    usart_baudrate_set(config->instance, 115200U);
    usart_parity_config(config->instance, USART_PM_NONE);
    usart_word_length_set(config->instance, USART_WL_8BIT);
    usart_stop_bit_set(config->instance, USART_STB_1BIT);
    usart_receive_config(config->instance, USART_RECEIVE_ENABLE);
    usart_transmit_config(config->instance, USART_TRANSMIT_ENABLE);

    /* Enable UART */
    usart_enable(config->instance);

    /* Configure GPIO */
    if (gd32_gpio_configure(config) != 0)
        return -1;

    return 0;
}

int rt_hw_usart_init(void)
{
    _uart_config = &uart_config[0];
    return gd32_configure(_uart_config);
}

void print_char(char c)
{
    while (RESET == usart_flag_get(_uart_config->instance, USART_FLAG_TBE));
    usart_data_transmit(_uart_config->instance, (uint8_t)c);
}

