/**
  ******************************************************************************
  * @file    app_light_switch_onoff.h
  * @author  Zigbee Application Team
  * @brief   Header for OnOff cluster of Light Switch EndPoint.
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
#ifndef APP_LIGHT_SWITCH_ONOFF_H
#define APP_LIGHT_SWITCH_ONOFF_H

#ifdef __cplusplus
extern "C" {
#endif

/* Defines ----------------------------------------------------------------- */
#define LIGHT_SWITCH_ONOFF_MIN_REPORT                0x0000
#define LIGHT_SWITCH_ONOFF_MAX_REPORT                0x0000
#define LIGHT_SWITCH_ONOFF_REPORT_CHANGE             0x0001 /* On/Off */

/* Typedef ----------------------------------------------------------------- */
typedef struct
{
  /* Find&Bind part */
  uint8_t  bind_nb;
  struct ZbApsAddrT bind_table[NB_OF_SERV_BINDABLE];
  
  //OnOff
  bool is_on_init;  
  bool On;
  bool cmd_send;

  /* Cluster used */
  struct ZbZclClusterT *onoff_client;

} OnOff_Control_T;

/* Exported Prototypes -------------------------------------------------------*/
OnOff_Control_T * App_Light_Switch_OnOff_cfg(struct ZigBeeT *zb);
void App_Light_Switch_OnOff_ReportConfig    (struct ZbApsAddrT * dst);
void App_Light_Switch_OnOff_Read_Attribute  (struct ZbApsAddrT * dst);
void App_Light_Switch_OnOff_Cmd             (struct ZbApsAddrT * dst);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* APP_LIGHT_SWITCH_ONOFF_H */
