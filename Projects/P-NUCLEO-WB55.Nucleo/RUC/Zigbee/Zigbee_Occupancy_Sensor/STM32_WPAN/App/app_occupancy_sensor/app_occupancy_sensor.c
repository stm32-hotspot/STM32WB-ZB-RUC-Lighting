/**
  ******************************************************************************
  * @file    app_occupancy.c
  * @author  Zigbee Application Team
  * @brief   Application interface for Occupancy functionnalities
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
#include "pir_parallax.h"
#include "app_occupancy_sensor.h"

#include "stm32_seq.h"

/* Private defines ---------------------------------------------------------- */
#define IDENTIFY_MODE_DELAY         30
#define HW_TS_IDENTIFY_MODE_DELAY    (IDENTIFY_MODE_DELAY * HW_TS_SERVER_1S_NB_TICKS)

#define PIR_REFRESH_DELAY           5
#define HW_TS_PIR_REFRESH_DELAY     (PIR_REFRESH_DELAY * HW_TS_SERVER_1S_NB_TICKS)

/* Timer definitions */
static uint8_t TS_ID_PIR_REFRESH;

/* Application Variable-------------------------------------------------------*/
Occupancy_Control_T app_Occupancy =
{
  .Occupancy = 0U,
};


/* Private functions prototypes-----------------------------------------------*/
// static void App_Occupancy_Sensor_detect(void);
// static void App_Occupancy_Sensor_Refresh(void);

/* Clusters CFG ------------------------------------------------------------ */
/**
 * @brief  Configure and register Zigbee application endpoints
 * @param  zb Zigbee stack instance
 * @retval None
 */
void App_Occupancy_Sensor_Cfg_Endpoint(struct ZigBeeT *zb)
{ 
  struct ZbApsmeAddEndpointReqT  req;
  struct ZbApsmeAddEndpointConfT conf;
  
  /* Endpoint: OCCUPANCY_SENSOR_ENDPOINT */
  memset(&req, 0, sizeof(req));
  req.profileId = ZCL_PROFILE_HOME_AUTOMATION;
  req.deviceId  = ZCL_DEVICE_ONOFF_OCCUPANCY_SENSOR;
  req.endpoint  = OCCUPANCY_SENSOR_ENDPOINT;
  ZbZclAddEndpoint(zb, &req, &conf);
  assert(conf.status == ZB_STATUS_SUCCESS);

  /* Idenfity server */
  app_Occupancy.identify_server = ZbZclIdentifyServerAlloc(zb, OCCUPANCY_SENSOR_ENDPOINT, NULL);
  assert(app_Occupancy.identify_server != NULL);
  assert(ZbZclClusterEndpointRegister(app_Occupancy.identify_server) != NULL);

  /* Occupancy server */
  app_Occupancy.occupancy_server = ZbZclOccupancyServerAlloc(zb, OCCUPANCY_SENSOR_ENDPOINT);
  assert(app_Occupancy.occupancy_server != NULL);
  assert(ZbZclClusterEndpointRegister(app_Occupancy.occupancy_server) != NULL);

  /* Initialise Occupancy attribute by default values  */
  (void) ZbZclAttrIntegerWrite(app_Occupancy.occupancy_server, ZCL_OCC_ATTR_SENSORTYPE, OCCUPANCY_SENSOR_TYPE);
  (void) ZbZclAttrIntegerWrite(app_Occupancy.occupancy_server, ZCL_OCC_ATTR_SENSORTYPE_BITMAP, OCCUPANCY_SENSOR_BITMAP);
  (void) ZbZclAttrIntegerWrite(app_Occupancy.occupancy_server, ZCL_OCC_ATTR_PIR_OU_DELAY, PIR_REFRESH_DELAY);

  /* make a local pointer of Zigbee stack to read easier */
  app_Occupancy.zb = zb;

  /* Initilaze the PIR associated to the cluster */
  BSP_PIR_Init(BUTTON_MODE_EXTI); 
  UTIL_SEQ_RegTask(1U << CFG_TASK_BUTTON_PIR, UTIL_SEQ_RFU, App_Occupancy_Sensor_detect);
  HW_TS_Create(CFG_TIM_PIR_REFRESH, &TS_ID_PIR_REFRESH, hw_ts_Repeated, App_Occupancy_Sensor_Refresh);
} /* App_Occupancy_Sensor_Cfg_Endpoint */

/**
 * @brief  Associate the Endpoint to the group address
 * @param  zb Zigbee stack instance
 * @retval None
 */
void App_Occupancy_Sensor_ConfigGroupAddr(void)
{
  struct ZbApsmeAddGroupReqT  req;
  struct ZbApsmeAddGroupConfT conf;
  
  memset(&req, 0, sizeof(req));
  req.endpt     = OCCUPANCY_SENSOR_ENDPOINT;
  req.groupAddr = OCCUPANCY_SENSOR_GROUP_ADDR;
  ZbApsmeAddGroupReq(app_Occupancy.zb, &req, &conf);
} /* App_Occupancy_Sensor_ConfigGroupAddr */

/**
 * @brief Restore the state after persitance data loaded
 * 
 */
void App_Occupancy_Sensor_Restore_State(void)
{}

/* Identify actions ----------------------------------------------------------*/
/**
 * @brief Put Identify cluster into identify mode for the binding with it
 * @param  None
 * @retval None
 */
void App_Occupancy_Sensor_IdentifyMode(void)
{
  uint64_t epid = 0U;

  if (app_Occupancy.zb == NULL)
  {
    APP_ZB_DBG("Error, zigbee stack not initialized");
    return;
  }
  /* Check if the router joined the network */
  if (ZbNwkGet(app_Occupancy.zb, ZB_NWK_NIB_ID_ExtendedPanId, &epid, sizeof(epid)) != ZB_STATUS_SUCCESS)
  {
    APP_ZB_DBG("Error, failed to get network information");
    return;
  }
  if (epid == 0U)
  {
    APP_ZB_DBG("Error, device not on a network");
    return;
  }
  APP_ZB_DBG("Turn on Identify Mode for %ds", IDENTIFY_MODE_DELAY);

  if (app_Occupancy.identify_server != NULL)
  {
    ZbZclIdentifyServerSetTime(app_Occupancy.identify_server, IDENTIFY_MODE_DELAY);
  }
} /* App_Occupancy_Sensor_IdentifyMode */

/**
 * @brief Launched when the PIR detect a presence
 * 
 */
void App_Occupancy_Sensor_detect(void)
{
  enum ZclStatusCodeT status = ZCL_STATUS_FAILURE;
  
  APP_ZB_DBG("PIR detection start");
  app_Occupancy.Occupancy = 1;
  status = ZbZclAttrIntegerWrite(app_Occupancy.occupancy_server, ZCL_OCC_ATTR_OCCUPANCY, 1);
  if (status != ZCL_STATUS_SUCCESS)
    APP_ZB_DBG("Error during writing ATTRIBUTE ZCL_OCC_ATTR_OCCUPANCY");
    
  HW_TS_Start(TS_ID_PIR_REFRESH, HW_TS_PIR_REFRESH_DELAY);
} /* App_Occupancy_Sensor_detect */

/**
 * @brief Launch by timer to detect the end of presence detection
 * 
 */
void App_Occupancy_Sensor_Refresh(void)
{
  enum ZclStatusCodeT status = ZCL_STATUS_FAILURE;
  uint32_t PIR_Value = BSP_PIR_GetState();
  
  // APP_ZB_DBG("PIR detection refresh");
  if (PIR_Value == 0)
  {
    APP_ZB_DBG("PIR detection end");
    app_Occupancy.Occupancy = 0;
    status = ZbZclAttrIntegerWrite(app_Occupancy.occupancy_server, ZCL_OCC_ATTR_OCCUPANCY, 0);
    if (status != ZCL_STATUS_SUCCESS)
      APP_ZB_DBG("Error during writing ATTRIBUTE ZCL_OCC_ATTR_OCCUPANCY");
    
    HW_TS_Stop(TS_ID_PIR_REFRESH);
  }
} /* App_Occupancy_Sensor_Refresh */

/**
 * @brief Display the Occupancy status
 * 
 */
void App_Occupancy_Sensor_Disp(void)
{
  enum ZclStatusCodeT status = ZCL_STATUS_FAILURE;
  uint8_t Occupancy = 0;

  APP_ZB_DBG("Status Occupancy local : %d", app_Occupancy.Occupancy);

  status = ZbZclAttrRead(app_Occupancy.occupancy_server, ZCL_OCC_ATTR_OCCUPANCY, NULL,
                       &(Occupancy), sizeof(Occupancy), false);
  if (status != ZCL_STATUS_SUCCESS)
    APP_ZB_DBG("Error during reading ATTRIBUTE ZCL_OCC_ATTR_OCCUPANCY");

  APP_ZB_DBG("Status Occupancy attribute : %d", Occupancy);
} /* App_Occupancy_Sensor_Disp */

