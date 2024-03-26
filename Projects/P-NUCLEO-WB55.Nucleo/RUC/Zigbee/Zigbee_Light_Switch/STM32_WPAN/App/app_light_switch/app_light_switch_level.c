/**
  ******************************************************************************
  * @file    app_light_switch_level.c
  * @author  Zigbee Application Team
  * @brief   Application interface for Level cluster of Light Switch EndPoint.
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
static Level_Control_T app_Level_Control =
{
  .level         = 0xFFU,
  .is_level_init = 0U,  
  .cmd_send      = 0xFFU,
};

/* External variables --------------------------------------------------------*/
extern Light_Switch_Control_T app_LightSwitch_Ctrl;

/* Light_Switch_Level callbacks Declaration --------------------------------------------- */
static void App_Light_Switch_Level_ReportConfig_cb(struct ZbZclCommandRspT *cmd_rsp,void *arg);
static void App_Light_Switch_Level_client_report(struct ZbZclClusterT *clusterPtr,
  struct ZbApsdeDataIndT *dataIndPtr, uint16_t attributeId, enum ZclDataTypeT dataType,
  const uint8_t *in_payload, uint16_t in_len);
static void App_Light_Switch_Level_Read_cb(const ZbZclReadRspT *readRsp, void *arg);
static void App_Light_Switch_Level_Cmd_cb (struct ZbZclCommandRspT *cmd_rsp, void *arg);

// Clusters CFG ----------------------------------------------------------------
/**
 * @brief  Configure and register cluster for endpoint dedicated
 * 
 * @param  zb Zigbee stack instance
 * @return Level_Control_T* for EndPoint control
 */
Level_Control_T * App_Light_Switch_Level_cfg(struct ZigBeeT *zb)
{
  app_Level_Control.levelControl_client = ZbZclLevelClientAlloc(zb, LIGHT_SWITCH_ENDPOINT);
  assert(app_Level_Control.levelControl_client != NULL);
  app_Level_Control.levelControl_client->report = &App_Light_Switch_Level_client_report;
  assert(ZbZclClusterEndpointRegister(app_Level_Control.levelControl_client) != NULL);

  return  &app_Level_Control;
} /* App_Light_Switch_Level_cfg */


/* Level Attribute Report --------------------------------------------------- */
/**
 * @brief  Configure the report for the Light_Switch_Level attribute
 *         at each value modified on the server
 * @param  Target addresse
 * @retval None
 */
void App_Light_Switch_Level_ReportConfig(struct ZbApsAddrT * dst)
{
  enum ZclStatusCodeT           status;
  struct ZbZclAttrReportConfigT reportCfg;

  /* Set Report Configuration  */
  memset( &reportCfg, 0, sizeof( reportCfg ) );
  
  reportCfg.dst = *dst; 
  reportCfg.num_records = 1;
  reportCfg.record_list[0].direction = ZCL_REPORT_DIRECTION_NORMAL;
  reportCfg.record_list[0].min       = LIGHT_SWITCH_LEVEL_MIN_REPORT;    
  reportCfg.record_list[0].max       = LIGHT_SWITCH_LEVEL_MAX_REPORT;
  reportCfg.record_list[0].change    = LIGHT_SWITCH_LEVEL_REPORT_CHANGE;
  reportCfg.record_list[0].attr_id   = ZCL_LEVEL_ATTR_CURRLEVEL;
  reportCfg.record_list[0].attr_type = ZCL_DATATYPE_UNSIGNED_8BIT;
  
  APP_ZB_DBG("Send Level Control Report Config ");
  status = ZbZclAttrReportConfigReq(app_Level_Control.levelControl_client, &reportCfg, &App_Light_Switch_Level_ReportConfig_cb, (void *) dst);
  if ( status != ZCL_STATUS_SUCCESS )
  {
    APP_ZB_DBG("Error during level ctrl Report Config Request 0x%02X", status );
  }
} /* App_Light_Switch_Level_ReportConfig */

/**
 * @brief  CallBack for the report configuration
 * @param  None
 * @retval None
 */
static void App_Light_Switch_Level_ReportConfig_cb(struct ZbZclCommandRspT *cmd_rsp, void *arg)
{
  /* retry_tab store all previous retry from all server. It's composed 
       of NB_OF_SERV_BINDABLE + 1 extra item in case of error */
  static Retry_number_T retry_tab[SIZE_RETRY_TAB] ;
  
  /* Retrieve the corresponding rety_nb with the specified extAddr */
  uint8_t * retry = Get_retry_nb (retry_tab, cmd_rsp->src.extAddr);
  
  /* Report failed, launch retry process */
  if(cmd_rsp->status != ZCL_STATUS_SUCCESS)
  {
    APP_ZB_DBG("Report Light_Switch_Level Config Failed error: 0x%02X",cmd_rsp->status);
    /* Check if max retry have already be reached */
    if ((*retry)++ < MAX_RETRY_REPORT )
    {    
      APP_ZB_DBG("Retry %d | report lvl ctrl config 0x%016llx", (*retry), cmd_rsp->src.extAddr);
      /* Launch retry process */
      App_LightSwitch_Retry_Cmd (REPORT_CONF_LVL, &(cmd_rsp->src));
      return;       
    }
    else
    {
      /* Max retry have already be reached */
      APP_ZB_DBG("Exceed max retry for ReportConfig cmd to 0x%016llx", cmd_rsp->src.extAddr);
    }
  }    
  else
  {  
    APP_ZB_DBG("Report Level Control Config set with success");
    
    /* Retrieve current value after binding*/
    App_Light_Switch_Level_Read_Attribute( &(cmd_rsp->src));
  }
  
  /* Reset retry counter */
  (*retry) = 0;
} /* App_Light_Switch_Level_ReportConfig_cb */

/**
 * @brief  Report the modification of ZCL_LEVEL_ATTR_CURRLEVEL and update locally the state
 * @param  clusterPtr
 * @retval None
 */
static void App_Light_Switch_Level_client_report(struct ZbZclClusterT *clusterPtr,
  struct ZbApsdeDataIndT *dataIndPtr, uint16_t attributeId, enum ZclDataTypeT dataType,
  const uint8_t *in_payload, uint16_t in_len)
{
  int attrLen;

  /* Attribute reporting */
  if (attributeId == ZCL_LEVEL_ATTR_CURRLEVEL)
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

    app_Level_Control.level = (uint8_t) in_payload[0];
    app_Level_Control.cmd_send = app_Level_Control.level;    
    APP_ZB_DBG("Level report attribute From %016llx -     0x%x",dataIndPtr->src.extAddr,app_Level_Control.level);
    
    /* Modif App Led State from the value read in nvm */
    UTIL_SEQ_SetTask(1U << CFG_TASK_LED_STATUS, CFG_SCH_PRIO_1);
  }
} /* App_Light_Switch_Level_client_report */

/**
 * @brief Read OTA Attribute to update the local status
 * @param  None
 * @retval None
 */
void App_Light_Switch_Level_Read_Attribute(struct ZbApsAddrT *dst)
{
  ZbZclReadReqT readReq;
  uint64_t epid = 0U;
  enum ZclStatusCodeT rd_status;

  /* Check that the Zigbee stack initialised */
  if(app_Level_Control.levelControl_client->zb == NULL)
  {
    return;
  }  
  /* Check if the device joined the network */
  if (ZbNwkGet(app_Level_Control.levelControl_client->zb, ZB_NWK_NIB_ID_ExtendedPanId, &epid, sizeof(epid)) != ZB_STATUS_SUCCESS)
  {
    return;
  }
  if (epid == 0U)
  {
    return;
  }

  memset(&readReq, 0, sizeof(readReq));
  readReq.dst     = * dst;
  readReq.count   = 1U;
  readReq.attr[0] = ZCL_LEVEL_ATTR_CURRLEVEL ;
   
  APP_ZB_DBG("Read the Level Ctrl Attribute");
  rd_status = ZbZclReadReq(app_Level_Control.levelControl_client, &readReq, App_Light_Switch_Level_Read_cb, NULL);
  if (rd_status != ZCL_STATUS_SUCCESS)
  {
    APP_ZB_DBG("Error, ZbZclReadReq failed %d", LIGHT_SWITCH_ENDPOINT);
  }
} /* App_Light_Switch_Level_Read_Attribute */

/**
 * @brief  Read OTA Light_Switch_Level attribute callback
 * @param  read rsp
 * @retval None
 */
static void App_Light_Switch_Level_Read_cb(const ZbZclReadRspT * cmd_rsp, void * arg)
{
  /* retry_tab store all previous retry from all server. It's composed 
       of NB_OF_SERV_BINDABLE + 1 extra item in case of error */
  static Retry_number_T retry_tab[SIZE_RETRY_TAB] ;
  
  /* Retrieve the corresponding rety_nb with the specified extAddr */
  uint8_t * retry = Get_retry_nb (retry_tab, cmd_rsp->src.extAddr);

  if (cmd_rsp->status != ZCL_STATUS_SUCCESS)
  {   
    /* Read failed, launch retry process */    
    APP_ZB_DBG("Error, Read level cmd failed | status : 0x%x", cmd_rsp->status);
    if ((*retry)++ < MAX_RETRY_REPORT )
    {    
      APP_ZB_DBG("Retry %d | read lvl ctrl 0x%016llx", (*retry), cmd_rsp->src.extAddr);
      /* Launch retry process */      
      App_LightSwitch_Retry_Cmd (READ_LVL, &(cmd_rsp->src));
      return;    
    }
    else
    {
      /* Max retry have already be reached */
      APP_ZB_DBG("Exceed max retry for level Read cmd to 0x%016llx", cmd_rsp->src.extAddr);
    }        
  }
  
  if (cmd_rsp->count == 0)
  {
    APP_ZB_DBG("Error, No attribute read");
  }
  
  else if (app_Level_Control.is_level_init == false)
  {
    app_Level_Control.is_level_init = true;
    app_Level_Control.level = (uint8_t) * cmd_rsp->attr[0].value;
    app_Level_Control.cmd_send = app_Level_Control.level;
    APP_ZB_DBG("Init Level value : %d ",app_Level_Control.level);
  }
  else 
  {
    APP_ZB_DBG("Read Level Ctrl attribute From %016llx -     0x%x",cmd_rsp->src.extAddr,app_Level_Control.level);
    if (app_LightSwitch_Ctrl.force_read == true)
    {
      app_Level_Control.level = (uint8_t) * cmd_rsp->attr[0].value;
      app_Level_Control.cmd_send = app_Level_Control.level;
      app_LightSwitch_Ctrl.force_read = false;
    }    
    else if (app_LightSwitch_Ctrl.app_OnOff_Control->On != (uint8_t) * cmd_rsp->attr[0].value)
    {
      APP_ZB_DBG("Conflict between current Client and Server");
      
      /* Take the semaphore */
      app_LightSwitch_Ctrl.is_rdy_for_next_cmd ++;
      app_Level_Control.level_mode = MODE_NOCHANGE;
      
      /* Send to all server the good value */
      /* Need to do memcpy because of const */
      struct ZbApsAddrT * dst;
      dst = malloc (sizeof(struct ZbApsAddrT));
      memcpy(dst,&(cmd_rsp->src),sizeof(struct ZbApsAddrT));
      for (uint8_t i=0; i < app_Level_Control.bind_nb; i++)
      {
        if (cmd_rsp->src.extAddr == app_Level_Control.bind_table[i].extAddr)
        {
          App_Light_Switch_OnOff_Cmd( &app_Level_Control.bind_table[i] );
          break;
        }
      }
    }
    else
    {
      APP_ZB_DBG("No Conflict between current Client and Server");
    }    
  }
  
  /* Reset retry value */
  (*retry) = 0;   

  /* Modif App Led State from the value read in nvm */
    UTIL_SEQ_SetTask(1U << CFG_TASK_LED_STATUS, CFG_SCH_PRIO_1);
} /* App_Light_Switch_Level_Read_cb */

/* Level actions ----------------------------------------------------------- */
/**
 * @brief  Light_Switch_Level pushed toggle req send 
 * @param  None
 * @retval None
 */
void App_Light_Switch_Level_Cmd(struct ZbApsAddrT * dst) 
{
  uint64_t epid = 0U;
  enum ZclStatusCodeT cmd_status;
  struct ZbZclLevelClientMoveToLevelReqT req;
  
  /* Check that the Zigbee stack initialised */
  if( app_Level_Control.levelControl_client->zb == NULL)
  {
    return;
  }
  
  /* Check if the device joined the network */
  if (ZbNwkGet( app_Level_Control.levelControl_client->zb, ZB_NWK_NIB_ID_ExtendedPanId, &epid, sizeof(epid)) != ZB_STATUS_SUCCESS)
  {
    return;
  }

  if (epid == 0U)
  {
    return;
  }
  
  req.with_onoff = false;
  req.transition_time = 0;   
  req.mask = 0;
  req.override = 0;
      
   /* No target specified, send cmd to all server binded */
  if ( dst == NULL )
  {
    /* First check if the semaphore is available */
    if ( app_LightSwitch_Ctrl.is_rdy_for_next_cmd == 0 )
    {
      if ( app_Level_Control.cmd_send != app_Level_Control.level)
      {
        APP_ZB_DBG("Error last level repport non received current lvl : 0x%x | last lvl send : 0x%x", app_Level_Control.level, app_Level_Control.cmd_send);
      }      
      
      switch (app_Level_Control.level_mode)
      {
      case MODE_DOWN :
        if (app_Level_Control.level > LIGHT_SWITCH_LEVEL_MIN_VALUE)
        {
          APP_ZB_DBG("Level Ctrl Down : 0x%x --> 0x%x",app_Level_Control.level,app_Level_Control.level - LIGHT_SWITCH_LEVEL_STEP);
          req.level = app_Level_Control.level - LIGHT_SWITCH_LEVEL_STEP;
        }
        else
        {
          APP_ZB_DBG("Min Level already reached");
          return;
        }
        break;
        
      case MODE_UP :
        if (app_Level_Control.level == LIGHT_SWITCH_LEVEL_MAX_VALUE)
        {
          APP_ZB_DBG("Max Level already reached");
          return;
        }
        /* Check if overflow happens */
        else if ((uint8_t) (app_Level_Control.level + LIGHT_SWITCH_LEVEL_STEP) < app_Level_Control.level)
        {
          APP_ZB_DBG("Level Ctrl Up : 0x%x --> 0xff",app_Level_Control.level);
          req.level = LIGHT_SWITCH_LEVEL_MAX_VALUE ;
        }
        else
        {
          APP_ZB_DBG("Level Ctrl Up : 0x%x --> 0x%x",app_Level_Control.level,app_Level_Control.level + LIGHT_SWITCH_LEVEL_STEP);
          req.level = app_Level_Control.level + LIGHT_SWITCH_LEVEL_STEP;
        }
        break;
        
      case MODE_NOCHANGE :
        APP_ZB_DBG("To unblock onoff, send same lvl : 0x%x", app_Level_Control.cmd_send);
        req.level = app_Level_Control.cmd_send;
        break;
        
      default :
        APP_ZB_DBG("Eror: Unknown level command mode : %d",app_Level_Control.level_mode);
        return;
        break;
      }
      
      app_Level_Control.cmd_send = req.level;
      
      /* Browse binding table to find target */
      for (uint8_t i = 0; i < app_Level_Control.bind_nb; i++)
      {
        cmd_status = ZbZclLevelClientMoveToLevelReq( app_Level_Control.levelControl_client, &app_Level_Control.bind_table[i], &req, &App_Light_Switch_Level_Cmd_cb, NULL);    

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
    req.level = app_Level_Control.cmd_send;    
    cmd_status = ZbZclLevelClientMoveToLevelReq( app_Level_Control.levelControl_client, dst,&req, &App_Light_Switch_Level_Cmd_cb, NULL);    
  }
  
   /* check status of command request send to the Server */
  if (cmd_status != ZCL_STATUS_SUCCESS)
  {
    APP_ZB_DBG("Error, ZbZclLight_Switch_LevelClient Req failed : 0x%x",cmd_status);
  }
} /* App_Light_Switch_Level_Cmd */

/**
 * @brief  CallBack for the level cmd
 * @param  command response 
 * @param  arg passed trough cb function
 * @retval None
 */
static void App_Light_Switch_Level_Cmd_cb (struct ZbZclCommandRspT *cmd_rsp, void *arg)
{
  /* retry_tab store all previous retry from all server. It's composed 
       of NB_OF_SERV_BINDABLE + 1 extra item in case of error */
  static Retry_number_T retry_tab[SIZE_RETRY_TAB] ;
  
  /* Retrieve the corresponding rety_nb with the specified extAddr */
  uint8_t * retry = Get_retry_nb (retry_tab, cmd_rsp->src.extAddr);

  if ((cmd_rsp->aps_status != ZB_STATUS_SUCCESS) && (cmd_rsp->status != ZCL_STATUS_SUCCESS))
  {
    APP_ZB_DBG("client 0x%016llx didn't responded from Level_Ctrl cmd | aps_status : 0x%02x  | zcl_status : 0x%02x",cmd_rsp->src.extAddr, cmd_rsp->aps_status, cmd_rsp->status);
    if ( (*retry)++ < MAX_RETRY_CMD )
    {
      APP_ZB_DBG("Retry %d | lvl ctrl cmd to 0x%016llx", (*retry), cmd_rsp->src.extAddr);
      App_LightSwitch_Retry_Cmd (WRITE_LVL, &(cmd_rsp->src));
      return;
    }
    else
    {
      APP_ZB_DBG("Exceed max retry for sending level_ctrl cmd to 0x%016llx", cmd_rsp->src.extAddr);
    }
  }
  else
  {
    APP_ZB_DBG("Response cb from Level_Ctrl cmd from client :  0x%016llx", cmd_rsp->src.extAddr); 
  }

  /* Reset retry value */
  (*retry) = 0;
  
  /* Release the semaphore */
  app_LightSwitch_Ctrl.is_rdy_for_next_cmd --;  
} /* App_Light_Switch_Level_Cmd_cb */

