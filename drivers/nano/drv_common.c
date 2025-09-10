/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-7      SummerGift   first version
 */

#include "drv_common.h"
#include "board.h"

#ifdef RT_USING_SERIAL
#ifdef RT_USING_SERIAL_V2
#include "drv_usart_v2.h"
#else
#include "drv_usart.h"
#endif /* RT_USING_SERIAL */
#endif /* RT_USING_SERIAL_V2 */

#define DBG_TAG    "drv_common"
#define DBG_LVL    DBG_INFO
#include <rtdbg.h>

#ifdef RT_USING_FINSH
#include <finsh.h>
static void reboot(uint8_t argc, char **argv)
{
    extern void rt_hw_cpu_reset(void);
    rt_hw_cpu_reset();
}
MSH_CMD_EXPORT(reboot, Reboot System);
#endif /* RT_USING_FINSH */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler */
    /* User can add his own implementation to report the HAL error return state */
    while (1)
    {
    }
    /* USER CODE END Error_Handler */
}

/** System Clock Configuration
*/
void SystemClock_Config(void)
{
    SysTick_Config(SystemCoreClock / RT_TICK_PER_SECOND);
    NVIC_SetPriority(SysTick_IRQn, 0);
}

/**
 * This is the timer interrupt service routine.
 *
 */
void SysTick_Handler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    rt_tick_increase();

    /* leave interrupt */
    rt_interrupt_leave();
}

/**
 * This function will initial STM32 board.
 */
void hw_board_init(char *clock_src, int32_t clock_src_freq, int32_t clock_target_freq)
{
    /* NVIC Configuration */
#define NVIC_VTOR_MASK              0x3FFFFF80
#ifdef  VECT_TAB_RAM
    /* Set the Vector Table base location at 0x10000000 */
    SCB->VTOR = (0x10000000 & NVIC_VTOR_MASK);
#else  /* VECT_TAB_FLASH  */
    /* Set the Vector Table base location at 0x08000000 */
    SCB->VTOR = (0x08000000 & NVIC_VTOR_MASK);
#endif

    SystemClock_Config();

    /* Pin driver initialization is open by default */
#ifdef RT_USING_PIN
    extern int rt_hw_pin_init(void);
    rt_hw_pin_init();
#endif

    /* USART driver initialization is open by default */
#ifdef RT_USING_SERIAL
    extern int rt_hw_usart_init(void);
    rt_hw_usart_init();
#endif
}

