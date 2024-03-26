/**
  ******************************************************************************
  * @file    app_light_switch.h
  * @author  Zigbee Application Team
  * @brief   Header for Light Switch application file.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef APP_LIGHT_SWITCH_H
#define APP_LIGHT_SWITCH_H

#ifdef __cplusplus
extern "C" {
#endif

/* Defines ----------------------------------------------------------------- */
/* Endpoint device */
#define LIGHT_SWITCH_ENDPOINT           0x0002U
#define LIGHT_SWITCH_GROUP_ADDR         0x0001U

/* Retry */
#define NB_OF_SERV_BINDABLE                             5
#define SIZE_RETRY_TAB            NB_OF_SERV_BINDABLE + 1
#define RETRY_ERROR_INDEX             NB_OF_SERV_BINDABLE
#define MAX_RETRY_REPORT                                5
#define MAX_RETRY_READ                                  5
#define MAX_RETRY_CMD                                   5

/* Typedef ----------------------------------------------------------------- */
typedef enum
{
  IDLE,
  FIND_AND_BIND,
  REPORT_CONF_ONOFF,
  READ_ONOFF,
  WRITE_ONOFF,
  REPORT_CONF_LVL,
  READ_LVL,
  WRITE_LVL,
} Cmd_Type_T;
      
typedef struct 
{
  Cmd_Type_T cmd_type;
  struct ZbApsAddrT * dst;
} Cmd_Retry_T;

typedef struct 
{
  uint8_t retry_nb;
  uint64_t ext_addr;
} Retry_number_T;


/* Exported Prototypes -------------------------------------------------------*/
void App_LightSwitch_Cfg_Endpoint   (struct ZigBeeT *zb);
void App_LightSwitch_ConfigGroupAddr(void);
void App_LightSwitch_Restore_State  (void);

void App_LightSwitch_FindBind       (void);
void App_LightSwitch_Bind_Disp      (void);

/* Exported Prototypes -------------------------------------------------------*/
// Retry function Declaration --------------------------------------------------
uint8_t * Get_retry_nb              (Retry_number_T retry_tab[SIZE_RETRY_TAB], uint64_t server_addr);
void App_LightSwitch_Retry_Cmd      (Cmd_Type_T cmd, const struct ZbApsAddrT * dst);


/* Light control device ------------------------------------------------------*/
void App_LightSwitch_Toggle    (void);
void App_LightSwitch_Level_Up  (void);
void App_LightSwitch_Level_Down(void);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* APP_LIGHT_SWITCH_H */
