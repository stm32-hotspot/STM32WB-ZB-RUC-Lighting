/**
  ******************************************************************************
  * @file    app_occupancy.h
  * @author  Zigbee Application Team
  * @brief   Header for Occupancy cluster application file.
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
#ifndef APP_OCCUPANCY_H
#define APP_OCCUPANCY_H

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
#include "zcl/general/zcl.occupancy.h"

/* Defines -------------------------------------------------------------------*/
/* Endpoint device */
#define OCCUPANCY_SENSOR_ENDPOINT         0x0004U
#define OCCUPANCY_SENSOR_GROUP_ADDR       0x0004U

#define OCCUPANCY_SENSOR_TYPE        0x00U
#define OCCUPANCY_SENSOR_BITMAP      0x01U

/* Types ------------------------------------------------------------------- */
typedef struct
{
  bool Occupancy;
  struct ZigBeeT *zb; /* ZigBee stack reference. */
  
  /* Clusters used */
  struct ZbZclClusterT * identify_server;
  struct ZbZclClusterT * occupancy_server;
} Occupancy_Control_T;

/* Exported Prototypes -------------------------------------------------------*/
void App_Occupancy_Sensor_Cfg_Endpoint   (struct ZigBeeT *zb);
void App_Occupancy_Sensor_ConfigGroupAddr(void);
void App_Occupancy_Sensor_Restore_State  (void);
void App_Occupancy_Sensor_IdentifyMode   (void);
void App_Occupancy_Sensor_detect         (void);

// TODO to remove after debug
void App_Occupancy_Sensor_Refresh        (void);
void App_Occupancy_Sensor_Disp           (void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* APP_OCCUPANCY_H */