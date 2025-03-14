/**
  ******************************************************************************
  * @file    app_core.c
  * @author  Zigbee Application Team
  * @brief   Application Core to centralize all functionnalities
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
#include "app_core.h"

/* Private includes ----------------------------------------------------------*/
#include "app_common.h"
#include "app_entry.h"
#include "shci.h"
#include "stm32_seq.h"
#include "stm32wbxx_core_interface_def.h"

#include "zigbee_types.h"
#include "zigbee_interface.h"

/* Debug Part */
#include <assert.h>
#include "stm_logging.h"
#include "dbg_trace.h"

/* service dependencies */
#include "app_zigbee.h"
#include "app_nvm.h"
#include "app_menu.h"
#include "app_onoff_sensor.h"

/* Private typedef -----------------------------------------------------------*/


/* Private defines -----------------------------------------------------------*/

/* External variables --------------------------------------------------------*/
extern App_Zb_Info_T app_zb_info;

/* timers definition */
static uint8_t TS_ID_LED_TOGGLE; /* LED Toggling delay */

/* Private functions prototypes-----------------------------------------------*/
/* Buttons/Touchkey management for the application */
static void App_SW1_Action       (void);
static void App_SW2_Action       (void);
static void App_SW3_Action       (void);
static void App_Core_UpdateButtonState(Button_TypeDef button, int isPressed);

/* Informations functions */
static void App_Core_Name_Disp      (void);
static void App_Core_LED_Toggle     (void);
static void App_Core_Leave_cb       (struct ZbNlmeLeaveConfT *conf, void *arg);


/* Functions Definition ------------------------------------------------------*/
/**
 * @brief Core of the Application to manage all services to use
 * 
 */
void App_Core_Init(void)
{
  APP_ZB_DBG("Initialisation");

  BSP_LED_Off(LED_RED);
  BSP_LED_Off(LED_GREEN);
  BSP_LED_Off(LED_BLUE);

  App_Core_Name_Disp();

  App_Zigbee_Init();

  /* Task associated with button Action */
  UTIL_SEQ_RegTask(1U << CFG_TASK_BUTTON_SW1, UTIL_SEQ_RFU, App_SW1_Action);
  UTIL_SEQ_RegTask(1U << CFG_TASK_BUTTON_SW2, UTIL_SEQ_RFU, App_SW2_Action );
  UTIL_SEQ_RegTask(1U << CFG_TASK_BUTTON_SW3, UTIL_SEQ_RFU, App_SW3_Action);

  /* Initialize Zigbee stack layers */
  App_Zigbee_StackLayersInit();

  /* init Tx power to the default value */
  App_Zigbee_TxPwr_Disp();

  /* Display informations after Join */
  if (app_zb_info.join_status == ZB_STATUS_SUCCESS)
    App_Zigbee_Channel_Disp();
  
  /* Menu Config */
  if (Menu_Config() == 0)
  {
    APP_ZB_DBG("Error : Menu Config");
  }
} /* App_Core_Init */

/**
 * @brief Configure the Device End points according to the clusters used
 *        Call during the StackLayerInit
 * 
 */
void App_Core_ConfigEndpoints(void)
{
  APP_ZB_DBG("Configure the Endpoints for the device");
  App_OnOff_Sensor_cfg_EndPoint(app_zb_info.zb);
} /* App_Core_ConfigEndpoints */

/**
 * @brief Configure the Device End points according to the clusters used
 *        Call during the StackLayerInit
 * 
 */
void App_Core_ConfigGroupAddr(void)
{
  APP_ZB_DBG("Configure the GroupAddr for the device");
  // App_OnOff_Sensor_ConfigGroupAddr();
} /* App_Core_ConfigGroupAddr */

/**
 * @brief  Restore the application state as read cluster attribute after a startup from persistence
 * @param  None
 * @retval None
 */
void App_Core_Restore_State(void)
{
  APP_ZB_DBG("Restore Endpoint informations for the application");
  App_OnOff_Sensor_Restore_State();  
} /* App_Core_Restore_State */


/* Informations functions -------------------------------------------------- */
/**
 * @brief  Application name display for debug
 * 
 * @param  None
 * @retval None
 */
static void App_Core_Name_Disp(void)
{
  /* Display Application information header */
  APP_ZB_DBG("RUC OnOff Sensor");
} /* App_Core_Name_Disp */

/**
 * @brief Clean and refresh the global informations on the application ongoing
 * To call from the Menu when the user wants to retrevie the base informations
 * 
 */
void App_Core_Infos_Disp(void)
{
  uint64_t ExtAddr = 0;
  APP_ZB_DBG("**********************************************************");
  App_Core_Name_Disp();
  App_Zigbee_Channel_Disp();
  App_Zigbee_TxPwr_Disp();
  App_Zigbee_Check_Firmware_Info();
  App_Zigbee_Bind_Disp();
  ExtAddr = ZbExtendedAddress(app_zb_info.zb);
  if (ExtAddr != NULL)
    APP_ZB_DBG("Extended Address : 0x%016llx", ExtAddr);
  APP_ZB_DBG("**********************************************************");
  Menu_Config();
} /* App_Core_Infos_Disp */

/**
 * @brief Wrapper to toggle Green LED
 * 
 */
static void App_Core_LED_Toggle(void)
{
  BSP_LED_Toggle(LED_GREEN);
} /* App_Core_LED_Toggle */


/* Network Actions ---------------------------------------------------------- */
/**
 * @brief Launch the Network joining by User Action
 */
void App_Core_Ntw_Join(void)
{
  APP_ZB_DBG("Launching Network Join");
  /* Timer associated to GREEN LED toggling */
  HW_TS_Create(CFG_TIM_PROC_ID_ISR, &TS_ID_LED_TOGGLE, hw_ts_Repeated, App_Core_LED_Toggle);
  HW_TS_Start(TS_ID_LED_TOGGLE, (uint32_t) HW_TS_LED_TOGGLE_DELAY);

  /* If Network joining was not successful reschedule the current task to retry the process */
  while (app_zb_info.join_status != ZB_STATUS_SUCCESS)
  {
    /* No Rejoin Network from persistence so decide todo it */
    UTIL_SEQ_SetTask(1U << CFG_TASK_ZIGBEE_NETWORK_JOIN, CFG_SCH_PRIO_0);
    UTIL_SEQ_WaitEvt(EVENT_ZIGBEE_NETWORK_JOIN);
  }
  
  HW_TS_Stop(TS_ID_LED_TOGGLE);
  /* Indicates successful join*/
  BSP_LED_Off(LED_BLUE);
  BSP_LED_Off(LED_GREEN);
  HAL_Delay(300);
  BSP_LED_On(LED_GREEN);
  HAL_Delay(300);
  BSP_LED_On(LED_BLUE);
  BSP_LED_Off(LED_GREEN);
  HAL_Delay(300);
  BSP_LED_Off(LED_BLUE);
  BSP_LED_On(LED_GREEN);
  HAL_Delay(300);
  BSP_LED_On(LED_BLUE);
  BSP_LED_Off(LED_GREEN);
  HAL_Delay(300);
  BSP_LED_Off(LED_BLUE);

  /* Display informations after Join */
  App_Zigbee_Channel_Disp();

  /* Assign ourselves to the group addresses */
  App_Core_ConfigGroupAddr();

  /* Since we're using group addressing (broadcast), shorten the broadcast timeout */
  uint32_t bcast_timeout = 3;
  ZbNwkSet(app_zb_info.zb, ZB_NWK_NIB_ID_NetworkBroadcastDeliveryTime, &bcast_timeout, sizeof(bcast_timeout));
} /* App_Core_Ntw_Join */

/**
 * @brief Reset the state of the device like Factory.
 * Unbind on devices locally know, leave the Zigbbe network, clean the flash and reboot the chip
 * 
 * @param  None
 * @retval None
 */
void App_Core_Factory_Reset(void)
{
  APP_ZB_DBG("Factory Reset");
  // use ZbLeaveReq before to send information to the network before leave it
  if (app_zb_info.join_status == ZB_STATUS_SUCCESS)
  {
    App_Zigbee_Unbind_All();
    ZbLeaveReq(app_zb_info.zb, &App_Core_Leave_cb, NULL);
  }
  App_Persist_Delete();
  /* Wait 2 sec before clear display */
  HAL_Delay(2000);
  NVIC_SystemReset();
} /* App_Core_Factory_Reset */

/**
 * @brief Call back after perform an NLME-LEAVE.request
 * 
 * @param conf 
 * @param arg unused
 */
static void App_Core_Leave_cb(struct ZbNlmeLeaveConfT *conf, void *arg)
{
  /* check if device left correctly/not the network */
  if (conf->status != ZB_STATUS_SUCCESS)
  {
    APP_ZB_DBG("Error %x during leave the network", conf->status);
  }
} /* App_Core_Leave_cb */


/* Buttons/Touchkey management for the application ------------------------- */
/**
 * @brief Wrapper to manage the short/long press
 * @param None
 * @retval None
 */
static void App_SW1_Action(void)
{
  App_Core_UpdateButtonState(BUTTON_SW1, BSP_PB_GetState(BUTTON_SW1) == BUTTON_PRESSED);
  return;
}
static void App_SW2_Action(void)
{
  App_Core_UpdateButtonState(BUTTON_SW2, BSP_PB_GetState(BUTTON_SW2) == BUTTON_PRESSED);
  return;
}
static void App_SW3_Action(void)
{
  App_Core_UpdateButtonState(BUTTON_SW3, BSP_PB_GetState(BUTTON_SW3) == BUTTON_PRESSED);
  return;
}

static void App_Core_UpdateButtonState(Button_TypeDef button, int isPressed)
{
  if (isPressed == false)
    return;
  
  uint32_t t0 = 0, press_delay = 0;
  t0 = HAL_GetTick(); /* button press timing */
  while(BSP_PB_GetState(button) == BUTTON_PRESSED && press_delay <= MAX_PRESS_DELAY)
    press_delay = HAL_GetTick() - t0;

  if ( (BSP_PB_GetState(BUTTON_SW1) == BUTTON_PRESSED) && (BSP_PB_GetState(BUTTON_SW3) == BUTTON_PRESSED) )
  {
    App_Core_Factory_Reset();
  }
  
  /* Manage Push button time for each button */
  switch (button)
  {
    case BUTTON_SW1:
      if (press_delay > MIDDLE_PRESS_DELAY)
      {
        /* exit current submenu and Up to previous Menu */
        Exit_Menu_Item();
      }
      else if (press_delay > DEBOUNCE_DELAY)
      {
        /* Change menu selection */        
        Prev_Menu_Item();       
      }

      break;

    case BUTTON_SW2:
      /* Not used beacause too hard to press */
      break;

    case BUTTON_SW3:     
      if (press_delay > MIDDLE_PRESS_DELAY)
      {
        Select_Menu_Item();
      }
      else if (press_delay > DEBOUNCE_DELAY)
      {
        /* Change menu selection */        
        Next_Menu_Item();       
      }
      break;
      
    default:
      APP_ZB_DBG("ERROR : Button Unknow");
      break;
  }
} /* App_Core_UpdateButtonState */

