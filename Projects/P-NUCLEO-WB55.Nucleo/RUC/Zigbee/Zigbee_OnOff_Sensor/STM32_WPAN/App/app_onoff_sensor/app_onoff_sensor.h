/**
  ******************************************************************************
  * @file    app_onoff.h
  * @author  Zigbee Application Team
  * @brief   Header for OnOff cluster application file.
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
#ifndef APP_ONOFF_H
#define APP_ONOFF_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "app_common.h"
#include "app_entry.h"
#include "zigbee_interface.h"

/* Debug Part */
#include "stm_logging.h"
#include "dbg_trace.h"
#include <assert.h>

/* Zigbee Cluster */
#include "zcl/zcl.h"
#include "zcl/general/zcl.identify.h"
#include "zcl/general/zcl.onoff.h"

/* Defines ----------------------------------------------------------------- */
/* Endpoint device */
#define ONOFF_SENSOR_ENDPOINT           0x0003U
#define ONOFF_SENSOR_GROUP_ADDR         0x0001U

#define NB_OF_SERV_BINDABLE                  6U

/* ONOFF cluster */
#define ONOFF_SENSOR_MIN_REPORT         0x0000U
#define ONOFF_SENSOR_MAX_REPORT         0x0000U
#define ONOFF_SENSOR_REPORT_CHANGE      0x0001U /* On/Off */

/* Typedef ----------------------------------------------------------------- */
typedef struct
{
  struct ZigBeeT *zb; /* ZigBee stack reference. */

  /* Find&Bind part */
  uint8_t  bind_nb;
  struct ZbApsAddrT bind_table[NB_OF_SERV_BINDABLE];

  //OnOff
  bool is_on_init;  
  bool On;
  bool cmd_send;

  /* Cluster used */
  struct ZbZclClusterT *identify_client;
  struct ZbZclClusterT *onoff_client;
} OnOff_Control_T;

/* Exported Prototypes -------------------------------------------------------*/
void App_OnOff_Sensor_cfg_EndPoint   (struct ZigBeeT *zb);
void App_OnOff_Sensor_ConfigGroupAddr(void);
void App_OnOff_Sensor_Restore_State  (void);
void App_OnOff_Sensor_Read_Attribute (struct ZbApsAddrT * dst);
void App_OnOff_Sensor_Toggle_Cmd     (void);
void App_OnOff_Sensor_FindBind       (void);
void App_OnOff_Sensor_Bind_Disp      (void);
void App_OnOff_Sensor_ReportConfig   (struct ZbApsAddrT * dst);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* APP_ONOFF_H */
