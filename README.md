# **STM32WB Zigbee RUC Lighting**

***

## Introduction

This is an example to quickly handle an On/Off Light Device with On/Off Light Switch.
The project generates a Zigbee Light Device acting as a Router (**ZR**) and Zigbee Light Switches as End Devices(**ED**).
It is a good introduction to handle and have a first approach of ST Zigbee mesh solution.

For more details, please refer to [Wiki Zigbee RUC Lighting](https://wiki.st.com/stm32mcu/wiki/Connectivity:Zigbee_Realistic_Use_Case_Lighting)

## Application description

This example uses the RGB Led on the **STM32WB5MM-DK board** as a dimmable light and the **P-NUCLEO-WB55 board** as light switch.
After binding the light bulb with light switch(es), you can control OnOff and level.

For more information on the boards, please visit [P-NUCLEO-WB55 Pack](https://www.st.com/en/evaluation-tools/p-nucleo-wb55.html) and [STM32WB5MM-DK Pack](https://www.st.com/en/evaluation-tools/stm32wb5mm-dk.html)

## Setup

### Software requirements

The following applications are running on the **P-NUCLEO-WB55 boards**:

- Zigbee_Coord
- Zigbee_Light_Switch
- Zigbee_Occupancy_Sensor _(possible extension of the network)_
- Zigbee_OnOff_Sensor _(possible extension of the network)_

The following application run on the **STM32WB5MM-DK board**:

- Zigbee_OnOff_Light

The applications ***Zigbee_Coord*** and ***Zigbee_OnOff_Light*** require having the *stm32wb5x_Zigbee_FFD_fw.bin* binary to be flashed on the Wireless Coprocessor. All others applications require having the *stm32wb5x_Zigbee_RFD_fw.bin* binary. If it is not the case, you need to use the STM32CubeProgrammer to load the appropriate binary.
For the detailed procedure to the Wireless Coprocessor binary see following wiki for hardware setup [STM32WB Build Zigbee project](https://wiki.st.com/stm32mcu/wiki/Connectivity:STM32WB_Build_Zigbee_Project)

In order to make the programs work, you must do the following:

- Open your toolchain
- Rebuild all files and flash the board with the executable file

For more details refer to the Application Note :  [AN5289 - Building a Wireless application](https://www.st.com/resource/en/application_note/an5289-how-to-build-wireless-applications-with-stm32wb-mcus-stmicroelectronics.pdf)

### Hardware requirements

For ***Zigbee_Occupancy_Sensor*** and ***Zigbee_OnOff_Sensor*** application, a 3-pin PIR sensor is needed like [PIR Parallax](https://www.parallax.com/product/pir-sensor-with-led-signal/)

### Install the Zigbee Network

To install the RUC Lighting, please follow the [HowTo Join a Zigbee Network](https://wiki.st.com/stm32mcu/wiki/Connectivity:Introduction_to_Zigbee_Realistic_Use_Case#HowTo_Join_Zigbee_Network) and [HowTo bind devices](https://wiki.st.com/stm32mcu/wiki/Connectivity:Introduction_to_Zigbee_Realistic_Use_Case#HowTo_bind_devices)

## Troubleshooting

**Caution** : Issues and the pull-requests are **not supported** to submit problems or suggestions related to the software delivered in this repository. The STM32WB-ZB-RUC-Lighting example is being delivered as-is, and not necessarily supported by ST.

**For any other question** related to the product, the hardware performance or characteristics, the tools, the environment, you can submit it to the **ST Community** on the STM32 MCUs related [page](https://community.st.com/s/topic/0TO0X000000BSqSWAW/stm32-mcus).
