/**
  ******************************************************************************
  * @file    app_light_switch.c
  * @author  Zigbee Application Team
  * @brief   Application interface for Light Switch EndPoint functionnality
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
Light_Switch_Control_T app_LightSwitch_Ctrl =
{
  .is_rdy_for_next_cmd = 0U,
  .force_read = false,
};

/* Finding&Binding function Declaration --------------------------------------*/
static void App_LightSwitch_FindBind_cb(enum ZbStatusCodeT status, void *arg);
static void App_LightSwitch_Bind_Nb    (void);

/* Retry function Declaration ------------------------------------------------*/
static void App_LightSwitch_Task_Retry_Cmd (void);

/* App Light Switch functions ------------------------------------------------*/
static void App_LightSwitch_Status_Led (void);

// Clusters CFG ----------------------------------------------------------------
/**
 * @brief  Configure and register Zigbee application endpoint, with corresponding clusters
 * composing the endpoint
 * 
 * @param  zb Zigbee stack instance
 * @retval None
 */
void App_LightSwitch_Cfg_Endpoint(struct ZigBeeT *zb)
{
  struct ZbApsmeAddEndpointReqT     req;
  struct ZbApsmeAddEndpointConfT    conf;
  
  /* Endpoint: LIGHT_SWITCH_ENDPOINT */
  memset(&req, 0, sizeof(req));
  req.profileId = ZCL_PROFILE_HOME_AUTOMATION;
  req.deviceId  = ZCL_DEVICE_ONOFF_SWITCH;
  req.endpoint  = LIGHT_SWITCH_ENDPOINT;
  ZbZclAddEndpoint(zb, &req, &conf);
  assert(conf.status == ZB_STATUS_SUCCESS);

  /* Idenfity client */
  app_LightSwitch_Ctrl.identify_client = ZbZclIdentifyClientAlloc(zb, LIGHT_SWITCH_ENDPOINT);
  assert(app_LightSwitch_Ctrl.identify_client != NULL);
  assert(ZbZclClusterEndpointRegister(app_LightSwitch_Ctrl.identify_client)!= NULL);

  /* OnOff Client */
  app_LightSwitch_Ctrl.app_OnOff_Control = App_Light_Switch_OnOff_cfg(zb);

  /*  LevelControl Client */
  app_LightSwitch_Ctrl.app_Level_Control = App_Light_Switch_Level_cfg(zb);

  /* make a local pointer of Zigbee stack to read easier */
  app_LightSwitch_Ctrl.zb = zb;

  /* Init retry process */
  UTIL_SEQ_RegTask(1U << (uint32_t) CFG_TASK_RETRY_PROC , UTIL_SEQ_RFU, App_LightSwitch_Task_Retry_Cmd );
  app_LightSwitch_Ctrl.cmd_retry.dst = (struct ZbApsAddrT *) malloc( sizeof(struct ZbApsAddrT) );

  /* Command status on LEDs */
  UTIL_SEQ_RegTask(1U << (uint32_t) CFG_TASK_LED_STATUS, UTIL_SEQ_RFU, App_LightSwitch_Status_Led);
} /* App_LightSwitch_Cfg_Endpoint */

/**
 * @brief  Associate the Endpoint to the group address (if needed)
 * 
 * @param  None
 * @retval None
 */
void App_LightSwitch_ConfigGroupAddr(void)
{
  struct ZbApsmeAddGroupReqT  req;
  struct ZbApsmeAddGroupConfT conf;
  
  memset(&req, 0, sizeof(req));
  req.endpt     = LIGHT_SWITCH_ENDPOINT;
  req.groupAddr = LIGHT_SWITCH_GROUP_ADDR;
  ZbApsmeAddGroupReq(app_LightSwitch_Ctrl.zb, &req, &conf);
} /* App_LightSwitch_ConfigGroupAddr */

/**
 * @brief  Restore the ONOFF state after persitance data loaded
 * 
 * @param  None
 * @retval None
 */
void App_LightSwitch_Restore_State(void)
{
  struct ZbApsmeBindT entry;

  // Recalculate the effective binding
  App_LightSwitch_Bind_Nb();
  APP_ZB_DBG("Restore state %2d bind", app_LightSwitch_Ctrl.app_OnOff_Control->bind_nb + app_LightSwitch_Ctrl.app_Level_Control->bind_nb);

  /* Browse binding table to retrieve attribute value */
  for (uint8_t i = 0; ; i++)
  {
    if (ZbApsGetIndex(app_LightSwitch_Ctrl.zb, ZB_APS_IB_ID_BINDING_TABLE, &entry, sizeof(entry), i) != ZB_APS_STATUS_SUCCESS)
    {
      break;
    }
    if (entry.srcExtAddr == 0ULL)
    {
      continue;
    }

    // Reread the attribute from server and synchronize the differents endpoint if needed
    switch (entry.clusterId) 
    {
      case ZCL_CLUSTER_ONOFF :
        App_Light_Switch_OnOff_Read_Attribute( &entry.dst );
        break;
    
      case ZCL_CLUSTER_LEVEL_CONTROL :
        App_Light_Switch_Level_Read_Attribute( &entry.dst );
        break;
        
      default :
        // APP_ZB_DBG("Try to restore unknown cluster ID : %d",entry.clusterId);
        break;
    } 
  }
} /* App_LightSwitch_Restore_State */


// FindBind actions ------------------------------------------------------------
/**
 * @brief  Start Finding and Binding process as an initiator.
 * Call App_LightSwitch_FindBind_cb when successfull to configure correctly the binding
 * 
 * @param  None
 * @retval None
 */
void App_LightSwitch_FindBind(void)
{
  uint8_t status = ZCL_STATUS_FAILURE;
  uint64_t epid = 0U;

  if (app_LightSwitch_Ctrl.zb == NULL)
  {
    APP_ZB_DBG("Error, zigbee stack not initialized");
    return;
  }

  /* Check if the router joined the network */
  if (ZbNwkGet(app_LightSwitch_Ctrl.zb, ZB_NWK_NIB_ID_ExtendedPanId, &epid, sizeof(epid)) != ZB_STATUS_SUCCESS)
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
  
  status = ZbStartupFindBindStart(app_LightSwitch_Ctrl.zb, &App_LightSwitch_FindBind_cb, NULL);

  if (status != ZB_STATUS_SUCCESS)
  {
    APP_ZB_DBG(" Error, cannot start Finding & Binding, status = 0x%02x", status);
  }
} /* App_LightSwitch_FindBind */

/**
 * @brief  Task called after F&B process to configure a report and update the local status.
 *         Will call the configuration report for the good clusterId.
 * @param  None
 * @retval None
 */
static void App_LightSwitch_FindBind_cb(enum ZbStatusCodeT status, void *arg)
{
  static uint8_t      retry = 0;

  // Retry if not succes
  if (status != ZB_STATUS_SUCCESS)
  {
    APP_ZB_DBG("Error while F&B | error code : 0x%02X", status);
    // Retry command if failed
    if (retry++ < MAX_RETRY_REPORT )
    {
      APP_ZB_DBG("Retry %d", retry);
      App_LightSwitch_Retry_Cmd (FIND_AND_BIND, NULL);
      return;
    }
  }
  else
  {
    struct ZbApsmeBindT entry;
  
    APP_ZB_DBG(" Item |   ClusterId | Long Address     | End Point");
    APP_ZB_DBG(" -----|-------------|------------------|----------");

    /* Loop only on the new binding element */
    for (uint8_t i = (app_LightSwitch_Ctrl.app_OnOff_Control->bind_nb + app_LightSwitch_Ctrl.app_Level_Control->bind_nb);; i++)
    {
      if (ZbApsGetIndex(app_LightSwitch_Ctrl.zb, ZB_APS_IB_ID_BINDING_TABLE, &entry, sizeof(entry), i) != ZB_APS_STATUS_SUCCESS)
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
          // adding a new binding item locally for onoff cluster
          memcpy(&app_LightSwitch_Ctrl.app_OnOff_Control->bind_table[app_LightSwitch_Ctrl.app_OnOff_Control->bind_nb++], &entry.dst, sizeof(struct ZbApsAddrT));
          // start report config proc
          App_Light_Switch_OnOff_ReportConfig( &entry.dst);
          break;
        
        case ZCL_CLUSTER_LEVEL_CONTROL :
          memcpy(&app_LightSwitch_Ctrl.app_Level_Control->bind_table[app_LightSwitch_Ctrl.app_Level_Control->bind_nb++], &entry.dst, sizeof(struct ZbApsAddrT));          
          App_Light_Switch_Level_ReportConfig( &entry.dst ); 
          break;
        
        default :
          break;
      }
    }
  
  APP_ZB_DBG("Binding entries created: %d", app_LightSwitch_Ctrl.app_OnOff_Control->bind_nb + app_LightSwitch_Ctrl.app_Level_Control->bind_nb);
  }
  /* Reset retry value */
  retry = 0;
} /* App_LightSwitch_FindBind_cb */

/**
 * @brief  Checks the number of valid entries in the local binding table and update the local attribut.
 * Filters it by clusterId, so if multiple clusters only the local clusters is taken
 * @param  None
 * @retval None
 */
static void App_LightSwitch_Bind_Nb(void)
{
  struct ZbApsmeBindT entry;
  
  app_LightSwitch_Ctrl.app_OnOff_Control->bind_nb    = 0;
  app_LightSwitch_Ctrl.app_Level_Control->bind_nb = 0;
  
   memset(&app_LightSwitch_Ctrl.app_OnOff_Control->bind_table   , 0, sizeof(uint64_t) * NB_OF_SERV_BINDABLE);
   memset(&app_LightSwitch_Ctrl.app_Level_Control->bind_table, 0, sizeof(uint64_t) * NB_OF_SERV_BINDABLE);

  /* Loop on the local binding table */
  for (uint8_t i = 0;; i++)
  {
    /* check the end of the table */
    if (ZbApsGetIndex(app_LightSwitch_Ctrl.zb, ZB_APS_IB_ID_BINDING_TABLE, &entry, sizeof(entry), i) != ZB_APS_STATUS_SUCCESS)
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
        memcpy(&app_LightSwitch_Ctrl.app_OnOff_Control->bind_table[app_LightSwitch_Ctrl.app_OnOff_Control->bind_nb++], &entry.dst, sizeof(struct ZbApsAddrT));        
        break;
      case ZCL_CLUSTER_LEVEL_CONTROL :
        memcpy(&app_LightSwitch_Ctrl.app_Level_Control->bind_table[app_LightSwitch_Ctrl.app_Level_Control->bind_nb++], &entry.dst, sizeof(struct ZbApsAddrT));        
        break;
      default:
        continue;
        break;
    }    
  }
} /* App_LightSwitch_Bind_Nb */

/**
 * @brief  For debug purpose, display the local binding table information
 * @param  None 
 * @retval None
 */
void App_LightSwitch_Bind_Disp(void)
{
  struct ZbApsmeBindT entry;
  APP_ZB_DBG("Binding Table has %d items", app_LightSwitch_Ctrl.app_OnOff_Control->bind_nb + app_LightSwitch_Ctrl.app_Level_Control->bind_nb);

  /* only if table not empty */
  if (app_LightSwitch_Ctrl.app_OnOff_Control->bind_nb + app_LightSwitch_Ctrl.app_Level_Control->bind_nb > 0)
  {
  APP_ZB_DBG(" -----------------------------------------------------------------");
  APP_ZB_DBG(" Item |   Long Address   | ClusterId | Src Endpoint | Dst Endpoint");
  APP_ZB_DBG(" -----|------------------|-----------|--------------|-------------");

    /* Loop on the Binding Table */
    for (uint8_t i = 0;; i++)
    {
      if (ZbApsGetIndex(app_LightSwitch_Ctrl.zb, ZB_APS_IB_ID_BINDING_TABLE, &entry, sizeof(entry), i) != ZB_APS_STATUS_SUCCESS)
      {
        break;
      }
      if (entry.srcExtAddr == 0ULL)
      {
        continue;
      }
      APP_ZB_DBG("  %2d  | %016llx |   0x%04x  |    0x%04x    |    0x%04x", i, entry.dst.extAddr, entry.clusterId, entry.srcEndpt, entry.dst.endpoint);
    }
    
    /* For OnOff cluster */
    if (app_LightSwitch_Ctrl.app_OnOff_Control->bind_nb > 0)
    {
      APP_ZB_DBG("\r");
      APP_ZB_DBG(" ----- ClusterId 0x006 : ON OFF -------");
      APP_ZB_DBG(" Item |   Long Address   | Dst Endpoint");
      APP_ZB_DBG(" -----|------------------|-------------");

      /* Loop on the Binding Table */
      for (uint8_t i = 0;i < app_LightSwitch_Ctrl.app_OnOff_Control->bind_nb; i++)
      {
        APP_ZB_DBG("  %2d  | %016llx |  0x%04x", i, app_LightSwitch_Ctrl.app_OnOff_Control->bind_table[i].extAddr, app_LightSwitch_Ctrl.app_OnOff_Control->bind_table[i].endpoint);
      }
    }  
    
    /* For Lvl Ctrl cluster */
    if (app_LightSwitch_Ctrl.app_Level_Control->bind_nb > 0)
    {
      APP_ZB_DBG("\r");
      APP_ZB_DBG(" ----- ClusterId 0x008 : LVL CTRL -----");
      APP_ZB_DBG(" Item |   Long Address   | Dst Endpoint");
      APP_ZB_DBG(" -----|------------------|-------------");

      /* Loop on the Binding Table */
      for (uint8_t i = 0; i < app_LightSwitch_Ctrl.app_Level_Control->bind_nb; i++)
      {
        APP_ZB_DBG("  %2d  | %016llx |  0x%04x", i, app_LightSwitch_Ctrl.app_Level_Control->bind_table[i].extAddr, app_LightSwitch_Ctrl.app_Level_Control->bind_table[i].endpoint);
      }
    }    
    APP_ZB_DBG(" -----------------------------------------------------------------");
  }   
} /* App_LightSwitch_Bind_Disp */

// -----------------------------------------------------------------------------
// Retry functions -------------------------------------------------------------
/**
 * @brief  Relaunch previously failed cmd
 * @param  None 
 * @retval None
 */
static void App_LightSwitch_Task_Retry_Cmd (void)
{     
  switch (app_LightSwitch_Ctrl.cmd_retry.cmd_type)
  {
    case FIND_AND_BIND :
      App_LightSwitch_FindBind();
      break;        
    case REPORT_CONF_ONOFF :
      App_Light_Switch_OnOff_ReportConfig( app_LightSwitch_Ctrl.cmd_retry.dst );
      break;
    case READ_ONOFF :
      App_Light_Switch_OnOff_Read_Attribute( app_LightSwitch_Ctrl.cmd_retry.dst );
      break;
    case WRITE_ONOFF :
      App_Light_Switch_OnOff_Cmd( app_LightSwitch_Ctrl.cmd_retry.dst );
      break;
    case REPORT_CONF_LVL :
      App_Light_Switch_Level_ReportConfig( app_LightSwitch_Ctrl.cmd_retry.dst );
      break;    
    case READ_LVL :
      App_Light_Switch_Level_Read_Attribute( app_LightSwitch_Ctrl.cmd_retry.dst );
      break;    
    case WRITE_LVL :
      App_Light_Switch_Level_Cmd( app_LightSwitch_Ctrl.cmd_retry.dst );
      break;    
    default :
      APP_ZB_DBG("The command :%d haven't been implemented yet",app_LightSwitch_Ctrl.cmd_retry.cmd_type);
      break;
  }

  app_LightSwitch_Ctrl.cmd_retry.cmd_type = IDLE;
  return;
} /* App_LightSwitch_Task_Retry_Cmd */

/**
 * @brief  Add retry task to avoid recursivity
 * @param  Command type 
 * @param  Target addresse
 * @retval None
 */
void App_LightSwitch_Retry_Cmd (Cmd_Type_T cmd, const struct ZbApsAddrT * dst)
{
  /* Check if last command as been executed */
  if (app_LightSwitch_Ctrl.cmd_retry.cmd_type == IDLE)
  {
    app_LightSwitch_Ctrl.cmd_retry.cmd_type = cmd;
    memcpy(app_LightSwitch_Ctrl.cmd_retry.dst, dst,sizeof(struct ZbApsAddrT ));
    UTIL_SEQ_SetTask(1U << CFG_TASK_RETRY_PROC, CFG_SCH_PRIO_0);
  }
  else
  {
    APP_ZB_DBG("Trying to add %x cmd while already one hasn't been cleared : %x",cmd,app_LightSwitch_Ctrl.cmd_retry.cmd_type);
  }
} /* App_LightSwitch_Retry_Cmd */

/**
 * @brief  Get the retry number of a command for a dedicated server
 * @param  None 
 * @retval retry_nb corresponding to server
 */
uint8_t * Get_retry_nb (Retry_number_T retry_tab[SIZE_RETRY_TAB], uint64_t server_addr)
{
  /* Check in retry_tab if this serv has already failed this cmd */
  for (uint8_t i = 0 ; i < NB_OF_SERV_BINDABLE; i++)
  {
    if ( (server_addr == retry_tab[i].ext_addr) || (retry_tab[i].ext_addr == NULL))
    {
      return &retry_tab[i].retry_nb;
    }
  }
  
  APP_ZB_DBG("Didn't find the corresponding binded server");
  return &retry_tab[RETRY_ERROR_INDEX].retry_nb;
} /* Get_retry_nb */

/* Light control device ------------------------------------------------------*/
/**
 * @brief Wrapper function to control locally the Light remote
 * Calling by Menu
 * 
 */
void App_LightSwitch_Toggle(void)
{
  App_Light_Switch_OnOff_Cmd(NULL);  
} /* App_LightSwitch_Toggle */

void App_LightSwitch_Level_Up(void)
{
  app_LightSwitch_Ctrl.app_Level_Control->level_mode = MODE_UP;
  App_Light_Switch_Level_Cmd(NULL);  
} /* App_LightSwitch_Level_Up */

void App_LightSwitch_Level_Down(void)
{
  app_LightSwitch_Ctrl.app_Level_Control->level_mode = MODE_DOWN;
  App_Light_Switch_Level_Cmd(NULL);
} /* App_LightSwitch_Level_Down */

/* App function -------------------------------------------------------------*/
/**
 * @brief  Select the Led to display level control
 * @param  None
 * @retval None
 */
static void App_LightSwitch_Status_Led (void)
{
  APP_ZB_DBG("Display Lvl : %d | 0x%x",app_LightSwitch_Ctrl.app_OnOff_Control->On, app_LightSwitch_Ctrl.app_Level_Control->level);
  if (app_LightSwitch_Ctrl.app_OnOff_Control->On)
  {
    switch(app_LightSwitch_Ctrl.app_Level_Control->level)
    {
      /* Level Off */
      case 0x00 :
        BSP_LED_Off(LED_BLUE);
        BSP_LED_Off(LED_GREEN);
        BSP_LED_Off(LED_RED);
        break;      
      /* Level Min : Blue Led On */
      case 0x3F :
        BSP_LED_On (LED_BLUE);
        BSP_LED_Off(LED_GREEN);
        BSP_LED_Off(LED_RED);
        break;
      /* Level mid- : Green Led On */
      case 0x7F :
        BSP_LED_Off(LED_BLUE);
        BSP_LED_On (LED_GREEN);
        BSP_LED_Off(LED_RED);
        break;   
      /* Level mid+ : Red Led On */
      case 0xBF :
        BSP_LED_Off(LED_BLUE);
        BSP_LED_Off(LED_GREEN);
        BSP_LED_On (LED_RED);
        break;
      /* Level max : All Led On */
      case 0xFF :
        BSP_LED_On(LED_BLUE);
        BSP_LED_On(LED_GREEN);
        BSP_LED_On(LED_RED);
        break;
      default :
        //APP_ZB_DBG("Unknown Level : 0x%x", app_LightSwitch_Ctrl.level);
        break;
    }
  }
  /* else command off : All Led Off */
  else
  {
  BSP_LED_Off(LED_BLUE);
  BSP_LED_Off(LED_GREEN);
  BSP_LED_Off(LED_RED);
  }
} /* App_LightSwitch_Status_Led */

