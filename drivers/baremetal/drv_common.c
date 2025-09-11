/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-09-11     WangShun     first version
 */

#include <gd32h7xx.h>

extern int rt_hw_usart_init();

volatile static uint32_t delay;
/*!
    \brief      Configure the system clock
    \param[in]  clk_source: Clock source
    \param[in]  source_freq: Source frequency
    \param[in]  target_freq: Target frequency
    \retval     None
*/
void clk_init(char *clk_source, int source_freq, int target_freq)
{
    /* setup systick timer for 1000Hz interrupts */
    if(SysTick_Config(SystemCoreClock / 1000U))
    {
        /* capture error */
        while(1)
        {
        }
    }
    /* configure the systick handler priority */
    NVIC_SetPriority(SysTick_IRQn, 0x00U);
}

/*!
    \brief      Initialize UART communication
    \param[in]  None
    \param[out] None
    \retval     Returns 0 on success
*/
int uart_init()
{
    rt_hw_usart_init();
    return 0;
}

/*!
    \brief      System Tick interrupt handler
    \param[in]  None
    \param[out] None
    \retval     None
*/
void SysTick_Handler(void)
{
    if(0U != delay)
    {
        delay--;
    }
}

/*!
    \brief      Delay execution for a specified time in milliseconds
    \param[in]  ms_time: Time in milliseconds to delay
    \param[out] None
    \retval     None
*/
void wait_ms(unsigned long ms_time)
{
    delay = ms_time;

    while(0U != delay)
    {
    }
}

/*!
    \brief      Hard Fault exception handler
    \param[in]  None
    \param[out] None
    \retval     None
*/
void HardFault_Handler(void)
{
    /* if Hard Fault exception occurs, go to infinite loop */
    while(1)
    {
    }
}

