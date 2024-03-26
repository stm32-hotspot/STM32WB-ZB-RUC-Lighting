/**
  ******************************************************************************
  * @file    app_light_switch_onoff.c
  * @author  Zigbee Application Team
  * @brief   Application interface for OnOff cluster of Light Switch EndPoint.
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
#include "stm32_seq.h"
#include "app_light_switch_cfg.h"

/* Application Variable-------------------------------------------------------*/
OnOff_Control_T app_OnOff_Control =
{
  .On = 0U,
  .is_on_init = 0U,  
  .cmd_send = 0U,
};

/* External variables --------------------------------------------------------*/
extern Light_Switch_Control_T app_LightSwitch_Ctrl;

/* OnOff callbacks Declaration --------------------------------------------- */
static void App_Light_Switch_OnOff_ReportConfig_cb(struct ZbZclCommandRspT *cmd_rsp,void *arg);
static void App_Light_Switch_OnOff_client_report(struct ZbZclClusterT *clusterPtr,
  struct ZbApsdeDataIndT *dataIndPtr, uint16_t attributeId, enum ZclDataTypeT dataType,
  const uint8_t *in_payload, uint16_t in_len);
static void App_Light_Switch_OnOff_Read_cb(const ZbZclReadRspT *readRsp, void *arg);
static void App_Light_Switch_OnOff_Cmd_cb (struct ZbZclCommandRspT *rsp, void *arg);

// Clusters CFG ----------------------------------------------------------------
/**
 * @brief  Configure and register cluster for endpoint dedicated
 * 
 * @param  zb Zigbee stack instance
 * @retval OnOff_Control_T* endpoint control
 */
OnOff_Control_T * App_Light_Switch_OnOff_cfg(struct ZigBeeT *zb)
{
  app_OnOff_Control.onoff_client = ZbZclOnOffClientAlloc(zb, LIGHT_SWITCH_ENDPOINT);
  assert(app_OnOff_Control.onoff_client != NULL);
  app_OnOff_Control.onoff_client->report = &App_Light_Switch_OnOff_client_report;
  assert(ZbZclClusterEndpointRegister(app_OnOff_Control.onoff_client) != NULL);

  return  &app_OnOff_Control;
} /* App_Light_Switch_OnOff_cfg */

// -----------------------------------------------------------------------------
// OnOff Attribute Report config -----------------------------------------------
/**
 * @brief  Configure the report for the OnOff attribute at each modification on the server
 * 
 * @param  dst address configuration endpoint
 * @retval None
 */
void App_Light_Switch_OnOff_ReportConfig(struct ZbApsAddrT * dst)
{
  enum   ZclStatusCodeT         status;
  struct ZbZclAttrReportConfigT reportCfg;

  /* Set Report Configuration  */
  memset( &reportCfg, 0, sizeof( reportCfg ) );
  
  reportCfg.dst = * dst; 
  reportCfg.num_records = 1;
  reportCfg.record_list[0].direction = ZCL_REPORT_DIRECTION_NORMAL;
  reportCfg.record_list[0].min       = LIGHT_SWITCH_ONOFF_MIN_REPORT;    
  reportCfg.record_list[0].max       = LIGHT_SWITCH_ONOFF_MAX_REPORT;
  reportCfg.record_list[0].change    = LIGHT_SWITCH_ONOFF_REPORT_CHANGE;
  reportCfg.record_list[0].attr_id   = ZCL_ONOFF_ATTR_ONOFF;
  reportCfg.record_list[0].attr_type = ZCL_DATATYPE_BOOLEAN;
  
  APP_ZB_DBG("Send OnOff Report Config ");
  status = ZbZclAttrReportConfigReq(app_OnOff_Control.onoff_client, &reportCfg, &App_Light_Switch_OnOff_ReportConfig_cb, NULL);
  if ( status != ZCL_STATUS_SUCCESS )
  {
    APP_ZB_DBG("Error during Report Config Request 0x%02X", status );
  }
} /* App_Light_Switch_OnOff_ReportConfig */

/**
 * @brief  CallBack for the report configuration
 * @param  cmd_rsp response 
 * @param  arg unused
 * @retval None
 */
static void App_Light_Switch_OnOff_ReportConfig_cb (struct ZbZclCommandRspT * cmd_rsp, void *arg)
{
  /* retry_tab store all previous retry from all server. It's composed 
       of NB_OF_SERV_BINDABLE + 1 extra item in case of error */
  static Retry_number_T retry_tab[SIZE_RETRY_TAB] ;
  
  /* Retrieve the corresponding rety_nb with the specified extAddr */
  uint8_t * retry = Get_retry_nb (retry_tab, cmd_rsp->src.extAddr);
  
  /* Report failed, launch retry process */
  if(cmd_rsp->status != ZCL_STATUS_SUCCESS)
  {
    APP_ZB_DBG("Report OnOff Config Failed error: 0x%02X",cmd_rsp->status);
    /* Check if max retry have already been reached */
    if ((*retry)++ < MAX_RETRY_REPORT )
    {
      APP_ZB_DBG("Retry %d | report onoff config 0x%016llx", (*retry), cmd_rsp->src.extAddr);
      /* Launch retry process */
      App_LightSwitch_Retry_Cmd(REPORT_CONF_ONOFF, &(cmd_rsp->src));
      return;
    }
    else
    {
      /* Max retry reached */
      APP_ZB_DBG("Exceed max retry for ReportConfig cmd to 0x%016llx", cmd_rsp->src.extAddr);
    }
  }
  else
  {
    APP_ZB_DBG("Report OnOff Config set with success");
    App_Light_Switch_OnOff_Read_Attribute( &(cmd_rsp->src) );
  }
  
  /* Reset retry counter */
  (*retry) = 0;  
} /* App_Light_Switch_OnOff_ReportConfig_cb */

/**
 * @brief  Report the modification of ZCL_ONOFF_ATTR_ONOFF and update locally the state
 * @param  clusterPtr
 * @retval None
 */
static void App_Light_Switch_OnOff_client_report(struct ZbZclClusterT *clusterPtr,
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
    if (dataIndPtr->dst.endpoint != LIGHT_SWITCH_ENDPOINT)
    {
      APP_ZB_DBG("Report error wrong endpoint (%d)", dataIndPtr->dst.endpoint);
      return;
    }

    app_OnOff_Control.On = (uint8_t) in_payload[0];
    app_OnOff_Control.cmd_send = app_OnOff_Control.On;
    APP_ZB_DBG("OnOff Report attribute From %016llx -     %x",dataIndPtr->src.extAddr,app_OnOff_Control.On); 
    
    /* Modif App Led State from the value read in nvm */
    UTIL_SEQ_SetTask(1U << CFG_TASK_LED_STATUS, CFG_SCH_PRIO_1);
  }
} /* App_Light_Switch_OnOff_client_report */

/* OnOff Read Attribute  --------------------------------------------------- */
/**
 * @brief Read OTA Attribute to update the local status
 * @param  target addresse
 * @retval None
 */
void App_Light_Switch_OnOff_Read_Attribute(struct ZbApsAddrT * dst)
{
  ZbZclReadReqT readReq;
  uint64_t epid = 0U;
  enum ZclStatusCodeT rd_status;

  /* Check that the Zigbee stack initialised */
  if(app_OnOff_Control.onoff_client->zb == NULL)
  {
    return;
  }  
  /* Check if the device joined the network */
  if (ZbNwkGet(app_OnOff_Control.onoff_client->zb, ZB_NWK_NIB_ID_ExtendedPanId, &epid, sizeof(epid)) != ZB_STATUS_SUCCESS)
  {
    return;
  }
  if (epid == 0U)
  {
    return;
  }

  /* Create the read request for the attribut */
  memset(&readReq, 0, sizeof(readReq));
  readReq.dst     = *dst;
  readReq.count   = 1U;
  readReq.attr[0] = ZCL_ONOFF_ATTR_ONOFF ;
   
  APP_ZB_DBG("Read the OnOff Attribute");
  rd_status = ZbZclReadReq(app_OnOff_Control.onoff_client, &readReq, App_Light_Switch_OnOff_Read_cb, dst);
  if ( rd_status != ZCL_STATUS_SUCCESS )
  {
    APP_ZB_DBG("Error during onoff read request status : 0x%02X", rd_status );
  }
} /* App_Light_Switch_OnOff_Read_Attribute */

/**
 * @brief  Read OTA OnOff attribute callback
 * @param  read rsp
 * @retval None
 */
static void App_Light_Switch_OnOff_Read_cb(const ZbZclReadRspT * cmd_rsp, void * arg)
{
  /* retry_tab store all previous retry from all server. It's composed 
       of NB_OF_SERV_BINDABLE + 1 extra item in case of error */
  static Retry_number_T retry_tab[SIZE_RETRY_TAB] ;
  
  /* Retrieve the corresponding rety_nb with the specified extAddr */
  uint8_t * retry = Get_retry_nb (retry_tab, cmd_rsp->src.extAddr);
  
  /* Read failed, launch retry process */
  if (cmd_rsp->status != ZCL_STATUS_SUCCESS)
  {   
    APP_ZB_DBG("Error, Read cmd failed | status : 0x%x", cmd_rsp->status);
    /* Check if max retry have already been reached */
    if ( (*retry)++ < MAX_RETRY_REPORT )
    {    
      APP_ZB_DBG("Retry %d | read onoff 0x%016llx", (*retry), cmd_rsp->src.extAddr);
      /* Launch retry process */
      App_LightSwitch_Retry_Cmd (READ_ONOFF,  &(cmd_rsp->src));
      return;    
    }
    else
    {
      /* Max retry reached */
      APP_ZB_DBG("Exceed max retry for Read cmd to 0x%016llx", cmd_rsp->src.extAddr);
    }    
  }
    
  if (cmd_rsp->count == 0)
  {
    APP_ZB_DBG("Error, No attribute read");
  }
  /* Does this client has already read/send a value from/to a server */
  else if (app_OnOff_Control.is_on_init == false)
  {
    /* Init has to be done */
    app_OnOff_Control.is_on_init = true;
    app_OnOff_Control.On = (uint8_t) * cmd_rsp->attr[0].value;
    APP_ZB_DBG("Init value | On Off : %d", app_OnOff_Control.On);
    app_OnOff_Control.cmd_send = app_OnOff_Control.On;   
    
  }
  /* Server is new in the network, check if there is a conflict value batween attribute value store in client and in server */
  else 
  {
    APP_ZB_DBG("Read OnOff      attribute From %016llx -     %d", cmd_rsp->src.extAddr,app_OnOff_Control.On);
    if (app_LightSwitch_Ctrl.force_read == true)
    {
      app_OnOff_Control.On = (uint8_t) * cmd_rsp->attr[0].value;
      app_OnOff_Control.cmd_send = app_OnOff_Control.On;
      app_LightSwitch_Ctrl.force_read = false;
      
    }
    if (app_OnOff_Control.On != (uint8_t) * cmd_rsp->attr[0].value)
    {    
      APP_ZB_DBG("Conflict between current Client and Server");
      
      /* Take the semaphore */
      app_LightSwitch_Ctrl.is_rdy_for_next_cmd ++; 
       
      /* Send to all server the good value */
      /* Need to do memcpy because of const */
      struct ZbApsAddrT * dst;
      dst = malloc (sizeof(struct ZbApsAddrT));
      memcpy(dst,&(cmd_rsp->src),sizeof(struct ZbApsAddrT));
      App_Light_Switch_OnOff_Cmd( dst );
    }   
    else
    {
      APP_ZB_DBG("No Conflict between current Client and Server");
    }
  }
  
  /* Reset retry counter */
  (*retry) = 0;
  
  /* Modif App Led State */
  UTIL_SEQ_SetTask(1U << CFG_TASK_LED_STATUS, CFG_SCH_PRIO_1);
} /* App_Light_Switch_OnOff_Read_cb */

/* OnOff send cmd ----------------------------------------------------------- */
/**
 * @brief  OnOff pushed toggle req send
 * @param  target, if NULL send cmd to all server bind
 * @retval None
 */
void App_Light_Switch_OnOff_Cmd(struct ZbApsAddrT * dst)
{
  uint64_t epid = 0U;
  enum ZclStatusCodeT cmd_status;
  
  /* Check that the Zigbee stack initialised */
  if(app_OnOff_Control.onoff_client->zb == NULL)
  {
    return;
  }
  
  /* Check if the device joined the network */
  if (ZbNwkGet(app_OnOff_Control.onoff_client->zb, ZB_NWK_NIB_ID_ExtendedPanId, &epid, sizeof(epid)) != ZB_STATUS_SUCCESS)
  {
    return;
  }

  if (epid == 0U)
  {
    return;
  }
  
  /* No target specified, send cmd to all server binded */
  if ( dst == NULL )
  {
    /* First check if the semaphore is available */
    if ( app_LightSwitch_Ctrl.is_rdy_for_next_cmd == 0 )
    {
      if ( app_OnOff_Control.cmd_send != app_OnOff_Control.On)
      {
        APP_ZB_DBG("Error last onoff repport non received ");
        app_LightSwitch_Ctrl.force_read = true;
        App_Light_Switch_OnOff_Read_Attribute( &(app_OnOff_Control.bind_table[0]) );
        App_LightSwitch_Retry_Cmd (READ_ONOFF, NULL);
        return;
      }      
      
      /* Browse binding table to find target */
      for (uint8_t i = 0; i < app_OnOff_Control.bind_nb; i++)
      {
        /* Check value to send the correct command and not only Toggle */
        if  (app_OnOff_Control.On)
        {
          APP_ZB_DBG("CMD SENDING LED OFF");
          cmd_status = ZbZclOnOffClientOffReq(app_OnOff_Control.onoff_client, &app_OnOff_Control.bind_table[i] , &App_Light_Switch_OnOff_Cmd_cb, NULL);
          app_OnOff_Control.cmd_send = 0;
        }
        else
        {
          APP_ZB_DBG("CMD SENDING LED ON");
          cmd_status = ZbZclOnOffClientOnReq(app_OnOff_Control.onoff_client, &app_OnOff_Control.bind_table[i], &App_Light_Switch_OnOff_Cmd_cb, NULL);
          app_OnOff_Control.cmd_send = 1;
        }
       
        app_OnOff_Control.is_on_init = true;
        
        /* Take the semaphore */
        app_LightSwitch_Ctrl.is_rdy_for_next_cmd ++;      
      }  
    }
    else 
    {
      APP_ZB_DBG("W8 for all acknwoledge");
    }
  }
  else
  {
    /* Check value to send the correct command and not only Toggle */
    if  ( app_OnOff_Control.cmd_send )
    {
      APP_ZB_DBG("CMD SENDING LED ON");
      cmd_status = ZbZclOnOffClientOnReq(app_OnOff_Control.onoff_client, dst, &App_Light_Switch_OnOff_Cmd_cb, NULL);      
    }
    else
    {
      APP_ZB_DBG("CMD SENDING LED OFF");
      cmd_status = ZbZclOnOffClientOffReq(app_OnOff_Control.onoff_client, dst , &App_Light_Switch_OnOff_Cmd_cb, NULL);
    }
  }

  /* check status of command request send to the Server */
  if (cmd_status != ZCL_STATUS_SUCCESS)
  {
    APP_ZB_DBG("Error, ZbZclOnOffClientOn/OffReq failed : 0x%x",cmd_status);
  }
} /* App_Light_Switch_OnOff_Cmd */

/**
 * @brief  CallBack for the on off cmd
 * @param  command response 
 * @param  arg passed trough cb function
 * @retval None
 */
static void App_Light_Switch_OnOff_Cmd_cb (struct ZbZclCommandRspT * cmd_rsp, void *arg)
{
  /* retry_tab store all previous retry from all server. It's composed 
       of NB_OF_SERV_BINDABLE + 1 extra item in case of error */
  static Retry_number_T retry_tab[SIZE_RETRY_TAB] ;
  
  /* Retrieve the corresponding rety_nb with the specified extAddr */
  uint8_t * retry = Get_retry_nb (retry_tab, cmd_rsp->src.extAddr);

  if ((cmd_rsp->aps_status != ZB_STATUS_SUCCESS) && (cmd_rsp->status != ZCL_STATUS_SUCCESS))
  {
    APP_ZB_DBG("client  0x%016llx didn't responded from On_Off cmd | aps_status : 0x%02x | zcl_status : 0x%02x",cmd_rsp->src.extAddr, cmd_rsp->aps_status, cmd_rsp->status);
    if ( (*retry)++ < MAX_RETRY_CMD )
    {
      APP_ZB_DBG("Retry %d | onoff cmd to 0x%016llx", (*retry) ,cmd_rsp->src.extAddr);
      App_LightSwitch_Retry_Cmd (WRITE_ONOFF, &(cmd_rsp->src));
      return;
    }
    else
    {
      APP_ZB_DBG("Exceed max retry for sending led cmd to 0x%016llx", cmd_rsp->src.extAddr);
  
    }
  }
  else
  {
    APP_ZB_DBG("Response cb from On_Off cmd from client :  0x%016llx ", cmd_rsp->src.extAddr);
  }

  
  /* Reset retry number */
   (*retry) = 0;
  
  /* Release the semaphore */
  app_LightSwitch_Ctrl.is_rdy_for_next_cmd --;
} /* App_Light_Switch_OnOff_Cmd_cb */

