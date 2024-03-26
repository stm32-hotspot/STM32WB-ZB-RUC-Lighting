/**
  @page Zigbee_OnOff_Sensor application
  
  @verbatim
  ******************************************************************************
  * @file    Zigbee/Zigbee_OnOff_Sensor/readme.txt 
  * @author  MCD Application Team
  * @brief   Description of the Zigbee RUC Lighting Example. 
  ******************************************************************************
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  @endverbatim

@par Application Description 

This application act as a OnOff Sensor as describe in the Zigbee documentation 19-02016 from CSA.
It uses a PIR Sensor from Parallax.
Note this application was developped as part of the Zigbee RUC Lighting example 
to create a basic Zigbee network and not for an industrial application.
This device is a ZED with the following Zigbee clusters: OnOff.
It extends the Zigbee RUC Lighting with a PIR sensor acts as OnOff sensor.

@par Keywords

Zigbee, RUC, Realistic Use Cases, Light, OnOff, Sensor, PIR, End Point
 
@par Hardware and Software environment

  - This application runs on STM32WB55RG devices
  - This application uses a PIR Parallax see  https://www.parallax.com/product/pir-sensor-with-led-signal/
  - This application was developped with CubeWB package v1.18.0 and IAR 9.20.0.
  - STM32WB55RG board Set-up    
       - Connect the Board to your PC with a USB cable type A to mini-B to ST-LINK connector (USB_STLINK).

    
@par How to use it ? 


This application requires having the stm32wb5x_Zigbee_RFD_fw.bin binary flashed on the Wireles Coprocessor.
If it is not the case, you need to use STM32CubeProgrammer to load the appropriate binary.
For the detailed procedure to change the Wireless Coprocessor binary,
see following wiki for Hardware setup:
Refer to https://wiki.st.com/stm32mcu/wiki/Connectivity:STM32WB_Build_Zigbee_Project

In order to make the program work, you must do the following :
  - Open your toolchain
  - Rebuild all files and flash the board with executable file

1. Flash the Zigbee wireles stack **Reduce** on the STM32WB55RG board.

2. Build the application and flash the board with it.

3. You can now join a Zigbee network and bind with Light device and use it.

Available Wiki pages 
  - https://wiki.st.com/stm32mcu/wiki/Connectivity:Zigbee_Realistic_Use_Case_Lighting

 * <h3><center>&copy; COPYRIGHT STMicroelectronics</center></h3>
 */