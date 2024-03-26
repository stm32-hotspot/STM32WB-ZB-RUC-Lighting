/**
  ******************************************************************************
  * @file    app_light_switch_level.h
  * @author  Zigbee Application Team
  * @brief   Header for Level cluster of Light Switch EndPoint.
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
#ifndef APP_LEVEL_H
#define APP_LEVEL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Defines ----------------------------------------------------------------- */
#define LIGHT_SWITCH_LEVEL_MIN_REPORT            0x0000
#define LIGHT_SWITCH_LEVEL_MAX_REPORT            0x0000
#define LIGHT_SWITCH_LEVEL_REPORT_CHANGE         0x0001 /* Level */

#define LIGHT_SWITCH_LEVEL_MAX_VALUE           0xFFU
#define LIGHT_SWITCH_LEVEL_MIN_VALUE           0x0FU
#define LIGHT_SWITCH_LEVEL_STEP                0x10U

#define MODE_NOCHANGE           3U
#define MODE_UP                 1U
#define MODE_DOWN               0U

/* Typedef ----------------------------------------------------------------- */
typedef struct
{
  /* Find&Bind part */
  uint8_t  bind_nb;
  struct ZbApsAddrT bind_table[NB_OF_SERV_BINDABLE];
  
  // Level Ctrl
  bool    is_level_init;   
  uint8_t level_mode;
  uint8_t level;
  uint8_t cmd_send;
  
  /* Cluster used */
  struct ZbZclClusterT *levelControl_client;  

} Level_Control_T;

/* Exported Prototypes -------------------------------------------------------*/
Level_Control_T * App_Light_Switch_Level_cfg(struct ZigBeeT *zb);
void App_Light_Switch_Level_ReportConfig    (struct ZbApsAddrT * dst);
void App_Light_Switch_Level_Read_Attribute  (struct ZbApsAddrT * dst);
void App_Light_Switch_Level_Cmd             (struct ZbApsAddrT * dst);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* APP_LEVEL_H */
