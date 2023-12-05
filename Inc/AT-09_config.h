/** @addtogroup hm10_ble_clone
 * @{
 */

/**@defgroup main_AT_09_config AT-09 zs040 BLE Device Configuration Files
 * @{
 *
 * @brief   This module contains both the default and the application AT-09 zs040 BLE Driver Configuration Files.
 *
 * @author 	Cesar Miranda Meza (cmirandameza3@hotmail.com)
 * @date	December 05, 2023.
 */

/**@file
 * @brief	AT-09 zs040 BLE Driver default configuration file.
 *
 * @defgroup AT_09_config Default ETX OTA Protocol Configuration File
 * @{
 *
 * @brief   This file contains all the Default ETX OTA Protocol Configurations.
 *
 * @note    It is highly suggested not to directly edit the Configurations Settings defined in this file. Instead of
 *          doing that whenever requiring different Configuration Settings, it is suggested to do that instead in an
 *          additional header file named as "AT-09_app_config.h" whose File Path should be located as a sibbling of
 *          this @ref AT_09_config header file. However, to enable the use of that additional header file, you will
 *          also need to change the @ref ENABLE_APP_AT09_CONFIG define, that is located at @ref AT_09_config , to a
 *          value of 1.
 *
 * @author 	Cesar Miranda Meza (cmirandameza3@hotmail.com)
 * @date	December 05, 2023.
 */

#define ETX_OTA_CONFIG_H_
#include "AT-09_zs040_ble_driver.h" // Custom Mortrack's Library to be able to initialize, send configuration commands and send and/or receive data to/from a AT-09 zs040 BLE Device.

#define ENABLE_APP_AT09_CONFIG           (1)          		                                            /**< @brief Flag used to enable the use of @ref AT_09_app_config with a 1 or, otherwise to disable it with a 0. */
#if ENABLE_APP_AT09_CONFIG
#include "AT-09_app_config.h" // This is the user custom AT-09 Driver configuration file that is recommended to use whenever it is desired to edit default configuration settings as defined in this @ref AT_09_config file.
#endif

#ifndef ETX_OTA_VERBOSE
#define ETX_OTA_VERBOSE 			        (0U)   	        	                                        /**< @brief Flag value used to enable the compiler to take into account the code of both the @ref hm10_ble_clone library that displays detailed information about the processes made inside them via @ref printf with a \c 1 . Otherwise, a \c 0 for not displaying any messages at all with @ref printf . */
#endif

#ifndef HM10_CLONE_DEFAULT_BLE_NAME
#define HM10_CLONE_DEFAULT_BLE_NAME         'H', 'M', '-', '1', '0', ' ','n', 'a', 'm', 'e', '_', '1'	/**< @brief Designated ASCII Code data representing the desired default BLE Name that wants to be given to the HM-10 Clone BLE Device, whose length has to be @ref HM10_CLONE_MAX_BLE_NAME_SIZE at the most. */
#endif

#ifndef HM10_CLONE_DEFAULT_ROLE
#define HM10_CLONE_DEFAULT_ROLE	            (HM10_Clone_Role_Peripheral)								/**< @brief Designated default BLE Role that wants to be given to the HM-10 Clone BLE Device @details See @ref HM10_Clone_Role for more details. */
#endif

#ifndef HM10_CLONE_DEFAULT_PIN
#define HM10_CLONE_DEFAULT_PIN   			'0', '1', '2', '3', '4', '5'              					/**< @brief Designated ASCII Code data representing the desired default Pin Code that wants to be given to the HM-10 Clone BLE Device (all the ASCII Characters given must stand for the ASCII numbers 0, 1, ...., 9). */
#endif

#ifndef HM10_CLONE_DEFAULT_PIN_CODE_MODE
#define HM10_CLONE_DEFAULT_PIN_CODE_MODE	(HM10_Clone_Pin_Code_DISABLED)								/**< @brief Designated default BLE Pin Code Mode that wants to be given to the HM-10 Clone BLE Device @details See @ref HM10_Clone_Pin_Code_Mode for more details. */
#endif

#ifndef HM10_CLONE_CUSTOM_HAL_TIMEOUT
#define HM10_CLONE_CUSTOM_HAL_TIMEOUT	    (120U)				                						/**< @brief Designated time in milliseconds for the HAL Timeout to be requested during each FLASH request in our MCU/MPU that are used in the @ref hm10_ble_clone . @note For more details see @ref FLASH_WaitForLastOperation . @note As a reference, the lowest value at which the author the @ref hm10_ble_clone had always unsuccessful responses was with 100 milliseconds. On the other hand, 120 milliseconds worked most of the times but it did not on some rare occasions. Therefore, it is suggested that the implementer/user of the @ref hm10_ble_clone to assign a more convenient value for this field with which the implementer feels more confident that it will always work well. */
#endif

/** @} */ // AT_09_config

/** @} */ // main_AT_09_config

/** @} */ // hm10_ble_clone
