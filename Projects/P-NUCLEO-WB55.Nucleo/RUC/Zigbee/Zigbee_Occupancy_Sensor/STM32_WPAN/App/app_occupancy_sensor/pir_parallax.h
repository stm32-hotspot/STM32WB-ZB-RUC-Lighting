/**
  ******************************************************************************
  * @file    pir_parallax.h
  * @author  Zigbee Application Team
  * @brief   This header file contains the functions prototypes for the
  *          PIR driver
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef PIR_PARALLAX_H
#define PIR_PARALLAX_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "app_conf.h"
#include "stm_logging.h"
#include "dbg_trace.h"

/** @defgroup STM32WBXX_NUCLEO_PIR PIR Constants
  * @{
  */  

/**
 * @brief PIR detector
 */
#define PIR_PIN                          GPIO_PIN_2
#define PIR_GPIO_PORT                    GPIOC
#define PIR_GPIO_CLK_ENABLE()            __HAL_RCC_GPIOC_CLK_ENABLE()
#define PIR_GPIO_CLK_DISABLE()           __HAL_RCC_GPIOC_CLK_DISABLE()
#define PIR_EXTI_LINE                    GPIO_PIN_2
#ifdef CORE_CM0PLUS
#define PIR_EXTI_IRQn                    EXTI15_4_IRQn
#else
#define PIR_EXTI_IRQn                    EXTI2_IRQn
#endif
/**
  * @}
  */

/** @addtogroup STM32WBXX_NUCLEO_PIR_Functions
  * @{
  */
void             BSP_PIR_Init(ButtonMode_TypeDef ButtonMode);
void             BSP_PIR_DeInit(void);
uint32_t         BSP_PIR_GetState(void);
// void             BSP_PIR_IRQHandler(void);
// void             BSP_PIR_Callback(void);
/**
  * @}
  */


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PIR_PARALLAX_H */