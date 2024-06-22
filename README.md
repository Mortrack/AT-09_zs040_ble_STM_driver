# AT-09 zs040 ble (HM-10 ble clone) C STMicroelectronics driver library

Version: 1.0.2.3

This library provides the definitions, variables and functions necessary so that its implementer can use and communicate
with an AT-09 zs040 ble (HM-10 ble clone) device from a microcontroller/microprocessor of the STMicroelectronics device 
family and, in particular, via the STM32CubeIDE app. However, know that this driver library contains only the main
functions required for it to work in Slave Mode since there are several complications that makes this device not very
reliable in Central mode. For more details about this and to learn how to use this library, feel free to read the
<a href=https://github.com/Mortrack/AT-09_zs040_ble_STM_driver/tree/main/documentation>documentation of this project</a>.

# How to explore the project files.
The following will describe the general purpose of the folders that are located in the current directory address:

- **/'Inc'**:
    - This folder contains the header files required for this library to work, where you will find the following:
      - <a href=https://github.com/Mortrack/AT-09_zs040_ble_STM_driver/blob/main/Inc/AT-09_zs040_ble_driver.h>The actual driver library</a>.
      - Two configuration files for your AT-09 device:
        - <a href=https://github.com/Mortrack/AT-09_zs040_ble_STM_driver/blob/main/Inc/AT-09_config.h>The default configurations file<a/> for any AT-09 device with which this library is used with (this file should not be modified).
        - <a href=https://github.com/Mortrack/AT-09_zs040_ble_STM_driver/blob/main/Inc/AT-09_app_config.h>The application's configurations file</a> for any AT-09 device with which this library is used with (this is the file that should be modified in case that you want to have custom configurations).
- **/'Src'**:
    - This folder contains the <a href=https://github.com/Mortrack/AT-09_zs040_ble_STM_driver/blob/main/Src/AT-09_zs040_ble_driver.c>source code file for this library</a>.
- **/documentation**:
    - This folder provides the documentation to learn all the details of this library and to know how to use it. 

## Future additions planned for this library

Although I cannot commit to do the following soon, at some point if this project is useful for other people, then I plan
to continue adding more functions so that this can be a complete library with all the actual features of an AT-09
Bluetooth device.
