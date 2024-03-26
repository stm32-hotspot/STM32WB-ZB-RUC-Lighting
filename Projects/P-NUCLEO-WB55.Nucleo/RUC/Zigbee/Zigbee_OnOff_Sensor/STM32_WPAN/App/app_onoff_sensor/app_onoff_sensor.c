/**
  ******************************************************************************
  * @file    app_onoff.c
  * @author  Zigbee Application Team
  * @brief   Application interface for OnOff functionnality
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
#include "app_onoff_sensor.h"

#include "stm32_seq.h"

/* HW TimerServer definition */
#define PIR_REFRESH_DELAY           5
#define HW_TS_PIR_REFRESH_DELAY     (PIR_REFRESH_DELAY * HW_TS_SERVER_1S_NB_TICKS)
static uint8_t TS_ID_PIR_REFRESH;

/* Application Variable-------------------------------------------------------*/
OnOff_Control_T app_OnOff_Control =
{
  .On = 0U,
  .is_on_init = 0U,  
  .cmd_send = 0U,
};

/* External variables --------------------------------------------------------*/

/* OnOff callbacks Declaration --------------------------------------------- */
static void App_OnOff_Sensor_FindBind_cb    (enum ZbStatusCodeT status, void *arg);
static void App_OnOff_Sensor_Bind_Nb        (void);
static void App_OnOff_Sensor_ReportConfig_cb(struct ZbZclCommandRspT *cmd_rsp,void *arg);
static void App_OnOff_Sensor_client_report  (struct ZbZclClusterT *clusterPtr,
  struct ZbApsdeDataIndT *dataIndPtr, uint16_t attributeId, enum ZclDataTypeT dataType,
  const uint8_t *in_payload, uint16_t in_len);
static void App_OnOff_Sensor_Read_cb        (const ZbZclReadRspT *readRsp, void *arg);
static void App_OnOff_Sensor_Cmd_cb         (struct ZbZclCommandRspT *rsp, void *arg);
static void App_OnOff_Sensor_Detect     (void);
static void App_OnOff_Sensor_Refresh    (void);

// Clusters CFG ----------------------------------------------------------------
/**
 * @brief  Configure and register cluster for endpoint dedicated
 * 
 * @param  zb Zigbee stack instance
 * @retval None
 */
void App_OnOff_Sensor_cfg_EndPoint(struct ZigBeeT *zb)
{
  struct ZbApsmeAddEndpointReqT     req;
  struct ZbApsmeAddEndpointConfT    conf;
  
  /* Endpoint: LIGHT_SWITCH_ENDPOINT */
  memset(&req, 0, sizeof(req));
  req.profileId = ZCL_PROFILE_HOME_AUTOMATION;
  req.deviceId  = ZCL_DEVICE_ONOFF_SENSOR;
  req.endpoint  = ONOFF_SENSOR_ENDPOINT;
  ZbZclAddEndpoint(zb, &req, &conf);
  assert(conf.status == ZB_STATUS_SUCCESS);

  /* Idenfity client */
  app_OnOff_Control.identify_client = ZbZclIdentifyClientAlloc(zb, ONOFF_SENSOR_ENDPOINT);
  assert(app_OnOff_Control.identify_client != NULL);
  assert(ZbZclClusterEndpointRegister(app_OnOff_Control.identify_client)!= NULL);

  /* OnOff Client */
  app_OnOff_Control.onoff_client = ZbZclOnOffClientAlloc(zb, ONOFF_SENSOR_ENDPOINT);
  assert(app_OnOff_Control.onoff_client != NULL);
  app_OnOff_Control.onoff_client->report = &App_OnOff_Sensor_client_report;
  assert(ZbZclClusterEndpointRegister(app_OnOff_Control.onoff_client) != NULL);

  /* make a local pointer of Zigbee stack to read easier */
  app_OnOff_Control.zb = zb;

  /* Initialise PIR to use with CLuster */
  BSP_PIR_Init(BUTTON_MODE_EXTI);
  UTIL_SEQ_RegTask(TASK_BUTTON_PIR, UTIL_SEQ_RFU, App_OnOff_Sensor_Detect);

  /* prepare timer to refresh PIR status */
  HW_TS_Create(CFG_TIM_PIR_REFRESH, &TS_ID_PIR_REFRESH, hw_ts_Repeated, App_OnOff_Sensor_Refresh);
} /* App_OnOff_Sensor_cfg_EndPoint */

/**
 * @brief  Associate the Endpoint to the group address (if needed)
 * 
 */
void App_OnOff_Sensor_ConfigGroupAddr(void)
{
  struct ZbApsmeAddGroupReqT  req;
  struct ZbApsmeAddGroupConfT conf;
  
  memset(&req, 0, sizeof(req));
  req.endpt     = ONOFF_SENSOR_ENDPOINT;
  req.groupAddr = ONOFF_SENSOR_GROUP_ADDR;
  ZbApsmeAddGroupReq(app_OnOff_Control.zb, &req, &conf);
} /* App_OnOff_Sensor_ConfigGroupAddr */

/**
 * @brief  Restore the state after persitance data loaded
 * 
 */
void App_OnOff_Sensor_Restore_State(void)
{
  struct ZbApsmeBindT entry;

  // Recalculate the effective binding
  App_OnOff_Sensor_Bind_Nb();
  APP_ZB_DBG("Restore state %2d bind", app_OnOff_Control.bind_nb);

  /* Check the values on server */
  for (uint8_t i = 0;; i++)
  {
    /* check the end of the table */
    if (ZbApsGetIndex(app_OnOff_Control.zb, ZB_APS_IB_ID_BINDING_TABLE, &entry, sizeof(entry), i) != ZB_APS_STATUS_SUCCESS)
    {
      break;
    }
    /* remove itself */
    if (entry.srcExtAddr == 0ULL)
    {
      continue;
    }

    /* do not reference useless cluster */
    if (entry.clusterId == ZCL_CLUSTER_ONOFF )
    {
      App_OnOff_Sensor_Read_Attribute( &entry.dst );
    }
  }
} /* App_Window_Covering_Restore_State */


// FindBind actions ------------------------------------------------------------
/**
 * @brief  Start Finding and Binding process as an initiator.
 * Call App_OnOff_Sensor_FindBind_cb when successfull to configure correctly the binding
 * 
 * @param  None
 * @retval None
 */
void App_OnOff_Sensor_FindBind(void)
{
  uint8_t status = ZCL_STATUS_FAILURE;
  uint64_t epid = 0U;

  if (app_OnOff_Control.zb == NULL)
  {
    APP_ZB_DBG("Error, zigbee stack not initialized");
    return;
  }

  /* Check if the router joined the network */
  if (ZbNwkGet(app_OnOff_Control.zb, ZB_NWK_NIB_ID_ExtendedPanId, &epid, sizeof(epid)) != ZB_STATUS_SUCCESS)
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
  status = ZbStartupFindBindStart(app_OnOff_Control.zb, &App_OnOff_Sensor_FindBind_cb, NULL);

  if (status != ZB_STATUS_SUCCESS)
  {
    APP_ZB_DBG(" Error, cannot start Finding & Binding, status = 0x%02x", status);
  }
} /* App_OnOff_Sensor_FindBind */

/**
 * @brief  Task called after F&B process to configure a report and update the local status.
 *         Will call the configuration report for the good clusterId.
 * @param  None
 * @retval None
 */
static void App_OnOff_Sensor_FindBind_cb(enum ZbStatusCodeT status, void *arg)
{
  if (status != ZB_STATUS_SUCCESS)
  {
    APP_ZB_DBG("Error while F&B | error code : 0x%02X", status);
  }
  else
  {
    struct ZbApsmeBindT entry;
  
    APP_ZB_DBG(" Item |   ClusterId | Long Address     | End Point");
    APP_ZB_DBG(" -----|-------------|------------------|----------");

    /* Loop only on the new binding element */
    for (uint8_t i = app_OnOff_Control.bind_nb ;; i++)
    {
      if (ZbApsGetIndex(app_OnOff_Control.zb, ZB_APS_IB_ID_BINDING_TABLE, &entry, sizeof(entry), i) != ZB_APS_STATUS_SUCCESS)
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
        case ZCL_CLUSTER_ONOFF :
          // adding a new binding item locally for My_Cluster cluster
          memcpy(&app_OnOff_Control.bind_table[app_OnOff_Control.bind_nb++], &entry.dst, sizeof(struct ZbApsAddrT));
          // start report config proc
          App_OnOff_Sensor_ReportConfig( &entry.dst);
          break;

        default:
          APP_ZB_DBG("F&B WRONG CLUSTER 0x%04x", entry.clusterId);
          break;
      }
    }
  
  APP_ZB_DBG("Binding entries created: %d", app_OnOff_Control.bind_nb);
  }
} /* App_OnOff_Sensor_FindBind_cb */

/**
 * @brief  Checks the number of valid entries in the local binding table and update the local attribut.
 * Filters it by clusterId, so if multiple clusters only the local clusters is taken
 * @param  None
 * @retval None
 */
static void App_OnOff_Sensor_Bind_Nb(void)
{
  struct ZbApsmeBindT entry;
  
  app_OnOff_Control.bind_nb = 0;
  
  memset(&app_OnOff_Control.bind_table, 0, sizeof(uint64_t) * NB_OF_SERV_BINDABLE);

  /* Loop on the local binding table */
  for (uint8_t i = 0;; i++)
  {
    /* check the end of the table */
    if (ZbApsGetIndex(app_OnOff_Control.zb, ZB_APS_IB_ID_BINDING_TABLE, &entry, sizeof(entry), i) != ZB_APS_STATUS_SUCCESS)
    {
      break;
    }
    /* remove itself */
    if (entry.srcExtAddr == 0ULL)
    {
      continue;
    }

    /* do not reference useless cluster */
    switch (entry.clusterId)
    {
      case ZCL_CLUSTER_ONOFF :
        /* locally increment bind number of selected cluster and add extended addr to corresponding local binding table */
        memcpy(&app_OnOff_Control.bind_table[app_OnOff_Control.bind_nb++], &entry.dst, sizeof(struct ZbApsAddrT));        
        break;

      default:
        continue;
        break;
    }    
  }
} /* App_OnOff_Sensor_Bind_Nb */

/**
 * @brief  For debug purpose, display the local binding table information
 * @param  None 
 * @retval None
 */
void App_OnOff_Sensor_Bind_Disp(void)
{
  APP_ZB_DBG("Binding Table has %d items", app_OnOff_Control.bind_nb);
    
  /* only if My_Cluster Table not empty */
  if (app_OnOff_Control.bind_nb > 0)
  {
    printf("\n\n\r");
    APP_ZB_DBG("--------------------------------------------------");
    APP_ZB_DBG("Item |   Long Address   | ClusterId | Dst Endpoint");
    APP_ZB_DBG("-----|------------------|-----------|-------------");

    /* Loop on the Binding Table */
    for (uint8_t i = 0;i < app_OnOff_Control.bind_nb; i++)
    {
      APP_ZB_DBG("  %2d  | %016llx |   Window  |  0x%04x", i, app_OnOff_Control.bind_table[i].extAddr, app_OnOff_Control.bind_table[i].endpoint);
    }  
    
    APP_ZB_DBG("--------------------------------------------------\n\r");  
  } 
} /* App_OnOff_Sensor_Bind_Disp */


// -----------------------------------------------------------------------------
// OnOff Attribute Report config -----------------------------------------------
/**
 * @brief  Configure the report for the OnOff attribute at each modification on the server
 * 
 * @param  dst address configuration endpoint
 * @retval None
 */
void App_OnOff_Sensor_ReportConfig(struct ZbApsAddrT * dst)
{
  enum   ZclStatusCodeT         status;
  struct ZbZclAttrReportConfigT reportCfg;

  /* Set Report Configuration  */
  memset( &reportCfg, 0, sizeof( reportCfg ) );
  
  reportCfg.dst = * dst; 
  reportCfg.num_records = 1;
  reportCfg.record_list[0].direction = ZCL_REPORT_DIRECTION_NORMAL;
  reportCfg.record_list[0].min       = ONOFF_SENSOR_MIN_REPORT;    
  reportCfg.record_list[0].max       = ONOFF_SENSOR_MAX_REPORT;
  reportCfg.record_list[0].change    = ONOFF_SENSOR_REPORT_CHANGE;
  reportCfg.record_list[0].attr_id   = ZCL_ONOFF_ATTR_ONOFF;
  reportCfg.record_list[0].attr_type = ZCL_DATATYPE_BOOLEAN;
  
  APP_ZB_DBG("Send OnOff Report Config ");
  status = ZbZclAttrReportConfigReq(app_OnOff_Control.onoff_client, &reportCfg, &App_OnOff_Sensor_ReportConfig_cb, NULL);
  if ( status != ZCL_STATUS_SUCCESS )
  {
    APP_ZB_DBG("Error during Report Config Request 0x%02X", status );
  }
} /* App_OnOff_Sensor_ReportConfig */

/**
 * @brief  CallBack for the report configuration
 * @param  cmd_rsp response 
 * @param  arg unused
 * @retval None
 */
static void App_OnOff_Sensor_ReportConfig_cb (struct ZbZclCommandRspT * cmd_rsp, void *arg)
{  
  /* Report failed, launch retry process */
  if(cmd_rsp->status != ZCL_STATUS_SUCCESS)
  {
    APP_ZB_DBG("Report OnOff Config Failed error: 0x%02X",cmd_rsp->status);
  }
  else
  {
    APP_ZB_DBG("Report OnOff Config set with success");
    App_OnOff_Sensor_Read_Attribute( &(cmd_rsp->src) );
  }
} /* App_OnOff_Sensor_ReportConfig_cb */

/**
 * @brief  Report the modification of ZCL_ONOFF_ATTR_ONOFF and update locally the state
 * @param  clusterPtr
 * @retval None
 */
static void App_OnOff_Sensor_client_report(struct ZbZclClusterT *clusterPtr,
  struct ZbApsdeDataIndT *dataIndPtr, uint16_t attributeId, enum ZclDataTypeT dataType,
  const uint8_t *in_payload, uint16_t in_len)
{
  int attrLen;

  /* Attribute reporting */
  if (attributeId == ZCL_ONOFF_ATTR_ONOFF)
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
    if (dataIndPtr->dst.endpoint != ONOFF_SENSOR_ENDPOINT)
    {
      APP_ZB_DBG("Report error wrong endpoint (%d)", dataIndPtr->dst.endpoint);
      return;
    }

    app_OnOff_Control.On = (uint8_t) in_payload[0];
    app_OnOff_Control.cmd_send = app_OnOff_Control.On;
    APP_ZB_DBG("OnOff Report attribute From %016llx -     %x",dataIndPtr->src.extAddr,app_OnOff_Control.On);     
  }
  // else
  // {
  //   APP_ZB_DBG("Repport error Bad Attribute 0x%04x", attributeId);
  // }
} /* App_OnOff_Sensor_client_report */


/* OnOff Read Attribute  --------------------------------------------------- */
/**
 * @brief Read OTA Attribute to update the local status
 * @param  target addresse
 * @retval None
 */
void App_OnOff_Sensor_Read_Attribute(struct ZbApsAddrT * dst)
{
  ZbZclReadReqT readReq;
  uint64_t epid = 0U;
  enum ZclStatusCodeT rd_status;

  /* Check that the Zigbee stack initialised */
  if(app_OnOff_Control.zb == NULL)
  {
    return;
  }  
  /* Check if the device joined the network */
  if (ZbNwkGet(app_OnOff_Control.zb, ZB_NWK_NIB_ID_ExtendedPanId, &epid, sizeof(epid)) != ZB_STATUS_SUCCESS)
  {
    return;
  }
  if (epid == 0U)
  {
    return;
  }

  memset(&readReq, 0, sizeof(readReq));
  readReq.dst     = *dst;
  readReq.count   = 1U;
  readReq.attr[0] = ZCL_ONOFF_ATTR_ONOFF ;
   
  APP_ZB_DBG("Read the OnOff Attribute");
  rd_status = ZbZclReadReq(app_OnOff_Control.onoff_client, &readReq, App_OnOff_Sensor_Read_cb, dst);
  if ( rd_status != ZCL_STATUS_SUCCESS )
  {
    APP_ZB_DBG("Error during onoff read request status : 0x%02X", rd_status );
  }
} /* App_OnOff_Sensor_Read_Attribute */

/**
 * @brief  Read OTA OnOff attribute callback
 * @param  read rsp
 * @retval None
 */
static void App_OnOff_Sensor_Read_cb(const ZbZclReadRspT * cmd_rsp, void * arg)
{
  /* Read failed, launch retry process */
  if (cmd_rsp->status != ZCL_STATUS_SUCCESS)
  {   
    APP_ZB_DBG("Error, Read cmd failed | status : 0x%x", cmd_rsp->status);
  }
    
  if (cmd_rsp->count == 0)
  {
    APP_ZB_DBG("Error, No attribute read");
  }
  /* Server is new in the network, check if there is a conflict value batween attribute value store in client and in server */
  else 
  {
    app_OnOff_Control.On = (uint8_t) * cmd_rsp->attr[0].value;
    APP_ZB_DBG("Read OnOff      attribute From %016llx -     %d", cmd_rsp->src.extAddr,app_OnOff_Control.On);
  }  
} /* App_OnOff_Sensor_Read_cb */

/* OnOff send cmd ----------------------------------------------------------- */
/**
 * @brief  OnOff pushed toggle req send
 * @param  target, if NULL send cmd to all server bind
 * @retval None
 */
void App_OnOff_Sensor_Toggle_Cmd(void)
{
  uint64_t epid = 0U;
  enum ZclStatusCodeT cmd_status;
  
  /* Check that the Zigbee stack initialised */
  if(app_OnOff_Control.zb == NULL)
  {
    return;
  }
  
  /* Check if the device joined the network */
  if (ZbNwkGet(app_OnOff_Control.zb, ZB_NWK_NIB_ID_ExtendedPanId, &epid, sizeof(epid)) != ZB_STATUS_SUCCESS)
  {
    return;
  }

  if (epid == 0U)
  {
    return;
  }
  
  /* Browse binding table to find target */
  for (uint8_t i = 0; i < app_OnOff_Control.bind_nb; i++)
  {
    /* Check value to send the correct command and not only Toggle */
    if  (app_OnOff_Control.On)
    {
      APP_ZB_DBG("CMD SENDING LED OFF");
      cmd_status = ZbZclOnOffClientOffReq(app_OnOff_Control.onoff_client, &app_OnOff_Control.bind_table[i] , &App_OnOff_Sensor_Cmd_cb, NULL);
    }
    else
    {
      APP_ZB_DBG("CMD SENDING LED ON");
      cmd_status = ZbZclOnOffClientOnReq(app_OnOff_Control.onoff_client, &app_OnOff_Control.bind_table[i], &App_OnOff_Sensor_Cmd_cb, NULL);
    }    
  }  

  /* check status of command request send to the Server */
  if (cmd_status != ZCL_STATUS_SUCCESS)
  {
    APP_ZB_DBG("Error, ZbZclOnOffClientOn/OffReq failed : 0x%x",cmd_status);
  }
} /* App_OnOff_Sensor_Toggle_Cmd */

/**
 * @brief  CallBack for the on off cmd
 * @param  command response 
 * @param  arg passed trough cb function
 * @retval None
 */
static void App_OnOff_Sensor_Cmd_cb (struct ZbZclCommandRspT * cmd_rsp, void *arg)
{
  if ((cmd_rsp->aps_status != ZB_STATUS_SUCCESS) && (cmd_rsp->status != ZCL_STATUS_SUCCESS))
  {
    APP_ZB_DBG("client  0x%016llx didn't responded from On_Off cmd | aps_status : 0x%02x | zcl_status : 0x%02x",cmd_rsp->src.extAddr, cmd_rsp->aps_status, cmd_rsp->status);
  }
  else
  {
    APP_ZB_DBG("Response cb from On_Off cmd from client :  0x%016llx ", cmd_rsp->src.extAddr);
  }
} /* App_OnOff_Sensor_Cmd_cb */

/**
 * @brief Task when the PIR detect occupancy
 * 
 */
static void App_OnOff_Sensor_Detect(void)
{
  APP_ZB_DBG("Sensor detection start");
  if (!app_OnOff_Control.On)
    App_OnOff_Sensor_Toggle_Cmd();

  /* Timer to refresh the PIR status  */
  HW_TS_Start(TS_ID_PIR_REFRESH, HW_TS_PIR_REFRESH_DELAY);
} /* App_OnOff_Sensor_Detect */

/**
 * @brief Refresh the PIR status and executes according by the application
 * 
 */
static void App_OnOff_Sensor_Refresh(void)
{
  uint32_t value = BSP_PIR_GetState();
  if (!value)
  {
    APP_ZB_DBG("Sensor detection end");
    if (app_OnOff_Control.On)
      App_OnOff_Sensor_Toggle_Cmd();

    HW_TS_Stop(TS_ID_PIR_REFRESH);
  }
} /* App_OnOff_Sensor_Refresh */

