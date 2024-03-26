/**
  ******************************************************************************
  * @file    pir_parallax.c
  * @author  Zigbee Application Team
  * @brief   BSP PIR from Parallax ref #555-28027
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2019-2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "app_conf.h" 
#include "stm32wbxx_hal.h"
#include "pir_parallax.h"

/* Private includes ----------------------------------------------------------*/
#include <assert.h>

/** @defgroup STM32WBXX_NUCLEO_PIR_Functions BUTTON Functions
  * @{
  */ 

/**
  * @brief  Configures PIR GPIO and EXTI Line.
  * @param  PirMode: Specifies Pir mode.
  *   This parameter can be one of following parameters:   
  *     @arg BUTTON_MODE_GPIO: Pir will be used as simple IO 
  *     @arg BUTTON_MODE_EXTI: Pir will be connected to EXTI line with interrupt
  *                            generation capability  
  * @retval None
  */
void BSP_PIR_Init(ButtonMode_TypeDef PirMode)
{
  GPIO_InitTypeDef gpio_init_structure = {0};
  
  /* Enable the BUTTON Clock */
  PIR_GPIO_CLK_ENABLE();
  
  gpio_init_structure.Pin   = PIR_PIN;
  gpio_init_structure.Pull  = GPIO_PULLDOWN;
  gpio_init_structure.Speed = GPIO_SPEED_FREQ_HIGH;
  
  if(PirMode == BUTTON_MODE_GPIO)
  {
    /* Configure Button pin as input */
    gpio_init_structure.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(PIR_GPIO_PORT, &gpio_init_structure);
    
    /* Wait Button pin startup stability */
    HAL_Delay(1);

    /* Enable and set Button EXTI Interrupt to the lowest priority */
    HAL_NVIC_SetPriority(PIR_EXTI_IRQn, 0x0F, 0x00);
    HAL_NVIC_EnableIRQ(PIR_EXTI_IRQn);
  }
  else /* (PirMode == BUTTON_MODE_EXTI) */
  {
    /* Configure Button pin as input with External interrupt */
    gpio_init_structure.Mode = GPIO_MODE_IT_RISING; 
    HAL_GPIO_Init(PIR_GPIO_PORT, &gpio_init_structure);

    /* TODO    
    (void)HAL_EXTI_GetHandle(&hpb_exti[Button], button_exti_line[Button]);
    (void)HAL_EXTI_RegisterCallback(&hpb_exti[Button],  HAL_EXTI_COMMON_CB_ID, button_callback[Button]);
    */ 

    /* Enable and set Button EXTI Interrupt to the lowest priority */
    HAL_NVIC_SetPriority(PIR_EXTI_IRQn, 0x0F, 0x00);
    HAL_NVIC_EnableIRQ(PIR_EXTI_IRQn);
  }
} /* BSP_PIR_Init */

/**
  * @brief  Pir DeInit.
  * @param  None
  * @note PB DeInit does not disable the GPIO clock
  * @retval None
  */
void BSP_PIR_DeInit(void)
{
  GPIO_InitTypeDef gpio_init_structure;

  gpio_init_structure.Pin = PIR_PIN;
  HAL_NVIC_DisableIRQ(PIR_EXTI_IRQn);
  HAL_GPIO_DeInit(PIR_GPIO_PORT, gpio_init_structure.Pin);
}

/**
  * @brief  Returns the selected PIR state.
  * @param  None
  * @retval The PIR GPIO pin value.
  */
uint32_t BSP_PIR_GetState(void)
{
  return HAL_GPIO_ReadPin(PIR_GPIO_PORT, PIR_PIN);
}

// /**
//   * @brief  BSP PIR interrupt handler.
//   * @param  None
//   * @retval None.
//   */
// void BSP_PIR_IRQHandler(void)
// {
//   HAL_EXTI_IRQHandler(&hpb_exti[Button]);
// }


/**
  * @}
  */ 

