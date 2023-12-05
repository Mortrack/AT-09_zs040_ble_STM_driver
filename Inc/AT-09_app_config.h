/** @addtogroup hm10_ble_clone
 * @{
 */

/** @addtogroup main_AT_09_config
 * @{
 */

/**@file
 * @brief	AT-09 zs040 BLE Driver personalized configuration file (edit AT-09 zs040 BLE Driver Configurations here).
 *
 * @defgroup AT_09_app_config Application AT-09 zs040 BLE Driver Configuration File
 * @{
 *
 * @brief   This file should contain all the AT-09 zs040 BLE Driver Configurations made by the user/implementor.
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

#ifndef AT_09_APP_CONFIG_H_
#define AT_09_APP_CONFIG_H_

#define ETX_OTA_VERBOSE 			        (1)   	        					/**< @brief Flag value used to enable the compiler to take into account the code of both the @ref hm10_ble_clone library that displays detailed information about the processes made inside them via @ref printf with a \c 1 . Otherwise, a \c 0 for not displaying any messages at all with @ref printf . */

#endif /* AT_09_APP_CONFIG_H_ */

/** @} */ // AT_09_app_config

/** @} */ // main_AT_09_config

/** @} */ // hm10_ble_clone
