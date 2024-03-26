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

/* Includes ------------------------------------------------------------------*/
#include "app_common.h"
#include "app_entry.h"
#include "app_zigbee.h"
#include "shci.h"
#include "stm32_seq.h"
#include "stm32wbxx_core_interface_def.h"

#include "zigbee_types.h"
#include "zigbee_interface.h"

/* board dependancies */
#include "stm32wb5mm_dk_lcd.h"
#include "stm32_lcd.h"

/* Debug Part */
#include <assert.h>
#include "stm_logging.h"
#include "dbg_trace.h"

/* service dependencies */
#include "app_zigbee.h"
#include "app_nvm.h"
#include "app_menu.h"
#include "app_light_cfg.h"

/* Private defines -----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/
typedef struct 
{
  /* Zigbee stack */
  struct ZigBeeT * zb;

  /* data */
  App_Zb_Info_T app_zb_info;
} App_Core_Info_T;

/* External variables ------------------------------------------------------- */
extern App_Zb_Info_T app_zb_info;

/* timer server ID definitions */
uint8_t TS_ID_CLEAN_STATUS_DISP;

/* Private functions prototypes-----------------------------------------------*/
/* Buttons management for the application */
static void App_SW1_Action(void);
static void App_SW2_Action(void);
static void App_Core_UpdateButtonState(Button_TypeDef button, int isPressed);

/* Informations functions */
static void App_Core_Name_Disp   (void);
static void Display_Clean_Status (void);

/* Others Action */
static void App_Core_Leave_cb (struct ZbNlmeLeaveConfT *conf, void *arg);

/* Functions Definition ------------------------------------------------------*/

/**
 * @brief Core of the Application to manage all services to use
 * 
 */
void App_Core_Init(void)
{
  APP_ZB_DBG("Initialisation");

  App_Core_Name_Disp();

  App_Zigbee_Init();

  /* Task associated with button Action */
  UTIL_SEQ_RegTask(1U << CFG_TASK_BUTTON_SW1, UTIL_SEQ_RFU, App_SW1_Action);
  UTIL_SEQ_RegTask(1U << CFG_TASK_BUTTON_SW2, UTIL_SEQ_RFU, App_SW2_Action);

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
    UTIL_LCD_DisplayStringAt(0, LINE(DK_LCD_STATUS_LINE), (uint8_t *)"Error : Menu Config", CENTER_MODE);
    BSP_LCD_Refresh(0);
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
  App_Light_Cfg_Endpoint(app_zb_info.zb);
} /* App_Core_ConfigEndpoints */

/**
 * @brief Configure the Device End points according to the clusters used
 *        Call during the StackLayerInit
 * 
 */
void App_Core_ConfigGroupAddr(void)
{
  APP_ZB_DBG("Configure the GroupAddr for the device");
  App_Light_ConfigGroupAddr();
} /* App_Core_ConfigGroupAddr */

/**
 * @brief  Restore the application state as read cluster attribute after a startup from persistence
 * @param  None
 * @retval None
 */
void App_Core_Restore_State(void)
{
  APP_ZB_DBG("Restore Cluster informations for the application");
  App_Light_Restore_State();
} /* App_Core_Restore_State */


/* Informations functions -------------------------------------------------- */
/**
 * @brief  LCD DK Application name initialisation
 * 
 * @param  None
 * @retval None
 */
static void App_Core_Name_Disp(void)
{
  char LCD_Text[32];

  sprintf(LCD_Text, "RUC OnOff Light Router");
  /* Display Application information header */
  APP_ZB_DBG("%s", LCD_Text);
  /* Text Feature */
  BSP_LCD_Clear(0,SSD1315_COLOR_BLACK);
  BSP_LCD_Refresh(0);
  UTIL_LCD_DisplayStringAt(0, LINE(DK_LCD_APP_NAME_LINE), (uint8_t *)LCD_Text, CENTER_MODE);
  BSP_LCD_Refresh(0);
} /* App_Core_Name_Disp */

/**
 * @brief  Clean LCD display at the end of the status display
 * 
 * @param  None
 * @retval None
 */
static void Display_Clean_Status(void)
{
  UTIL_LCD_ClearStringLine(DK_LCD_STATUS_LINE);
  BSP_LCD_Refresh(0);
} /* Display_Clean_Status */

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


/* Buttons/Touchkey management for the application ------------------------- */
/**
 * @brief Wrapper to manage the short/long press
 * 
 * @param None
 * @retval None
 */
static void App_SW1_Action(void)
{
  App_Core_UpdateButtonState(BUTTON_USER1, BSP_PB_GetState(BUTTON_USER1) == BUTTON_PRESSED);
  return;
}
static void App_SW2_Action(void)
{
  App_Core_UpdateButtonState(BUTTON_USER2, BSP_PB_GetState(BUTTON_USER2) == BUTTON_PRESSED);
  return;
}
static void App_Core_UpdateButtonState(Button_TypeDef button, int isPressed)
{
  uint32_t t0 = 0,press_delay = 1;
  t0 = HAL_GetTick(); /* button press timing */
  while(BSP_PB_GetState(button) == BUTTON_PRESSED && press_delay <= MAX_PRESS_DELAY)
    press_delay = HAL_GetTick() - t0;
  
  if ( (BSP_PB_GetState(BUTTON_USER1) == BUTTON_PRESSED) && (BSP_PB_GetState(BUTTON_USER2) == BUTTON_PRESSED) )
  {
    App_Core_Factory_Reset();
  }
  
  /* Manage Push button time by each button */
  /* From Long time to short time*/
  switch (button)
  {
    case BUTTON_USER1:
      if (press_delay > LONG_PRESS_DELAY)
      {
        App_Core_Factory_Reset();
      }
      else if (press_delay > MIDDLE_PRESS_DELAY)
      {
        /* exit current submenu and Up to previous Menu */
        Exit_Menu_Item();
      }
      else if (press_delay > DEBOUNCE_DELAY)
      {
        /* Change menu selection to left */
        Prev_Menu_Item();       
      }
      break;

    case BUTTON_USER2:
      if (press_delay > MIDDLE_PRESS_DELAY)
      {
        /* execute action or enter sub-menu */
        Select_Menu_Item();
      }
      else if (press_delay > DEBOUNCE_DELAY)
      {
        /* Change menu selection to right */
        Next_Menu_Item();       
      }
      break;

    default:
      APP_ZB_DBG("ERROR : Button Unknow");
      break;
  }
} /* App_Core_UpdateButtonState */

/* Network Actions ---------------------------------------------------------- */
/**
 * @brief Launches the Network joining action when the user ready to add the device at network
 * could be called by menu action
 * 
 * @param  None
 * @retval None
*/
void App_Core_Ntw_Join(void)
{
  APP_ZB_DBG("Launching Network Join");
  UTIL_LCD_ClearStringLine(DK_LCD_STATUS_LINE);
  UTIL_LCD_DisplayStringAt(0, LINE(DK_LCD_STATUS_LINE), (uint8_t *)"Network Join", CENTER_MODE);
  BSP_LCD_Refresh(0);

  /* If Network joining was not successful reschedule the current task to retry the process */
  while (app_zb_info.join_status != ZB_STATUS_SUCCESS)
  {
    /* No Rejoin Network from persistence so decide todo it */
    UTIL_SEQ_SetTask(1U << CFG_TASK_ZIGBEE_NETWORK_JOIN, CFG_SCH_PRIO_0);
    UTIL_SEQ_WaitEvt(EVENT_ZIGBEE_NETWORK_JOIN);
  }

  /* Indicates successful join*/
  LED_Off();
  HAL_Delay(300);
  LED_Set_rgb(PWM_LED_GSDATA_OFF, PWM_LED_GSDATA_47_0, PWM_LED_GSDATA_OFF);
  HAL_Delay(300);
  LED_Set_rgb(PWM_LED_GSDATA_OFF, PWM_LED_GSDATA_OFF, PWM_LED_GSDATA_47_0);
  HAL_Delay(300);
  LED_Set_rgb(PWM_LED_GSDATA_OFF, PWM_LED_GSDATA_47_0, PWM_LED_GSDATA_OFF);
  HAL_Delay(300);
  LED_Set_rgb(PWM_LED_GSDATA_OFF, PWM_LED_GSDATA_OFF, PWM_LED_GSDATA_47_0);
  HAL_Delay(300);
  LED_Off();

  /* Assign ourselves to the group addresses */
  App_Core_ConfigGroupAddr();

  /* Since we're using group addressing (broadcast), shorten the broadcast timeout */
  uint32_t bcast_timeout = 3;
  ZbNwkSet(app_zb_info.zb, ZB_NWK_NIB_ID_NetworkBroadcastDeliveryTime, &bcast_timeout, sizeof(bcast_timeout));

  /* Display informations after Join */
  App_Zigbee_Channel_Disp();
  Display_Clean_Status();
} /* App_Core_Ntw_Join */


/* Actions from Menu ------------------------------------------------------- */
/**
 * @brief  Leave the Zigbbe network, clean the flash and reboot of the chip
 * 
 */
void App_Core_Factory_Reset(void)
{
  char LCD_Text[32];

  sprintf(LCD_Text, "FACTORY RESET");
  APP_ZB_DBG("%s", LCD_Text);
  /* Text Feature */
  UTIL_LCD_ClearStringLine(DK_LCD_STATUS_LINE);
  UTIL_LCD_DisplayStringAt(0, LINE(DK_LCD_STATUS_LINE), (uint8_t *)LCD_Text, CENTER_MODE);
  BSP_LCD_Refresh(0);
  // use ZbLeaveReq before to send information to the network before leave it
  if (app_zb_info.join_status == ZB_STATUS_SUCCESS)
  {
    App_Zigbee_Unbind_All();
    ZbLeaveReq(app_zb_info.zb, &App_Core_Leave_cb, NULL);
  }
  App_Persist_Delete();
  /* Wait 2 sec before clear display */
  HAL_Delay(2000);
  Display_Clean_Status();
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
}

