/**
  ******************************************************************************
  * @file    app_light_occupancy.c
  * @author  Zigbee Application Team
  * @brief   Application interface for Occupancy functionnalities for Light Endpoint.
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

/* Private defines -----------------------------------------------------------*/

/* Application Variable-------------------------------------------------------*/
Occupancy_Control_T app_Occupancy;

extern Light_Control_T app_Light_Control;

/* Occupancy callbacks Declaration -------------------------------------------*/
static void App_Light_Occupancy_ReportConfig_cb(struct ZbZclCommandRspT *cmd_rsp,void *arg);
static void App_Light_Occupancy_client_report(struct ZbZclClusterT *clusterPtr,
  struct ZbApsdeDataIndT *dataIndPtr, uint16_t attributeId, enum ZclDataTypeT dataType,
  const uint8_t *in_payload, uint16_t in_len);

/* Clusters CFG -------------------------------------------------------------*/
/**
 * @brief  Configure and register cluster for endpoint dedicated
 * @param  zb Zigbee stack instance
 * @retval None
 */
Occupancy_Control_T * App_Light_Occupancy_Cfg(struct ZigBeeT *zb)
{
  app_Occupancy.occupancy_client = ZbZclOccupancyClientAlloc(zb, LIGHT_ENDPOINT);
  assert(app_Occupancy.occupancy_client != NULL);
  app_Occupancy.occupancy_client->report = &App_Light_Occupancy_client_report;
  assert(ZbZclClusterEndpointRegister(app_Occupancy.occupancy_client) != NULL);

  return  &app_Occupancy;
}

/* Occupancy Attribute Report config -----------------------------------------*/
/**
 * @brief  Configure the report for the Occupancy attribute at each modification on the server
 * 
 * @param  dst address configuration endpoint
 * @retval None
 */
void App_Light_Occupancy_ReportConfig(struct ZbApsAddrT * dst)
{
  enum   ZclStatusCodeT         status;
  struct ZbZclAttrReportConfigT reportCfg;

  /* Set Report Configuration  */
  memset( &reportCfg, 0, sizeof( reportCfg ) );
  
  reportCfg.dst = * dst; 
  reportCfg.num_records = 1;
  reportCfg.record_list[0].direction = ZCL_REPORT_DIRECTION_NORMAL;
  reportCfg.record_list[0].min       = LIGHT_OCCUPANCY_MIN_REPORT;    
  reportCfg.record_list[0].max       = LIGHT_OCCUPANCY_MAX_REPORT;
  reportCfg.record_list[0].change    = LIGHT_OCCUPANCY_REPORT_CHANGE;
  reportCfg.record_list[0].attr_id   = ZCL_OCC_ATTR_OCCUPANCY;
  reportCfg.record_list[0].attr_type = ZCL_DATATYPE_BITMAP_8BIT;
  
  APP_ZB_DBG("Send Occupancy Report Config ");
  status = ZbZclAttrReportConfigReq(app_Occupancy.occupancy_client, &reportCfg, &App_Light_Occupancy_ReportConfig_cb, NULL);
  if ( status != ZCL_STATUS_SUCCESS )
  {
    APP_ZB_DBG("Error during Report Config Request 0x%02X", status );
  }
} /* App_Light_Occupancy_ReportConfig */

/**
 * @brief  CallBack for the report configuration
 * @param  cmd_rsp response 
 * @param  arg unused
 * @retval None
 */
static void App_Light_Occupancy_ReportConfig_cb (struct ZbZclCommandRspT * cmd_rsp, void *arg)
{
  /* Report failed, TODO something ??? */
  if(cmd_rsp->status != ZCL_STATUS_SUCCESS)
  {
    APP_ZB_DBG("Report Occupancy Config Failed error: 0x%02X",cmd_rsp->status);
  }
  else
  {
    APP_ZB_DBG("Report Occupancy Config set with success");

    /* Could add the read attribute to launch now the functionnality of the application */
    // App_Light_Occupancy_Read_Attribute( &(cmd_rsp->src) );
  }
} /* App_Light_Occupancy_ReportConfig_cb */

/**
 * @brief  Report the modification of ZCL_ONOFF_ATTR_ONOFF and update locally the state
 * @param  clusterPtr
 * @retval None
 */
static void App_Light_Occupancy_client_report(struct ZbZclClusterT *clusterPtr,
  struct ZbApsdeDataIndT *dataIndPtr, uint16_t attributeId, enum ZclDataTypeT dataType,
  const uint8_t *in_payload, uint16_t in_len)
{
  int attrLen;

  /* Attribute reporting */
  if (attributeId == ZCL_OCC_ATTR_OCCUPANCY)
  {
    attrLen = ZbZclAttrParseLength(dataType, in_payload, dataIndPtr->asduLength, 0);
    if (attrLen < 0)
    {
      APP_ZB_DBG("Report error length 0");
      return;
    }
    if (attrLen > (int)in_len)
    {
      APP_ZB_DBG("Report error length >");
      return;
    }
    if (dataIndPtr->dst.endpoint != LIGHT_ENDPOINT)
    {
      APP_ZB_DBG("Report error wrong endpoint (%d)", dataIndPtr->dst.endpoint);
      return;
    }

    app_Occupancy.Occupancy = (uint8_t) in_payload[0];
    APP_ZB_DBG("Occupancy Report attribute From 0x%016llx -     %x",dataIndPtr->src.extAddr,app_Occupancy.Occupancy);

    /* Update the OnOff server according to the Sensor information */
    if (app_Occupancy.Occupancy)
    {
      (void) light_onoff_server_on_cb(app_Light_Control.app_OnOff->onoff_server, NULL, NULL);
    }
    else
    {
      (void) light_onoff_server_off_cb(app_Light_Control.app_OnOff->onoff_server, NULL, NULL);
    }
  }
} /* App_Light_Occupancy_client_report */

