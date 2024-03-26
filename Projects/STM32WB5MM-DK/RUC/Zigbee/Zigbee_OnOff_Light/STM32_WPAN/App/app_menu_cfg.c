/**
******************************************************************************
* @file    app_menu_cfg.c
* @author  Zigbee Application Team
* @brief   Configuration interface for application menu
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
#include "app_menu.h"

#include "app_zigbee.h"
#include "app_core.h"
#include "app_light_cfg.h"

/* External variables ------------------------------------------------------- */
extern uint8_t display_type;

/* Private Functions prototypes -------------------------------------------- */
// static void My_func(void);

/**
 * @brief  Configure the Menu tree
 * @param  display_select to choose where the Menu appears
 * @retval state
 */
bool Menu_Config(void)
{
  // Choose where display the menu
  display_type = (UART_DISPLAY | DK_LCD_DISPLAY);

  /* Menu item instances -----------------------------------------------------*/
  // 1st level of menu
  Menu_Item_T * menu_reset= Create_Menu_Item();
  Menu_Item_T * menu_info = Create_Menu_Item();

  // Network Menu
  Menu_Item_T * menu_ntw            = Create_Menu_Item();
  Menu_Item_T * menu_ntw_join       = Create_Menu_Item();
  Menu_Item_T * menu_permit_join    = Create_Menu_Item();
  Menu_Item_T * menu_ntw_idmode     = Create_Menu_Item();
  Menu_Item_T * menu_ntw_findbind   = Create_Menu_Item();
  Menu_Item_T * menu_ntw_bind_tab   = Create_Menu_Item();
  Menu_Item_T * menu_ntw_unbind_all = Create_Menu_Item();
  Menu_Item_T * menu_ntw_txpwr_disp = Create_Menu_Item();
  Menu_Item_T * menu_ntw_txpwr_up   = Create_Menu_Item();
  Menu_Item_T * menu_ntw_txpwr_down = Create_Menu_Item();

  // Light conf Menu
  Menu_Item_T * menu_light_config     = Create_Menu_Item();
  Menu_Item_T * menu_light_toggle     = Create_Menu_Item();
  Menu_Item_T * menu_light_lvl_inc    = Create_Menu_Item();
  Menu_Item_T * menu_light_lvl_dec    = Create_Menu_Item();
  

  /* Menu link --------------------------------------------------------------*/
  // Main menu --------|  Menu name     | Current Item     | Next Item        | Sub-Menu         | Action to launch        |
  Add_Menu_Item((char *) "Network"      , menu_ntw         , menu_light_config, menu_ntw_join    , NULL);
  Add_Menu_Item((char *) "Light Cfg"    , menu_light_config, menu_reset       , menu_light_toggle, NULL);
  Add_Menu_Item((char *) "Factory Reset", menu_reset       , menu_info        , NULL             , &App_Core_Factory_Reset);
  Add_Menu_Item((char *) "Global Infos" , menu_info        , menu_ntw         , NULL             , &App_Core_Infos_Disp);
  
  // Network Menu
  Add_Menu_Item((char *) "Join Network" , menu_ntw_join      , menu_permit_join   , NULL, &App_Core_Ntw_Join);
  Add_Menu_Item((char *) "Permit Join"  , menu_permit_join   , menu_ntw_idmode    , NULL, &App_Zigbee_Permit_Join);
  Add_Menu_Item((char *) "Id-Mode"      , menu_ntw_idmode    , menu_ntw_findbind  , NULL, &App_Light_IdentifyMode);   
  Add_Menu_Item((char *) "Find & Bind"  , menu_ntw_findbind  , menu_ntw_bind_tab  , NULL, &App_Light_FindBind);   
  Add_Menu_Item((char *) "Bind Table"   , menu_ntw_bind_tab  , menu_ntw_unbind_all, NULL, &App_Zigbee_Bind_Disp);
  Add_Menu_Item((char *) "UnBind All"   , menu_ntw_unbind_all, menu_ntw_txpwr_disp, NULL, &App_Zigbee_Unbind_All);
  Add_Menu_Item((char *) "Tx Power Disp", menu_ntw_txpwr_disp, menu_ntw_txpwr_up  , NULL, &App_Zigbee_TxPwr_Disp);
  Add_Menu_Item((char *) "Tx Power +"   , menu_ntw_txpwr_up  , menu_ntw_txpwr_down, NULL, &App_Zigbee_TxPwr_Up);
  Add_Menu_Item((char *) "Tx Power -"   , menu_ntw_txpwr_down, menu_ntw_join      , NULL, &App_Zigbee_TxPwr_Down);
  
  // Light Menu
  Add_Menu_Item((char *) "Light Toggle"   , menu_light_toggle    , menu_light_lvl_inc, NULL, &App_Light_Toggle);
  Add_Menu_Item((char *) "Light lvl +"    , menu_light_lvl_inc   , menu_light_lvl_dec, NULL, &App_Light_Up);
  Add_Menu_Item((char *) "Light lvl -"    , menu_light_lvl_dec   , menu_light_toggle , NULL, &App_Light_Down);
  
  return Def_Start_Menu_Item(menu_ntw);
} /* Menu_Config */

/* User menu config function ---------------------------------------------------*/

