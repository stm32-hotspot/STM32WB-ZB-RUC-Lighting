/**
  ******************************************************************************
  * @file    app_light.c
  * @author  Zigbee Application Team
  * @brief   Application interface for Light EndPoint functionnalities
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

/* board dependancies */
#include "stm32wb5mm_dk_lcd.h"
#include "stm32_lcd.h"

/* service dependancies */
#include "app_light_cfg.h"
#include "app_core.h"
#include "app_zigbee.h"

/* Private defines -----------------------------------------------------------*/
#define IDENTIFY_MODE_DELAY              30
#define HW_TS_IDENTIFY_MODE_DELAY         (IDENTIFY_MODE_DELAY * HW_TS_SERVER_1S_NB_TICKS)

/* Application Variable-------------------------------------------------------*/
Light_Control_T app_Light_Control;

/* Timer definitions */
// static uint8_t TS_ID_IDENTIFY_MODE;

/* Finding&Binding function Declaration --------------------------------------*/
static void App_Light_Identify_cb(struct ZbZclClusterT *cluster, enum ZbZclIdentifyServerStateT state, void *arg);
static void App_Light_FindBind_cb(enum ZbStatusCodeT status, void *arg);

/* Clusters CFG ------------------------------------------------------------ */
/**
 * @brief  Configure and register Zigbee application endpoints, onoff callbacks
 * @param  zb Zigbee stack instance
 * @retval None
 */
void App_Light_Cfg_Endpoint(struct ZigBeeT *zb)
{
  struct ZbApsmeAddEndpointReqT  req;
  struct ZbApsmeAddEndpointConfT conf;
  
  /* Endpoint: LIGHT_ENDPOINT */
  memset(&req, 0, sizeof(req));
  req.profileId = ZCL_PROFILE_HOME_AUTOMATION;
  req.deviceId  = ZCL_DEVICE_ONOFF_LIGHT;
  req.endpoint  = LIGHT_ENDPOINT;
  ZbZclAddEndpoint(zb, &req, &conf);
  assert(conf.status == ZB_STATUS_SUCCESS);

  /* Idenfity server */
  app_Light_Control.identify_server = ZbZclIdentifyServerAlloc(zb, LIGHT_ENDPOINT, NULL);
  assert(app_Light_Control.identify_server != NULL);
  assert(ZbZclClusterEndpointRegister(app_Light_Control.identify_server)!= NULL);
  ZbZclIdentifyServerSetCallback(app_Light_Control.identify_server, App_Light_Identify_cb);

  /* Idenfity client */
  app_Light_Control.identify_client = ZbZclIdentifyClientAlloc(zb, LIGHT_ENDPOINT);
  assert(app_Light_Control.identify_client != NULL);
  assert(ZbZclClusterEndpointRegister(app_Light_Control.identify_client)!= NULL);

  /* OnOff Server */
  app_Light_Control.app_OnOff = App_Light_OnOff_Cfg(zb);

  /*  Level Server */
  app_Light_Control.app_Level = App_Light_Level_Cfg(zb);

  /*  Occupancy Client */
  app_Light_Control.app_Occupancy = App_Light_Occupancy_Cfg(zb);

  /* make a local pointer of Zigbee stack to read easier */
  app_Light_Control.zb = zb;
} /* App_Light_Cfg_Endpoint */

/**
 * @brief  Associate the Endpoint to the group address
 * @param  zb Zigbee stack instance
 * @retval None
 */
void App_Light_ConfigGroupAddr(void)
{
  struct ZbApsmeAddGroupReqT  req;
  struct ZbApsmeAddGroupConfT conf;
  
  memset(&req, 0, sizeof(req));
  req.endpt     = LIGHT_ENDPOINT;
  req.groupAddr = LIGHT_GROUP_ADDR;
  ZbApsmeAddGroupReq(app_Light_Control.zb, &req, &conf);

} /* App_Light_ConfigGroupAddr */


/* Identify & FindBind actions ----------------------------------------------------------*/
/**
 * @brief Put Identify cluster into identify mode for the binding with it
 * @param  None
 * @retval None
 */
void App_Light_IdentifyMode(void)
{
  uint64_t epid = 0U;
  char disp_identify[18];

  /* Display info on LCD */
  UTIL_LCD_ClearStringLine(DK_LCD_STATUS_LINE);
  sprintf(disp_identify, "Identify Mode %2d", IDENTIFY_MODE_DELAY);
  UTIL_LCD_DisplayStringAt(0, LINE(DK_LCD_STATUS_LINE), (uint8_t *)disp_identify, CENTER_MODE);
  BSP_LCD_Refresh(0);

  if (app_Light_Control.zb == NULL)
  {
    APP_ZB_DBG("Error, zigbee stack not initialized");
    return;
  }
  /* Check if the router joined the network */
  if (ZbNwkGet(app_Light_Control.zb, ZB_NWK_NIB_ID_ExtendedPanId, &epid, sizeof(epid)) != ZB_STATUS_SUCCESS)
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

  ZbZclIdentifyServerSetTime(app_Light_Control.identify_server, IDENTIFY_MODE_DELAY);
  
} /* App_Light_IdentifyMode */

/**
 * @brief Needed to avoid all endpoints are at the same time in identify mode
 * 
 * @param cluster 
 * @param state 
 * @param arg 
 */
static void App_Light_Identify_cb(struct ZbZclClusterT *cluster, enum ZbZclIdentifyServerStateT state, void *arg)
{
  char disp_identify[18];
  switch (state)
  {
    case ZCL_IDENTIFY_START :
      APP_ZB_DBG("Turn on Identify Mode for %ds", IDENTIFY_MODE_DELAY);
      /* Display info on LCD */
      UTIL_LCD_ClearStringLine(DK_LCD_STATUS_LINE);
      sprintf(disp_identify, "Identify Mode %2d", IDENTIFY_MODE_DELAY);
      UTIL_LCD_DisplayStringAt(0, LINE(DK_LCD_STATUS_LINE), (uint8_t *)disp_identify, CENTER_MODE);
      BSP_LCD_Refresh(0);
      break;
    case ZCL_IDENTIFY_STOP:
      APP_ZB_DBG("Turn off Identify Mode");
      // TODO launch a task for Display_Clean_Status();
      break;
    default :
      APP_ZB_DBG("Identification mode cb state unknown");
      break;    
  }
}; /* App_Light_Identify_cb */

/**
 * @brief  Start Finding and Binding process as an initiator for Occupancy cluster
 * Call App_Light_FindBind_cb when successfull to configure correctly the binding
 * 
 */
void App_Light_FindBind(void)
{
  uint8_t status = ZCL_STATUS_FAILURE;
  uint64_t epid = 0U;

  if (app_Light_Control.zb == NULL)
  {
    APP_ZB_DBG("Error, zigbee stack not initialized");
    return;
  }

  /* Check if the router joined the network */
  if (ZbNwkGet(app_Light_Control.zb, ZB_NWK_NIB_ID_ExtendedPanId, &epid, sizeof(epid)) != ZB_STATUS_SUCCESS)
  {
    APP_ZB_DBG("Error, failed to get network information");
    return;
  }
  if (epid == 0U)
  {
    APP_ZB_DBG("Error, device not on a network");
    return;
  }
  
  APP_ZB_DBG("Initiate F&B");
  
  status = ZbStartupFindBindStart(app_Light_Control.zb, &App_Light_FindBind_cb, NULL);

  if (status != ZB_STATUS_SUCCESS)
  {
    APP_ZB_DBG(" Error, cannot start Finding & Binding, status = 0x%02x", status);
  }
} /* App_Light_FindBind */

/**
 * @brief  Task called after F&B process to configure a report and update the local status.
 *         Will call the configuration report for the good clusterId.
 * @param  None
 * @retval None
 */
static void App_Light_FindBind_cb(enum ZbStatusCodeT status, void *arg)
{
  // Retry if not succes ????
  if (status != ZB_STATUS_SUCCESS)
  {
    APP_ZB_DBG("Error while F&B | error code : 0x%02X", status);
    return;
  }
  else
  {
    struct ZbApsmeBindT entry;
  
    APP_ZB_DBG(" Item |   ClusterId | Long Address     | End Point");
    APP_ZB_DBG(" -----|-------------|------------------|----------");

    /* Loop only on the new binding element */
    for (uint8_t i = 0;; i++)
    {
      if (ZbApsGetIndex(app_Light_Control.zb, ZB_APS_IB_ID_BINDING_TABLE, &entry, sizeof(entry), i) != ZB_APS_STATUS_SUCCESS)
      {
        break;
      }
      if (entry.srcExtAddr == 0ULL)
      {
        continue;
      }
    
      /* display binding infos */
      APP_ZB_DBG("  %2d  |     0x%03x   | %016llx |   %2d", i, entry.clusterId, entry.dst.extAddr, entry.dst.endpoint);
        
      // Report on the Cluster selected to know when a modification status
      switch (entry.clusterId)
      {
        case ZCL_CLUSTER_MEAS_OCCUPANCY:
          // start report config proc
          App_Light_Occupancy_ReportConfig( &entry.dst);
          break;
        default:
          break;
      }
    }  
  }
} /* App_Light_FindBind_cb */


/* Light Persistence -------------------------------------------------------- */
/**
 * @brief  Restore the state at startup from persistence
 * @param  None
 * @retval stack status code
 */
enum ZclStatusCodeT App_Light_Restore_State(void)
{
  if (App_Light_OnOff_Restore_State() != ZCL_STATUS_SUCCESS)
    return ZCL_STATUS_FAILURE;
  if (App_Light_Level_Restore_State() != ZCL_STATUS_SUCCESS)
    return ZCL_STATUS_FAILURE;
  APP_ZB_DBG("Read back cluster information : SUCCESS");  
  
  App_Light_Refresh();
  App_Zigbee_Bind_Disp();

  return ZCL_STATUS_SUCCESS;
} /* App_Light_Restore_State */

/* Light Device Application ------------------------------------------------- */
/**
 * @brief Refresh the Light status from the attribut of the clusters
 * 
 */
void App_Light_Refresh(void)
{
  if (app_Light_Control.app_OnOff->On)
  {
    HAL_Delay(10);
    LED_Set_rgb(app_Light_Control.app_Level->level, app_Light_Control.app_Level->level, app_Light_Control.app_Level->level);
    APP_ZB_DBG("Light to ON - Level to : 0x%02x", app_Light_Control.app_Level->level);
  }
  else
  {
    APP_ZB_DBG("Light to OFF");
    HAL_Delay(10);
    LED_Off();
  }
} /* App_Light_Refresh */

/**
 * @brief Toggle the light from menu
 * 
 */
void App_Light_Toggle(void)
{
  // Update the cluster server attribute and send report attribute automatically if exists one
  (void) light_onoff_server_toggle_cb(app_Light_Control.app_OnOff->onoff_server, NULL, NULL);
} /* App_Light_Toggle */

/**
 * @brief Increase the light from menu
 * 
 */
void App_Light_Up(void)
{
  // TODO replace by light_level_server_move_to_level_cb ???
  if (app_Light_Control.app_Level->level < LIGHT_LEVEL_MAX_VALUE)
  {
    app_Light_Control.app_Level->level += LIGHT_LEVEL_STEP;
    (void)ZbZclAttrIntegerWrite(app_Light_Control.app_Level->level_server, ZCL_LEVEL_ATTR_CURRLEVEL, app_Light_Control.app_Level->level);
  }
  App_Light_Refresh();
} /* App_Light_Up */

/**
 * @brief Decrease the light from menu
 * 
 */
void App_Light_Down(void)
{
  if (app_Light_Control.app_Level->level > LIGHT_LEVEL_MIN_VALUE)
  {
    app_Light_Control.app_Level->level -= LIGHT_LEVEL_STEP;
    (void)ZbZclAttrIntegerWrite(app_Light_Control.app_Level->level_server, ZCL_LEVEL_ATTR_CURRLEVEL, app_Light_Control.app_Level->level);
  }

  App_Light_Refresh();
} /* App_Light_Down */

