/** @addtogroup hm10_ble_clone
 * @{
 */

#include "AT-09_zs040_ble_driver.h"
#include <string.h>	// Library from which "memset()" and "memcpy()" are located at.

#define HM10_CLONE_MAX_AT_COMMAND_SIZE							(21)		/**< @brief Total maximum bytes in a Tx/Rx AT Command of the HM-10 Clone BLE Device. */
#define HM10_CLONE_MAX_PACKET_SIZE								(18)		/**< @brief Total maximum bytes in a Tx/Rx packet/Payload to/from the HM-10 Clone BLE Device. @note Due to the lack of documentation for the HM-10 CTFZ54812 ZS-040 Clone BLE Device, several empirical tests were conducted, from which it was concluded that although the device had no restrictions on the maximum amount of data that is desired to be transmitted from the HM-10 Clone device to an external BLE Device, this is not the case for receiving data. It was concluded that the HM-10 Clone BLE device could only receive a maximum of 18 ASCII characters from a single request, which means that if more data is to be received, this would have to be broke into several parts with a maximum size of 18 bytes each. @note Since the restriction of receiving data is of 18 bytes per request, to manage things homogeneously, both the transmit and receive requests will be managed with the same size of 18 bytes. */
#define HM10_CLONE_TEST_CMD_SIZE								(4)			/**< @brief	Length in bytes of a Test Command in the HM-10 Clone BLE device. */
#define HM10_CLONE_RESET_CMD_SIZE								(10)		/**< @brief	Length in bytes of a Reset Command in the HM-10 Clone BLE device. */
#define HM10_CLONE_NAME_RESPONSE_SIZE_WITHOUT_REQUESTED_NAME	(8)			/**< @brief	Length in bytes of a Name Response from the HM-10 Clone BLE device but without considering the length of the requested name. */
#define HM10_CLONE_GET_NAME_CMD_SIZE							(9)			/**< @brief	Length in bytes of a Get Name Command in the HM-10 Clone BLE device. */
#define HM10_CLONE_SET_ROLE_CMD_SIZE							(10)		/**< @brief	Length in bytes of a Set Role Command in the HM-10 Clone BLE device. */
#define HM10_CLONE_GET_ROLE_CMD_SIZE							(9)			/**< @brief	Length in bytes of a Get Role Command in the HM-10 Clone BLE device. */
#define HM10_CLONE_ROLE_RESPONSE_SIZE							(9)			/**< @brief	Length in bytes of either a Get or a Set Role Command's Response in the HM-10 Clone BLE device. */
#define HM10_CLONE_SET_PIN_CMD_SIZE								(14)		/**< @brief	Length in bytes of the Set Pin Command of a HM-10 Clone BLE device. */
#define HM10_CLONE_GET_PIN_CMD_SIZE								(8)			/**< @brief	Length in bytes of the Get Pin Command of a HM-10 Clone BLE device. */
#define HM10_CLONE_PIN_RESPONSE_SIZE							(13)		/**< @brief	Length in bytes of either a Get or a Set Pin Command's Response in the HM-10 Clone BLE device. */
#define HM10_CLONE_SET_TYPE_CMD_SIZE							(10)		/**< @brief	Length in bytes of the Set Type Command of a HM-10 Clone BLE device. */
#define HM10_CLONE_GET_TYPE_CMD_SIZE							(9)			/**< @brief	Length in bytes of the Get Type Command of a HM-10 Clone BLE device. */
#define HM10_CLONE_TYPE_RESPONSE_SIZE							(9)			/**< @brief	Length in bytes of either a Get or a Set Type Command's Response in the HM-10 Clone BLE device. */
#define HM10_CLONE_OK_RESPONSE_SIZE								(4)			/**< @brief	Length in bytes of a OK Response from the HM-10 Clone BLE device. */

static UART_HandleTypeDef *p_huart;												                /**< @brief Pointer to the UART Handle Structure of the UART that will be used in this @ref hm10_ble_clone to communicate with the HM-10 Clone BLE device. @details This pointer's value is defined in the @ref init_hm10_clone_module function. */
static uint8_t TxRx_Buffer[HM10_CLONE_MAX_AT_COMMAND_SIZE];					                    /**< @brief Global buffer that will be used by our MCU/MPU to hold the whole data of a received response or a request to be send from/to the HM-10 Clone BLE Device. */
static uint8_t HM10_Clone_Name_resp[] = {'+', 'N', 'A', 'M', 'E', '='};	/**< @brief Pointer to the equivalent data of a BLE Name Response that the HM-10 Clone BLE device sends back to our MCU/MPU whenever a Get Name or Set Name request to the HM-10 Clone BLE device was processed successfully. */
static uint8_t HM10_Clone_Role_resp[] = {'+', 'R', 'O', 'L', 'E', '='};	/**< @brief Pointer to the equivalent data of a BLE Role Response that the HM-10 Clone BLE device sends back to our MCU/MPU whenever a Get Role or Set Role request to the HM-10 Clone BLE device was processed successfully. */
static uint8_t HM10_Clone_Pin_resp[] = {'+', 'P', 'I', 'N', '='};			/**< @brief Pointer to the equivalent data of a BLE Pin Response that the HM-10 Clone BLE device sends back to our MCU/MPU whenever a Get Pin or Set Pin request to the HM-10 Clone BLE device was processed successfully. */
static uint8_t HM10_Clone_Type_resp[] = {'+', 'T', 'Y', 'P', 'E', '='};	/**< @brief Pointer to the equivalent data of a BLE Type Response that the HM-10 Clone BLE device sends back to our MCU/MPU whenever a Get Type or Set Type request to the HM-10 Clone BLE device was processed successfully. */
static uint8_t HM10_Clone_OK_resp[] = {'O', 'K', '\r', '\n'};				    /**< @brief Pointer to the equivalent data of an OK Response that the HM-10 Clone BLE device sends back to our MCU/MPU whenever a request to set a new setting on the HM-10 Clone BLE device was processed successfully. */
static uint8_t resp_attempts;												                    /**< @brief Counter for the number of attempts for receiving an expected Response from the HM-10 Clone BLE device after having send to it a certain command. */
static const uint8_t CR_AND_LF_SIZE = 2;									                    /**< @brief Length in bytes of a Carriage Return and a New Line characters together. */

/**@brief	Numbers in ASCII code definitions.
 *
 * @details	These definitions contain the decimal equivalent value for each of the number characters available in the
 *          ASCII code table.
 *
 * @note    These definitions are used whenever sending and setting a pin value to the HM-10 Clone BLE Device. In
 *          particular, they are used for validating that the pin requested to be set on that Device contains only
 *          number characters available in the ASCII code table.
 */
typedef enum
{
	Number_0_in_ASCII	= 48U,    //!< \f$0_{ASCII} = 48_d\f$.
	Number_1_in_ASCII	= 49U,    //!< \f$1_{ASCII} = 49_d\f$.
	Number_2_in_ASCII	= 50U,    //!< \f$2_{ASCII} = 50_d\f$.
	Number_3_in_ASCII	= 51U,    //!< \f$3_{ASCII} = 51_d\f$.
	Number_4_in_ASCII	= 52U,    //!< \f$4_{ASCII} = 52_d\f$.
	Number_5_in_ASCII	= 53U,    //!< \f$5_{ASCII} = 53_d\f$.
	Number_6_in_ASCII	= 54U,    //!< \f$6_{ASCII} = 54_d\f$.
	Number_7_in_ASCII	= 55U,    //!< \f$7_{ASCII} = 55_d\f$.
	Number_8_in_ASCII	= 56U,    //!< \f$8_{ASCII} = 56_d\f$.
	Number_9_in_ASCII	= 57U     //!< \f$9_{ASCII} = 57_d\f$.
} Numbers_in_ASCII;

/**@brief	Sends a Test Command to the HM-10 Clone BLE Device with a maximum of two attempts.
 *
 * @retval	HM10_Clone_EC_OK	if the Test Command was successfully sent to the HM-10 Clone BLE Device and if an OK
 *                              Response was received from it subsequently.
 * @retval  HM10_Clone_EC_NR    if there was no response from the HM-10 Clone BLE Device.
 * @retval  HM10_Clone_EC_ERR   if, after having send the Test Command, the validation of the expected OK Response from
 *                              the HM-10 Clone BLE Device was unsuccessful, or if anything else went wrong.
 *
 * @author	César Miranda Meza (cmirandameza3@hotmail.com)
 * @date	October 17, 2023
 */
static HM10_Clone_Status send_test_cmd();

/**@brief	Sends a Reset Command to the HM-10 Clone BLE Device with a maximum of two attempts.
 *
 * @retval	HM10_Clone_EC_OK	if the Reset Command was successfully sent to the HM-10 Clone BLE Device and if an OK
 *                              Response was received from it subsequently.
 * @retval  HM10_Clone_EC_NR    if there was no response from the HM-10 Clone BLE Device.
 * @retval  HM10_Clone_EC_ERR   if, after having send the Reset Command, the validation of the expected OK Response from
 *                              the HM-10 Clone BLE Device was unsuccessful, or if anything else went wrong.
 *
 * @author	César Miranda Meza (cmirandameza3@hotmail.com)
 * @date	October 25, 2023
 */
static HM10_Clone_Status send_reset_cmd();

/**@brief	Sends a Name Command to the HM-10 Clone BLE Device with a maximum of two attempts.
 *
 * @param[in] hm10_name Pointer to the ASCII Code data representing the desired BLE Name that wants to be given to the
 *                      HM-10 Clone BLE Device.
 * @param size          Length in bytes of the data towards which the \p hm10_name param points to, which has to be a
 *                      maximum of @ref HM10_CLONE_MAX_BLE_NAME_SIZE .
 * 
 * @retval	HM10_Clone_EC_OK	if the Name Command was successfully sent to the HM-10 Clone BLE Device and if both the
 *                              Name and OK Responses were received from it subsequently.
 * @retval  HM10_Clone_EC_NR    if there was no response from the HM-10 Clone BLE Device.
 * @retval  HM10_Clone_EC_ERR   <ul>
 *                                  <li>
 *                                      If, after having send the Name Command, the validation of the expected OK
 *                                      Response from the HM-10 Clone BLE Device was unsuccessful.
 *                                  </li>
 *                                  <li>
 *                                      If the \p size param has a value greater than @ref HM10_CLONE_MAX_BLE_NAME_SIZE .
 *                                  </li>
 *                                  <li>
 *                                      If anything else went wrong.
 *                                  </li>
 *                              </ul>
 *
 * @author	César Miranda Meza (cmirandameza3@hotmail.com)
 * @date	October 17, 2023
 */
static HM10_Clone_Status send_set_name_cmd(uint8_t *hm10_name, uint8_t size);

/**@brief	Sends a Get Name Command to the HM-10 Clone BLE Device with a maximum of two attempts in order to get the
 *          BLE Name from the HM-10 Clone BLE Device.
 *
 * @param[out] hm10_name    Pointer to the ASCII Code data that should contain the BLE Name that is to be received from
 *                          the HM-10 Clone BLE Device. Note that the name stored into the address that this param
 *                          points to, will not contain either the Carriage Return or New Line characters in it.
 * @param[out] size         Length in bytes of the BLE Name received from the HM-10 Clone BLE Device, which should have
 *                          a maximum size of @ref HM10_CLONE_MAX_BLE_NAME_SIZE .
 *
 * @retval	HM10_Clone_EC_OK	if the BLE Name was successfully received from the HM-10 Clone BLE Device and if its
 *                              size did not exceed @ref HM10_CLONE_MAX_BLE_NAME_SIZE .
 * @retval  HM10_Clone_EC_NR    if there was no response from the HM-10 Clone BLE Device.
 * @retval  HM10_Clone_EC_ERR   <ul>
 *                                  <li>
 *                                      If, after having send the Get Name Command, the validation of the expected Get
 *                                      Name Response from the HM-10 Clone BLE Device was unsuccessful.
 *                                  </li>
 *                                  <li>
 *                                      If the \p size param has a value greater than @ref HM10_CLONE_MAX_BLE_NAME_SIZE .
 *                                  </li>
 *                                  <li>
 *                                      If anything else went wrong.
 *                                  </li>
 *                              </ul>
 *
 * @author	César Miranda Meza (cmirandameza3@hotmail.com)
 * @date	October 20, 2023
 */
static HM10_Clone_Status send_get_name_cmd(uint8_t *hm10_name, uint8_t *size);

/**@brief	Sends a Role Command to the HM-10 Clone BLE Device with a maximum of two attempts.
 *
 * @param ble_role	BLE Role that wants to be set on the HM-10 Clone BLE Device.
 *
 * @retval	HM10_Clone_EC_OK	if the Role Command was successfully sent to the HM-10 Clone BLE Device and if the Role
 *                              Response was received from it subsequently.
 * @retval  HM10_Clone_EC_NR    if there was no response from the HM-10 Clone BLE Device.
 * @retval  HM10_Clone_EC_ERR   <ul>
 *                                  <li>
 *                                      If, after having send the Role Command, the validation of the expected Role
 *                                      Response from the HM-10 Clone BLE Device was unsuccessful.
 *                                  </li>
 *                                  <li>
 *                                      If the \p ble_role param has an invalid value.
 *                                  </li>
 *                                  <li>
 *                                      If anything else went wrong.
 *                                  </li>
 *                              </ul>
 *
 * @author	César Miranda Meza (cmirandameza3@hotmail.com)
 * @date	October 23, 2023
 */
static HM10_Clone_Status send_set_role_cmd(HM10_Clone_Role ble_role);

/**@brief	Sends a Get Role Command to the HM-10 Clone BLE Device with a maximum of two attempts in order to get the
 *          BLE Role of the HM-10 Clone BLE Device.
 *
 * @param[out] ble_role	Pointer to the 1 byte data into which this function will write the BLE Role value given by the
 *                      HM-10 Clone BLE Device. Note that the possible values written are @ref HM10_Clone_Role .
 *
 * @retval	HM10_Clone_EC_OK	if the BLE Role was successfully received from the HM-10 Clone BLE Device and if its
 *                              given value is among the recognized/expected ones, which are described in
 *                              @ref HM10_Clone_Role .
 * @retval  HM10_Clone_EC_NR    if there was no response from the HM-10 Clone BLE Device.
 * @retval  HM10_Clone_EC_ERR   <ul>
 *                                  <li>
 *                                      If, after having send the Get Role Command, the validation of the expected Get
 *                                      Role Response from the HM-10 Clone BLE Device was unsuccessful.
 *                                  </li>
 *                                  <li>
 *                                      If anything else went wrong.
 *                                  </li>
 *                              </ul>
 *
 * @author	César Miranda Meza (cmirandameza3@hotmail.com)
 * @date	October 23, 2023
 */
static HM10_Clone_Status send_get_role_cmd(HM10_Clone_Role *ble_role);

/**@brief	Sends a Pin Command to the HM-10 Clone BLE Device with a maximum of two attempts.
 *
 * @param[in] pin	Pointer to the ASCII Code data representing the desired BLE Pin that wants to be given to the HM-10
 *                  Clone BLE Device. This pin data must consist of 6 bytes of data, where each byte must stand for any
 *                  number character in ASCII Code.
 *
 * @retval	HM10_Clone_EC_OK	if the Pin Command was successfully sent to the HM-10 Clone BLE Device and if the Pin
 *                              Response was received from it subsequently.
 * @retval  HM10_Clone_EC_NR    if there was no response from the HM-10 Clone BLE Device.
 * @retval  HM10_Clone_EC_ERR   <ul>
 *                                  <li>
 *                                      If, after having send the Pin Command, the validation of the expected Pin
 *                                      Response from the HM-10 Clone BLE Device was unsuccessful.
 *                                  </li>
 *                                  <li>
 *                                      If the \p pin param points to data where one of its bytes of data that does not
 *                                      correspond to a number character in ASCII Code.
 *                                  </li>
 *                                  <li>
 *                                      If anything else went wrong.
 *                                  </li>
 *                              </ul>
 *
 * @author	César Miranda Meza (cmirandameza3@hotmail.com)
 * @date	October 23, 2023
 */
static HM10_Clone_Status send_set_pin_cmd(uint8_t *pin);

/**@brief	Sends a Get Pin Command to the HM-10 Clone BLE Device with a maximum of two attempts in order to get the
 *          BLE Pin of the HM-10 Clone BLE Device.
 *
 * @param[out] pin	Pointer to the ASCII Code data representing the received BLE Pin from the HM-10 Clone BLE Device.
 *                  This pin data consists of a size of 6 bytes, where each byte must stand for any number character in
 *                  ASCII Code.
 *
 * @retval	HM10_Clone_EC_OK	if the BLE Pin was successfully received from the HM-10 Clone BLE Device and if each its
 *                              given bytes of data correspond to a number character in ASCII Code.
 * @retval  HM10_Clone_EC_NR    if there was no response from the HM-10 Clone BLE Device.
 * @retval  HM10_Clone_EC_ERR   <ul>
 *                                  <li>
 *                                      If, after having send the Get Pin Command, the validation of the expected Get
 *                                      Pin Response from the HM-10 Clone BLE Device was unsuccessful.
 *                                  </li>
 *                                  <li>
 *                                      If anything else went wrong.
 *                                  </li>
 *                              </ul>
 *
 * @author	César Miranda Meza (cmirandameza3@hotmail.com)
 * @date	October 23, 2023
 */
static HM10_Clone_Status send_get_pin_cmd(uint8_t *pin);

/**@brief	Sends a Type Command to the HM-10 Clone BLE Device with a maximum of two attempts.
 *
 * @param pin_code_mode Pin Code Mode that is desired to set in the HM-10 Clone BLE Device.
 *
 * @retval	HM10_Clone_EC_OK	if the Type Command was successfully sent to the HM-10 Clone BLE Device and if the Type
 *                              and OK Responses were received from it subsequently.
 * @retval  HM10_Clone_EC_NR    if there was no response from the HM-10 Clone BLE Device.
 * @retval  HM10_Clone_EC_ERR   <ul>
 *                                  <li>
 *                                      If, after having send the Type Command, the validation of either the expected
 *                                      Type or OK Responses from the HM-10 Clone BLE Device were unsuccessful.
 *                                  </li>
 *                                  <li>
 *                                      If the \p pin_code_mode param contains an invalid value (see
 *                                      @ref HM10_Clone_Pin_Code_Mode for valid values).
 *                                  </li>
 *                                  <li>
 *                                      If anything else went wrong.
 *                                  </li>
 *                              </ul>
 *
 * @author	César Miranda Meza (cmirandameza3@hotmail.com)
 * @date	October 24, 2023
 */
static HM10_Clone_Status send_set_type_cmd(HM10_Clone_Pin_Code_Mode pin_code_mode);

/**@brief	Sends a Get Type Command to the HM-10 Clone BLE Device with a maximum of two attempts in order to get the
 *          Pin Code Mode of the HM-10 Clone BLE Device.
 *
 * @param[out] pin_code_mode    @ref HM10_Clone_Pin_Code_Mode Type Pointer to the Pin Code Mode that the HM-10 Clone BLE
 *                              Device currently has configured in it.
 *
 * @retval	HM10_Clone_EC_OK	if the Pin Code Mode was successfully received from the HM-10 Clone BLE Device and if
 *                              the given data corresponds to a @ref HM10_Clone_Pin_Code_Mode value.
 * @retval  HM10_Clone_EC_NR    if there was no response from the HM-10 Clone BLE Device.
 * @retval  HM10_Clone_EC_ERR   <ul>
 *                                  <li>
 *                                      If, after having send the Get Type Command, the validation of the expected Get
 *                                      Type Response from the HM-10 Clone BLE Device was unsuccessful.
 *                                  </li>
 *                                  <li>
 *                                      If anything else went wrong.
 *                                  </li>
 *                              </ul>
 *
 * @author	César Miranda Meza (cmirandameza3@hotmail.com)
 * @date	October 23, 2023
 */
static HM10_Clone_Status send_get_type_cmd(HM10_Clone_Pin_Code_Mode *pin_code_mode);

/**@brief	Flushes the RX of the UART towards which the @ref p_huart Global Pointer points to.
 *
 * @details This function will poll-receive one byte from the RX of the UART previously mentioned with a timeout of @ref
 *          HM10_CLONE_CUSTOM_HAL_TIMEOUT over and over until a @ref HAL_TIMEOUT HAL Status is received.
 *
 * @author	César Miranda Meza (cmirandameza3@hotmail.com)
 * @date    November 30, 2023.
 */
static void HAL_uart_rx_flush();

/**@brief	Gets the corresponding @ref HM10_Clone_Status value depending on the given @ref HAL_StatusTypeDef value.
 *
 * @param HAL_status	HAL Status value (see @ref HAL_StatusTypeDef ) that wants to be converted into its equivalent
 * 						of a @ref HM10_Clone_Status value.
 *
 * @retval				HM10_Clone_EC_NR if \p HAL_status param equals \c HAL_BUSY or \c HAL_TIMEOUT .
 * @retval				HM10_Clone_EC_ERR if \p HAL_status param equals \c HAL_ERROR .
 * @retval				HAL_status param otherwise.
 *
 * @note	For more details on the returned values listed, see @ref HM10_Clone_Status and @ref HAL_StatusTypeDef .
 *
 * @author	César Miranda Meza (cmirandameza3@hotmail.com)
 * @date 	October 17, 2023.
 * @date    LAST UPDATE: June 21, 2024.
 */
static HM10_Clone_Status HAL_ret_handler(HAL_StatusTypeDef HAL_status);

HM10_Clone_Status init_hm10_clone_module(UART_HandleTypeDef *huart)
{
	p_huart = huart;

	return HM10_Clone_EC_OK;
}

HM10_Clone_Status send_hm10clone_test_cmd()
{
	resp_attempts = 0; // Reset the response attempts counter.
	return send_test_cmd(); // Send the HM-10 Clone Device's Test Command.
}

static HM10_Clone_Status send_test_cmd()
{
	/* Flush the UART's RX before starting. */
	HAL_uart_rx_flush();

	/** <b>Local variable ret:</b> Return value of either a HAL function or a @ref HM10_Clone_Status function type. */
	int16_t  ret;

	/* Populate the HM-10 Clone Device's Test Command into the Tx/Rx Buffer. */
	#if ETX_OTA_VERBOSE
		printf("Sending Test Command to HM-10 Clone BLE Device...\r\n");
	#endif
	TxRx_Buffer[0] = 'A';
	TxRx_Buffer[1] = 'T';
	TxRx_Buffer[2] = '\r';
	TxRx_Buffer[3] = '\n';

	/* Send the HM-10 Clone Device's Test Command. */
	ret = HAL_UART_Transmit(p_huart, TxRx_Buffer, HM10_CLONE_TEST_CMD_SIZE, HM10_CLONE_CUSTOM_HAL_TIMEOUT);
	ret = HAL_ret_handler(ret);
	if (ret != HAL_OK)
	{
		if (resp_attempts == 0)
		{
			resp_attempts++;
			#if ETX_OTA_VERBOSE
				printf("WARNING: Attempt %d to transmit Test Command to HM-10 Clone BLE Device has failed.\r\n", resp_attempts);
			#endif
			ret = send_test_cmd();
		}
		#if ETX_OTA_VERBOSE
			else
			{
				printf("ERROR: Last attempt for transmitting the Test Command to HM-10 Clone BLE Device has failed.\r\n");
			}
		#endif
		return ret;
	}

	/* Receive the HM-10 Clone Device's Response. */
	ret = HAL_UART_Receive(p_huart, TxRx_Buffer, HM10_CLONE_OK_RESPONSE_SIZE, HM10_CLONE_CUSTOM_HAL_TIMEOUT);
	ret = HAL_ret_handler(ret);
	if (ret != HAL_OK)
	{
		if (resp_attempts == 0)
		{
			resp_attempts++;
			#if ETX_OTA_VERBOSE
				printf("WARNING: Attempt %d to receive OK Response from HM-10 Clone BLE Device has failed.\r\n", resp_attempts);
			#endif
			ret = send_test_cmd();
		}
		#if ETX_OTA_VERBOSE
			else
			{
				printf("ERROR: An OK Response from the HM-10 Clone BLE Device was expected, but none was received (HM-10 Clone Exception code = %d)\r\n", ret);
			}
		#endif
		return ret;
	}

	/* Validate the HM-10 Clone Device's Response. */
	for (int i=0; i<HM10_CLONE_OK_RESPONSE_SIZE; i++)
	{
		if (TxRx_Buffer[i] != HM10_Clone_OK_resp[i])
		{
			#if ETX_OTA_VERBOSE
				printf("ERROR: An OK Response from the HM-10 Clone BLE Device was expected, but something else was received instead.\r\n");
			#endif
			return HM10_Clone_EC_ERR;
		}
	}
	#if ETX_OTA_VERBOSE
		printf("DONE: A Test Command was successfully sent to the HM-10 Clone BLE Device.\r\n");
	#endif

	return HM10_Clone_EC_OK;
}

HM10_Clone_Status send_hm10clone_reset_cmd()
{
	resp_attempts = 0; // Reset the response attempts counter.
	return send_reset_cmd(); // Send the HM-10 Clone Device's Reset Command.
}

static HM10_Clone_Status send_reset_cmd()
{
	/* Flush the UART's RX before starting. */
	HAL_uart_rx_flush();

	/** <b>Local variable ret:</b> Return value of either a HAL function or a @ref HM10_Clone_Status function type. */
	int16_t  ret;

	/* Populate the HM-10 Clone Device's Reset Command into the Tx/Rx Buffer. */
	#if ETX_OTA_VERBOSE
		printf("Sending Reset Command to HM-10 Clone BLE Device...\r\n");
	#endif
	TxRx_Buffer[0] = 'A';
	TxRx_Buffer[1] = 'T';
	TxRx_Buffer[2] = '+';
	TxRx_Buffer[3] = 'R';
	TxRx_Buffer[4] = 'E';
	TxRx_Buffer[5] = 'S';
	TxRx_Buffer[6] = 'E';
	TxRx_Buffer[7] = 'T';
	TxRx_Buffer[8] = '\r';
	TxRx_Buffer[9] = '\n';

	/* Send the HM-10 Clone Device's Reset Command. */
	ret = HAL_UART_Transmit(p_huart, TxRx_Buffer, HM10_CLONE_RESET_CMD_SIZE, HM10_CLONE_CUSTOM_HAL_TIMEOUT);
	ret = HAL_ret_handler(ret);
	if (ret != HAL_OK)
	{
		if (resp_attempts == 0)
		{
			resp_attempts++;
			#if ETX_OTA_VERBOSE
				printf("WARNING: Attempt %d to transmit Reset Command to HM-10 Clone BLE Device has failed.\r\n", resp_attempts);
			#endif
			ret = send_reset_cmd();
		}
		#if ETX_OTA_VERBOSE
			else
			{
				printf("ERROR: Last attempt for transmitting the Reset Command to HM-10 Clone BLE Device has failed.\r\n");
			}
		#endif
		return ret;
	}

	/* Receive the HM-10 Clone Device's OK Response. */
	ret = HAL_UART_Receive(p_huart, TxRx_Buffer, HM10_CLONE_OK_RESPONSE_SIZE, HM10_CLONE_CUSTOM_HAL_TIMEOUT);
	ret = HAL_ret_handler(ret);
	if (ret != HAL_OK)
	{
		if (resp_attempts == 0)
		{
			resp_attempts++;
			#if ETX_OTA_VERBOSE
				printf("WARNING: Attempt %d to receive OK Response from HM-10 Clone BLE Device has failed.\r\n", resp_attempts);
			#endif
			ret = send_reset_cmd();
		}
		#if ETX_OTA_VERBOSE
			else
			{
				printf("ERROR: An OK Response from the HM-10 Clone BLE Device was expected, but none was received (HM-10 Clone Exception code = %d)\r\n", ret);
			}
		#endif
		return ret;
	}

	/* Validate the HM-10 Clone Device's Response. */
	for (int i=0; i<HM10_CLONE_OK_RESPONSE_SIZE; i++)
	{
		if (TxRx_Buffer[i] != HM10_Clone_OK_resp[i])
		{
			#if ETX_OTA_VERBOSE
				printf("ERROR: An OK Response from the HM-10 Clone BLE Device was expected, but something else was received instead.\r\n");
			#endif
			return HM10_Clone_EC_ERR;
		}
	}
	#if ETX_OTA_VERBOSE
		printf("DONE: A Reset Command was successfully sent to the HM-10 Clone BLE Device.\r\n");
	#endif

	return HM10_Clone_EC_OK;
}

HM10_Clone_Status set_hm10clone_name(uint8_t *hm10_name, uint8_t size)
{
	resp_attempts = 0; // Reset the response attempts counter.
	return send_set_name_cmd(hm10_name, size); // Send the HM-10 Clone Device's Name Command with the desired name to set to it.
}

static HM10_Clone_Status send_set_name_cmd(uint8_t *hm10_name, uint8_t size)
{
	/* Flush the UART's RX before starting. */
	HAL_uart_rx_flush();

	/* Validating given name. */
	if (size > HM10_CLONE_MAX_BLE_NAME_SIZE)
	{
		#if ETX_OTA_VERBOSE
			printf("ERROR: Requested BLE Name must not exceed a length of %d bytes (i.e., %d ASCII Characters).\r\n", HM10_CLONE_MAX_BLE_NAME_SIZE, HM10_CLONE_MAX_BLE_NAME_SIZE);
		#endif
		return HM10_Clone_EC_ERR;
	}

	/** <b>Local variable ret:</b> Return value of either a HAL function or a @ref HM10_Clone_Status function type. */
	int16_t  ret;
	/** <b>Local variable bytes_populated_in_TxRx_Buffer:</b> Currently populated data into the Tx/Rx Global Buffer. */
	uint8_t bytes_populated_in_TxRx_Buffer = 0;

	/* Populate the HM-10 Clone Device's Name Command into the Tx/Rx Buffer. */
	#if ETX_OTA_VERBOSE
		printf("Sending Name Command to HM-10 Clone BLE Device...\r\n");
	#endif
	TxRx_Buffer[bytes_populated_in_TxRx_Buffer++] = 'A';
	TxRx_Buffer[bytes_populated_in_TxRx_Buffer++] = 'T';
	TxRx_Buffer[bytes_populated_in_TxRx_Buffer++] = '+';
	TxRx_Buffer[bytes_populated_in_TxRx_Buffer++] = 'N';
	TxRx_Buffer[bytes_populated_in_TxRx_Buffer++] = 'A';
	TxRx_Buffer[bytes_populated_in_TxRx_Buffer++] = 'M';
	TxRx_Buffer[bytes_populated_in_TxRx_Buffer++] = 'E';

	/** <b>Local variable size_with_offset:</b> Either the total size in bytes stated at the \p size param plus the bytes populated in the Tx/Rx Buffer for only the Name Command (i.e., without the carriage return and the new line ASCII characters), or the total size in bytes stated at the \p size param plus the bytes read from the Tx/Rx Buffer for only the Name Response (i.e., without the carriage return and the new line ASCII characters). */
	uint8_t size_with_offset = size + bytes_populated_in_TxRx_Buffer;
	for (uint8_t i=0; bytes_populated_in_TxRx_Buffer<size_with_offset; i++)
	{
		TxRx_Buffer[bytes_populated_in_TxRx_Buffer++] = hm10_name[i];
	}

	TxRx_Buffer[bytes_populated_in_TxRx_Buffer++] = '\r';
	TxRx_Buffer[bytes_populated_in_TxRx_Buffer++] = '\n';

	/* Send the HM-10 Clone Device's Name Command. */
	ret = HAL_UART_Transmit(p_huart, TxRx_Buffer, bytes_populated_in_TxRx_Buffer, HM10_CLONE_CUSTOM_HAL_TIMEOUT);
	ret = HAL_ret_handler(ret);
	if (ret != HAL_OK)
	{
		if (resp_attempts == 0)
		{
			resp_attempts++;
			#if ETX_OTA_VERBOSE
				printf("WARNING: Attempt %d to transmit Name Command to HM-10 Clone BLE Device has failed.\r\n", resp_attempts);
			#endif
			ret = send_set_name_cmd(hm10_name, size);
		}
		#if ETX_OTA_VERBOSE
			else
			{
				printf("ERROR: Last attempt for transmitting the Name Command to HM-10 Clone BLE Device has failed.\r\n");
			}
		#endif
		return ret;
	}

	/* Receive the HM-10 Clone Device's Name Response. */
	bytes_populated_in_TxRx_Buffer = HM10_CLONE_NAME_RESPONSE_SIZE_WITHOUT_REQUESTED_NAME + size;
	ret = HAL_UART_Receive(p_huart, TxRx_Buffer, bytes_populated_in_TxRx_Buffer, HM10_CLONE_CUSTOM_HAL_TIMEOUT);
	ret = HAL_ret_handler(ret);
	if (ret != HAL_OK)
	{
		if (resp_attempts == 0)
		{
			resp_attempts++;
			#if ETX_OTA_VERBOSE
				printf("WARNING: Attempt %d to receive Name Response from HM-10 Clone BLE Device has failed.\r\n", resp_attempts);
			#endif
			ret = send_set_name_cmd(hm10_name, size);
		}
		#if ETX_OTA_VERBOSE
			else
			{
				printf("ERROR: A Name Response from the HM-10 Clone BLE Device was expected, but none was received (HM-10 Clone Exception code = %d)\r\n", ret);
			}
		#endif
		return ret;
	}

	/* Validate the HM-10 Clone Device's Name Response. */
	/** <b>Local variable name_resp_size_without_cr_and_lf:</b> Size in bytes of the Name Response from the HM-10 Clone BLE device but without considering the length of the requested name and withouth the Carriage Return and New Line bytes. */
	uint8_t name_resp_size_without_cr_and_lf = HM10_CLONE_NAME_RESPONSE_SIZE_WITHOUT_REQUESTED_NAME - CR_AND_LF_SIZE;
	/** <b>Local variable bytes_compared:</b> Counter for the bytes that have been compared and validated to match between the received Name Response (which should be stored in @ref TxRx_Buffer buffer ) and the expected Name Response (i.e., @ref HM10_Clone_Name_resp ). */
	uint8_t bytes_compared = 0;
	for (; bytes_compared<name_resp_size_without_cr_and_lf; bytes_compared++)
	{
		if (TxRx_Buffer[bytes_compared] != HM10_Clone_Name_resp[bytes_compared])
		{
			#if ETX_OTA_VERBOSE
				printf("ERROR: A Name Response from the HM-10 Clone BLE Device was expected, but something else was received instead.\r\n");
			#endif
			return HM10_Clone_EC_ERR;
		}
	}
	size_with_offset = size + bytes_compared;
	for (uint8_t i=0; bytes_compared<size_with_offset; i++)
	{
		if (TxRx_Buffer[bytes_compared++] != hm10_name[i])
		{
			#if ETX_OTA_VERBOSE
				printf("ERROR: A Name Response from the HM-10 Clone BLE Device was expected, but something else was received instead.\r\n");
			#endif
			return HM10_Clone_EC_ERR;
		}
	}
	if ((TxRx_Buffer[bytes_compared]!='\r') || (TxRx_Buffer[bytes_compared+1]!='\n'))
	{
		#if ETX_OTA_VERBOSE
			printf("ERROR: A Name Response from the HM-10 Clone BLE Device was expected, but something else was received instead.\r\n");
		#endif
		return HM10_Clone_EC_ERR;
	}

	/* Receive the HM-10 Clone Device's Response. */
	ret = HAL_UART_Receive(p_huart, TxRx_Buffer, HM10_CLONE_OK_RESPONSE_SIZE, HM10_CLONE_CUSTOM_HAL_TIMEOUT);
	ret = HAL_ret_handler(ret);
	if (ret != HAL_OK)
	{
		if (resp_attempts == 0)
		{
			resp_attempts++;
			#if ETX_OTA_VERBOSE
				printf("WARNING: Attempt %d to receive OK Response from HM-10 Clone BLE Device has failed.\r\n", resp_attempts);
			#endif
			ret = send_set_name_cmd(hm10_name, size);
		}
		#if ETX_OTA_VERBOSE
			else
			{
				printf("ERROR: An OK Response from the HM-10 Clone BLE Device was expected, but none was received (HM-10 Clone Exception code = %d)\r\n", ret);
			}
		#endif
		return ret;
	}

	/* Validate the HM-10 Clone Device's Response. */
	for (int i=0; i<HM10_CLONE_OK_RESPONSE_SIZE; i++)
	{
		if (TxRx_Buffer[i] != HM10_Clone_OK_resp[i])
		{
			#if ETX_OTA_VERBOSE
				printf("ERROR: An OK Response from the HM-10 Clone BLE Device was expected, but something else was received instead.\r\n");
			#endif
			return HM10_Clone_EC_ERR;
		}
	}
	#if ETX_OTA_VERBOSE
		printf("DONE: A Name Command was successfully sent to the HM-10 Clone BLE Device.\r\n");
	#endif

	return HM10_Clone_EC_OK;
}

HM10_Clone_Status get_hm10clone_name(uint8_t *hm10_name, uint8_t *size)
{
	resp_attempts = 0; // Reset the response attempts counter.
	return send_get_name_cmd(hm10_name, size); // Get the HM-10 Clone Device's Name.
}

static HM10_Clone_Status send_get_name_cmd(uint8_t *hm10_name, uint8_t *size)
{
	/* Flush the UART's RX before starting. */
	HAL_uart_rx_flush();

	/** <b>Local variable ret:</b> Return value of either a HAL function or a @ref HM10_Clone_Status function type. */
	int16_t  ret;

	/* Populate the HM-10 Clone Device's Get Name Command into the Tx/Rx Buffer. */
	#if ETX_OTA_VERBOSE
		printf("Sending Get Name Command to HM-10 Clone BLE Device...\r\n");
	#endif
	TxRx_Buffer[0] = 'A';
	TxRx_Buffer[1] = 'T';
	TxRx_Buffer[2] = '+';
	TxRx_Buffer[3] = 'N';
	TxRx_Buffer[4] = 'A';
	TxRx_Buffer[5] = 'M';
	TxRx_Buffer[6] = 'E';
	TxRx_Buffer[7] = '\r';
	TxRx_Buffer[8] = '\n';

	/* Send the HM-10 Clone Device's Get Name Command. */
	ret = HAL_UART_Transmit(p_huart, TxRx_Buffer, HM10_CLONE_GET_NAME_CMD_SIZE, HM10_CLONE_CUSTOM_HAL_TIMEOUT);
	ret = HAL_ret_handler(ret);
	if (ret != HAL_OK)
	{
		if (resp_attempts == 0)
		{
			resp_attempts++;
			#if ETX_OTA_VERBOSE
				printf("WARNING: Attempt %d to transmit Get Name Command to HM-10 Clone BLE Device has failed.\r\n", resp_attempts);
			#endif
			ret = send_get_name_cmd(hm10_name, size);
		}
		#if ETX_OTA_VERBOSE
			else
			{
				printf("ERROR: Last attempt for transmitting the Get Name Command to HM-10 Clone BLE Device has failed.\r\n");
			}
		#endif
		return ret;
	}

	/* Receive the HM-10 Clone Device's Get Name Response but just before the BLE Name bytes. */
	ret = HAL_UART_Receive(p_huart, TxRx_Buffer, HM10_CLONE_NAME_RESPONSE_SIZE_WITHOUT_REQUESTED_NAME, HM10_CLONE_CUSTOM_HAL_TIMEOUT);
	ret = HAL_ret_handler(ret);
	if (ret != HAL_OK)
	{
		if (resp_attempts == 0)
		{
			resp_attempts++;
			#if ETX_OTA_VERBOSE
				printf("WARNING: Attempt %d to receive the Get Name Response from HM-10 Clone BLE Device has failed.\r\n", resp_attempts);
			#endif
			ret = send_get_name_cmd(hm10_name, size);
		}
		#if ETX_OTA_VERBOSE
			else
			{
				printf("ERROR: A Get Name Response from the HM-10 Clone BLE Device was expected, but none was received (HM-10 Clone Exception code = %d)\r\n", ret);
			}
		#endif
		return ret;
	}

	/* Validate the HM-10 Clone Device's Get Name Response but just before the BLE Name bytes (last 2 bytes already obtained should not be validated here). */
	/** <b>Local variable name_resp_size_without_cr_and_lf:</b> Size in bytes of the Name Response from the HM-10 Clone BLE device but without considering the length of the requested name and withouth the Carriage Return and New Line bytes. */
	uint8_t name_resp_size_without_cr_and_lf = HM10_CLONE_NAME_RESPONSE_SIZE_WITHOUT_REQUESTED_NAME - CR_AND_LF_SIZE;
	/** <b>Local variable bytes_populated_in_TxRx_Buffer:</b> Currently populated data into the Tx/Rx Global Buffer. */
	uint8_t bytes_populated_in_TxRx_Buffer = 0;
	for (; bytes_populated_in_TxRx_Buffer<name_resp_size_without_cr_and_lf; bytes_populated_in_TxRx_Buffer++)
	{
		if (TxRx_Buffer[bytes_populated_in_TxRx_Buffer] != HM10_Clone_Name_resp[bytes_populated_in_TxRx_Buffer])
		{
			#if ETX_OTA_VERBOSE
				printf("ERROR: A Get Name Response from the HM-10 Clone BLE Device was expected, but something else was received instead.\r\n");
			#endif
			return HM10_Clone_EC_ERR;
		}
	}

	/* Receive the BLE Name bytes part from the HM-10 Clone Device's Get Name Response. */
	*size = 0;
	bytes_populated_in_TxRx_Buffer += 2; // Adding last 2 bytes already obtained.
	do
	{
		/* Receive the next byte from the BLE Name. */
		ret = HAL_UART_Receive(p_huart, &TxRx_Buffer[bytes_populated_in_TxRx_Buffer++], 1, HM10_CLONE_CUSTOM_HAL_TIMEOUT);
		(*size)++;
		ret = HAL_ret_handler(ret);
		if (ret != HAL_OK)
		{
			if (resp_attempts == 0)
			{
				resp_attempts++;
				#if ETX_OTA_VERBOSE
					printf("WARNING: Attempt %d to receive the BLE Name from HM-10 Clone BLE Device has failed.\r\n", resp_attempts);
				#endif
				ret = send_get_name_cmd(hm10_name, size);
			}
			#if ETX_OTA_VERBOSE
				else
				{
					printf("ERROR: A BLE Name from the HM-10 Clone BLE Device was expected, but none was received (HM-10 Clone Exception code = %d)\r\n", ret);
				}
            #endif
            *size = 0;
			return ret;
		}

		/* Check if the BLE Name has been completely received. */
		if ((TxRx_Buffer[bytes_populated_in_TxRx_Buffer-2]=='\r') && (TxRx_Buffer[bytes_populated_in_TxRx_Buffer-1]=='\n'))
		{
			break;
		}

		/* Validate the BLE Name bytes that have been received so far. */
		if (bytes_populated_in_TxRx_Buffer == (HM10_CLONE_MAX_BLE_NAME_SIZE + HM10_CLONE_NAME_RESPONSE_SIZE_WITHOUT_REQUESTED_NAME))
		{
			if (resp_attempts == 0)
			{
				resp_attempts++;
				#if ETX_OTA_VERBOSE
					printf("WARNING: Attempt %d to receive the BLE Name, with a maximum size of %d, from HM-10 Clone BLE Device has failed.\r\n", resp_attempts, HM10_CLONE_MAX_BLE_NAME_SIZE);
				#endif
				ret = send_get_name_cmd(hm10_name, size);
			}
			else
			{
				ret = HM10_Clone_EC_ERR;
				#if ETX_OTA_VERBOSE
					printf("ERROR: A BLE Name, with a maximum size of %d, from the HM-10 Clone BLE Device was expected, but a larger name was received instead.\r\n", HM10_CLONE_MAX_BLE_NAME_SIZE);
				#endif
			}

			return ret;
		}
	}
	while (1);

	/* Pass the BLE Name from the Buffer that is storing it into the \p hm10_name param. */
	memcpy(hm10_name, &TxRx_Buffer[name_resp_size_without_cr_and_lf], *size);

	#if ETX_OTA_VERBOSE
		printf("DONE: The BLE Name was successfully received from the HM-10 Clone BLE Device.\r\n");
	#endif

	return HM10_Clone_EC_OK;
}

HM10_Clone_Status set_hm10clone_role(HM10_Clone_Role ble_role)
{
	resp_attempts = 0; // Reset the response attempts counter.
	return send_set_role_cmd(ble_role); // Send the HM-10 Clone Device's Role Command with the desired role to set to it.
}

static HM10_Clone_Status send_set_role_cmd(HM10_Clone_Role ble_role)
{
	/* Flush the UART's RX before starting. */
	HAL_uart_rx_flush();

	/* Validating given role. */
	switch (ble_role)
	{
		case HM10_Clone_Role_Peripheral:
		case HM10_Clone_Role_Central:
			break;
		default:
			#if ETX_OTA_VERBOSE
				printf("ERROR: Requested BLE Role %d is not recognized.\r\n", ble_role);
			#endif
			return HM10_Clone_EC_ERR;
	}

	/** <b>Local variable ret:</b> Return value of either a HAL function or a @ref HM10_Clone_Status function type. */
	int16_t  ret;

	/* Populate the HM-10 Clone Device's Role Command into the Tx/Rx Buffer. */
	#if ETX_OTA_VERBOSE
		printf("Sending Role Command to HM-10 Clone BLE Device...\r\n");
	#endif
	TxRx_Buffer[0] = 'A';
	TxRx_Buffer[1] = 'T';
	TxRx_Buffer[2] = '+';
	TxRx_Buffer[3] = 'R';
	TxRx_Buffer[4] = 'O';
	TxRx_Buffer[5] = 'L';
	TxRx_Buffer[6] = 'E';
	TxRx_Buffer[7] = ble_role;
	TxRx_Buffer[8] = '\r';
	TxRx_Buffer[9] = '\n';

	/* Send the HM-10 Clone Device's Role Command. */
	ret = HAL_UART_Transmit(p_huart, TxRx_Buffer, HM10_CLONE_SET_ROLE_CMD_SIZE, HM10_CLONE_CUSTOM_HAL_TIMEOUT);
	ret = HAL_ret_handler(ret);
	if (ret != HAL_OK)
	{
		if (resp_attempts == 0)
		{
			resp_attempts++;
			#if ETX_OTA_VERBOSE
				printf("WARNING: Attempt %d to transmit Role Command to HM-10 Clone BLE Device has failed.\r\n", resp_attempts);
			#endif
			ret = send_set_role_cmd(ble_role);
		}
		#if ETX_OTA_VERBOSE
			else
			{
				printf("ERROR: Last attempt for transmitting the Role Command to HM-10 Clone BLE Device has failed.\r\n");
			}
		#endif
		return ret;
	}

	/* Receive the HM-10 Clone Device's Role Response. */
	ret = HAL_UART_Receive(p_huart, TxRx_Buffer, HM10_CLONE_ROLE_RESPONSE_SIZE, HM10_CLONE_CUSTOM_HAL_TIMEOUT);
	ret = HAL_ret_handler(ret);
	if (ret != HAL_OK)
	{
		if (resp_attempts == 0)
		{
			resp_attempts++;
			#if ETX_OTA_VERBOSE
				printf("WARNING: Attempt %d to receive Role Response from HM-10 Clone BLE Device has failed.\r\n", resp_attempts);
			#endif
			ret = send_set_role_cmd(ble_role);
		}
		#if ETX_OTA_VERBOSE
			else
			{
				printf("ERROR: A Role Response from the HM-10 Clone BLE Device was expected, but none was received (HM-10 Clone Exception code = %d)\r\n", ret);
			}
		#endif
		return ret;
	}

	/* Validate the HM-10 Clone Device's Role Response. */
	/** <b>Local variable role_resp_size_without_role_cr_and_lf:</b> Size in bytes of the Role Response from the HM-10 Clone BLE device but without considering the length of the requested role and without the Carriage Return and New Line bytes. */
	uint8_t role_resp_size_without_role_cr_and_lf = HM10_CLONE_ROLE_RESPONSE_SIZE - 3;
	/** <b>Local variable bytes_compared:</b> Counter for the bytes that have been compared and validated to match between the received Role Response (which should be stored in @ref TxRx_Buffer buffer ) and the expected Role Response (i.e., @ref HM10_Clone_Role_resp ). */
	uint8_t bytes_compared = 0;
	for (; bytes_compared<role_resp_size_without_role_cr_and_lf; bytes_compared++)
	{
		if (TxRx_Buffer[bytes_compared] != HM10_Clone_Role_resp[bytes_compared])
		{
			#if ETX_OTA_VERBOSE
				printf("ERROR: A Role Response from the HM-10 Clone BLE Device was expected, but something else was received instead.\r\n");
			#endif
			return HM10_Clone_EC_ERR;
		}
	}
	if ((TxRx_Buffer[bytes_compared]!=ble_role) || (TxRx_Buffer[bytes_compared+1]!='\r') || (TxRx_Buffer[bytes_compared+2]!='\n'))
	{
		#if ETX_OTA_VERBOSE
			printf("ERROR: A Role Response from the HM-10 Clone BLE Device was expected, but something else was received instead.\r\n");
		#endif
		return HM10_Clone_EC_ERR;
	}
	#if ETX_OTA_VERBOSE
		printf("DONE: A Role Command was successfully sent to the HM-10 Clone BLE Device.\r\n");
	#endif

	return HM10_Clone_EC_OK;
}

HM10_Clone_Status get_hm10clone_role(HM10_Clone_Role *ble_role)
{
	resp_attempts = 0; // Reset the response attempts counter.
	return send_get_role_cmd(ble_role); // Get the HM-10 Clone Device's Role.
}

static HM10_Clone_Status send_get_role_cmd(HM10_Clone_Role *ble_role)
{
	/* Flush the UART's RX before starting. */
	HAL_uart_rx_flush();

	/** <b>Local variable ret:</b> Return value of either a HAL function or a @ref HM10_Clone_Status function type. */
	int16_t  ret;

	/* Populate the HM-10 Clone Device's Get Role Command into the Tx/Rx Buffer. */
	#if ETX_OTA_VERBOSE
		printf("Sending Get Role Command to HM-10 Clone BLE Device...\r\n");
	#endif
	TxRx_Buffer[0] = 'A';
	TxRx_Buffer[1] = 'T';
	TxRx_Buffer[2] = '+';
	TxRx_Buffer[3] = 'R';
	TxRx_Buffer[4] = 'O';
	TxRx_Buffer[5] = 'L';
	TxRx_Buffer[6] = 'E';
	TxRx_Buffer[7] = '\r';
	TxRx_Buffer[8] = '\n';

	/* Send the HM-10 Clone Device's Get Role Command. */
	ret = HAL_UART_Transmit(p_huart, TxRx_Buffer, HM10_CLONE_GET_ROLE_CMD_SIZE, HM10_CLONE_CUSTOM_HAL_TIMEOUT);
	ret = HAL_ret_handler(ret);
	if (ret != HAL_OK)
	{
		if (resp_attempts == 0)
		{
			resp_attempts++;
			#if ETX_OTA_VERBOSE
				printf("WARNING: Attempt %d to transmit Get Role Command to HM-10 Clone BLE Device has failed.\r\n", resp_attempts);
			#endif
			ret = send_get_role_cmd(ble_role);
		}
		#if ETX_OTA_VERBOSE
			else
			{
				printf("ERROR: Last attempt for transmitting the Get Role Command to HM-10 Clone BLE Device has failed.\r\n");
			}
		#endif
		return ret;
	}

	/* Receive the HM-10 Clone Device's Get Role Response. */
	ret = HAL_UART_Receive(p_huart, TxRx_Buffer, HM10_CLONE_ROLE_RESPONSE_SIZE, HM10_CLONE_CUSTOM_HAL_TIMEOUT);
	ret = HAL_ret_handler(ret);
	if (ret != HAL_OK)
	{
		if (resp_attempts == 0)
		{
			resp_attempts++;
			#if ETX_OTA_VERBOSE
				printf("WARNING: Attempt %d to receive the Get Role Response from HM-10 Clone BLE Device has failed.\r\n", resp_attempts);
			#endif
			ret = send_get_role_cmd(ble_role);
		}
		#if ETX_OTA_VERBOSE
			else
			{
				printf("ERROR: A Get Role Response from the HM-10 Clone BLE Device was expected, but none was received (HM-10 Clone Exception code = %d)\r\n", ret);
			}
		#endif
		return ret;
	}

	/* Validate the HM-10 Clone Device's Get Role Response. */
	/** <b>Local variable role_resp_size_without_role_cr_and_lf:</b> Size in bytes of the Role Response from the HM-10 Clone BLE device but without considering the length of the role value and without the Carriage Return and New Line bytes. */
	uint8_t role_resp_size_without_role_cr_and_lf = HM10_CLONE_ROLE_RESPONSE_SIZE - 3;
	/** <b>Local variable bytes_compared:</b> Counter for the bytes that have been compared and validated to match between the received Role Response (which should be stored in @ref TxRx_Buffer buffer ) and the expected Role Response (i.e., @ref HM10_Clone_Role_resp ). */
	uint8_t bytes_compared = 0;
	for (; bytes_compared<role_resp_size_without_role_cr_and_lf; bytes_compared++)
	{
		if (TxRx_Buffer[bytes_compared] != HM10_Clone_Role_resp[bytes_compared])
		{
			#if ETX_OTA_VERBOSE
				printf("ERROR: A Get Role Response from the HM-10 Clone BLE Device was expected, but something else was received instead.\r\n");
			#endif
			return HM10_Clone_EC_ERR;
		}
	}
	switch (TxRx_Buffer[bytes_compared])
	{
		case HM10_Clone_Role_Peripheral:
		case HM10_Clone_Role_Central:
			break;
		default:
			#if ETX_OTA_VERBOSE
				printf("ERROR: Received BLE Role %d is not recognized.\r\n", TxRx_Buffer[bytes_compared]);
			#endif
			return HM10_Clone_EC_ERR;
	}
	bytes_compared++;
	if ((TxRx_Buffer[bytes_compared]!='\r') || (TxRx_Buffer[bytes_compared+1]!='\n'))
	{
		#if ETX_OTA_VERBOSE
			printf("ERROR: A Role Response from the HM-10 Clone BLE Device was expected, but something else was received instead.\r\n");
		#endif
		return HM10_Clone_EC_ERR;
	}
	*ble_role = TxRx_Buffer[role_resp_size_without_role_cr_and_lf];

	#if ETX_OTA_VERBOSE
		printf("DONE: The BLE Role was successfully received from the HM-10 Clone BLE Device.\r\n");
	#endif

	return HM10_Clone_EC_OK;
}

HM10_Clone_Status set_hm10clone_pin(uint8_t *pin)
{
	resp_attempts = 0; // Reset the response attempts counter.
	return send_set_pin_cmd(pin); // Send the HM-10 Clone Device's Pin Command with the desired pin to set in it.
}

static HM10_Clone_Status send_set_pin_cmd(uint8_t *pin)
{
	/* Flush the UART's RX before starting. */
	HAL_uart_rx_flush();

	/* Validating given pin. */
	for (uint8_t current_pin_character=0; current_pin_character<HM10_CLONE_PIN_VALUE_SIZE; current_pin_character++)
	{
		switch (pin[current_pin_character])
		{
			case Number_0_in_ASCII:
			case Number_1_in_ASCII:
			case Number_2_in_ASCII:
			case Number_3_in_ASCII:
			case Number_4_in_ASCII:
			case Number_5_in_ASCII:
			case Number_6_in_ASCII:
			case Number_7_in_ASCII:
			case Number_8_in_ASCII:
			case Number_9_in_ASCII:
				break;
			default:
				#if ETX_OTA_VERBOSE
					printf("ERROR: Expected a number character value in ASCII code on given pin value at index %d, but the following ASCII value was given instead: %c.\r\n", current_pin_character, pin[current_pin_character]);
				#endif
				return HM10_Clone_EC_ERR;
		}
	}

	/** <b>Local variable ret:</b> Return value of either a HAL function or a @ref HM10_Clone_Status function type. */
	int16_t  ret;
	/** <b>Local variable bytes_populated_in_TxRx_Buffer:</b> Currently populated data into the Tx/Rx Global Buffer. */
	uint8_t bytes_populated_in_TxRx_Buffer = 0;
	/** <b>Local variable pin_cmd_size_without_cr_and_lf:</b> Size in bytes of the Pin Command in the HM-10 Clone BLE device but without considering the length of the Carriage Return and New Line bytes. */
	uint8_t pin_cmd_size_without_cr_and_lf = HM10_CLONE_SET_PIN_CMD_SIZE - CR_AND_LF_SIZE;

	/* Populate the HM-10 Clone Device's Pin Command into the Tx/Rx Buffer. */
	#if ETX_OTA_VERBOSE
		printf("Sending Pin Command to HM-10 Clone BLE Device...\r\n");
	#endif
	TxRx_Buffer[bytes_populated_in_TxRx_Buffer++] = 'A';
	TxRx_Buffer[bytes_populated_in_TxRx_Buffer++] = 'T';
	TxRx_Buffer[bytes_populated_in_TxRx_Buffer++] = '+';
	TxRx_Buffer[bytes_populated_in_TxRx_Buffer++] = 'P';
	TxRx_Buffer[bytes_populated_in_TxRx_Buffer++] = 'I';
	TxRx_Buffer[bytes_populated_in_TxRx_Buffer++] = 'N';
	for (uint8_t current_pin_character=0; bytes_populated_in_TxRx_Buffer<pin_cmd_size_without_cr_and_lf; current_pin_character++)
	{
		TxRx_Buffer[bytes_populated_in_TxRx_Buffer++] = pin[current_pin_character];
	}
	TxRx_Buffer[bytes_populated_in_TxRx_Buffer++] = '\r';
	TxRx_Buffer[bytes_populated_in_TxRx_Buffer++] = '\n';

	/* Send the HM-10 Clone Device's Role Command. */
	ret = HAL_UART_Transmit(p_huart, TxRx_Buffer, HM10_CLONE_SET_PIN_CMD_SIZE, HM10_CLONE_CUSTOM_HAL_TIMEOUT);
	ret = HAL_ret_handler(ret);
	if (ret != HAL_OK)
	{
		if (resp_attempts == 0)
		{
			resp_attempts++;
			#if ETX_OTA_VERBOSE
				printf("WARNING: Attempt %d to transmit Pin Command to HM-10 Clone BLE Device has failed.\r\n", resp_attempts);
			#endif
			ret = send_set_pin_cmd(pin);
		}
		#if ETX_OTA_VERBOSE
			else
			{
				printf("ERROR: Last attempt for transmitting the Pin Command to HM-10 Clone BLE Device has failed.\r\n");
			}
		#endif
		return ret;
	}

	/* Receive the HM-10 Clone Device's Pin Response. */
	ret = HAL_UART_Receive(p_huart, TxRx_Buffer, HM10_CLONE_PIN_RESPONSE_SIZE, HM10_CLONE_CUSTOM_HAL_TIMEOUT);
	ret = HAL_ret_handler(ret);
	if (ret != HAL_OK)
	{
		if (resp_attempts == 0)
		{
			resp_attempts++;
			#if ETX_OTA_VERBOSE
				printf("WARNING: Attempt %d to receive Pin Response from HM-10 Clone BLE Device has failed.\r\n", resp_attempts);
			#endif
			ret = send_set_pin_cmd(pin);
		}
		#if ETX_OTA_VERBOSE
			else
			{
				printf("ERROR: A Pin Response from the HM-10 Clone BLE Device was expected, but none was received (HM-10 Clone Exception code = %d)\r\n", ret);
			}
		#endif
		return ret;
	}

	/* Validate the HM-10 Clone Device's Pin Response. */
	/** <b>Local variable pin_resp_size_without_pin_cr_and_lf:</b> Size in bytes of the Pin Response from the HM-10 Clone BLE device but without considering the length of the pin value and without the Carriage Return and New Line bytes. */
	uint8_t pin_resp_size_without_pin_cr_and_lf = HM10_CLONE_PIN_RESPONSE_SIZE - HM10_CLONE_PIN_VALUE_SIZE - CR_AND_LF_SIZE;
	/** <b>Local variable pin_resp_size_without_cr_and_lf:</b> Size in bytes of the Pin Response from the HM-10 Clone BLE device but without considering the length of the Carriage Return and New Line bytes. */
	uint8_t pin_resp_size_without_cr_and_lf = HM10_CLONE_PIN_RESPONSE_SIZE - CR_AND_LF_SIZE;
	/** <b>Local variable bytes_compared:</b> Counter for the bytes that have been compared and validated to match between the received Pin Response (which should be stored in @ref TxRx_Buffer buffer ) and the expected Pin Response (i.e., @ref HM10_Clone_Pin_resp ). */
	uint8_t bytes_compared = 0;
	for (; bytes_compared<pin_resp_size_without_pin_cr_and_lf; bytes_compared++)
	{
		if (TxRx_Buffer[bytes_compared] != HM10_Clone_Pin_resp[bytes_compared])
		{
			#if ETX_OTA_VERBOSE
				printf("ERROR: A Pin Response from the HM-10 Clone BLE Device was expected, but something else was received instead.\r\n");
			#endif
			return HM10_Clone_EC_ERR;
		}
	}
	for (uint8_t current_pin_character=0; bytes_compared<pin_resp_size_without_cr_and_lf; current_pin_character++)
	{
		if (TxRx_Buffer[bytes_compared++] != pin[current_pin_character])
		{
			#if ETX_OTA_VERBOSE
				printf("ERROR: A Pin Response from the HM-10 Clone BLE Device was expected, but something else was received instead.\r\n");
			#endif
			return HM10_Clone_EC_ERR;
		}
	}
	if ((TxRx_Buffer[bytes_compared]!='\r') || (TxRx_Buffer[bytes_compared+1]!='\n'))
	{
		#if ETX_OTA_VERBOSE
			printf("ERROR: A Pin Response from the HM-10 Clone BLE Device was expected, but something else was received instead.\r\n");
		#endif
		return HM10_Clone_EC_ERR;
	}

	/* Receive the HM-10 Clone Device's OK Response. */
	ret = HAL_UART_Receive(p_huart, TxRx_Buffer, HM10_CLONE_OK_RESPONSE_SIZE, HM10_CLONE_CUSTOM_HAL_TIMEOUT);
	ret = HAL_ret_handler(ret);
	if (ret != HAL_OK)
	{
		if (resp_attempts == 0)
		{
			resp_attempts++;
			#if ETX_OTA_VERBOSE
				printf("WARNING: Attempt %d to receive OK Response from HM-10 Clone BLE Device has failed.\r\n", resp_attempts);
			#endif
			ret = send_set_pin_cmd(pin);
		}
		#if ETX_OTA_VERBOSE
			else
			{
				printf("ERROR: An OK Response from the HM-10 Clone BLE Device was expected, but none was received (HM-10 Clone Exception code = %d)\r\n", ret);
			}
		#endif
		return ret;
	}

	/* Validate the HM-10 Clone Device's OK Response. */
	for (int i=0; i<HM10_CLONE_OK_RESPONSE_SIZE; i++)
	{
		if (TxRx_Buffer[i] != HM10_Clone_OK_resp[i])
		{
			#if ETX_OTA_VERBOSE
				printf("ERROR: An OK Response from the HM-10 Clone BLE Device was expected, but something else was received instead.\r\n");
			#endif
			return HM10_Clone_EC_ERR;
		}
	}

	#if ETX_OTA_VERBOSE
		printf("DONE: A Pin Command was successfully sent to the HM-10 Clone BLE Device.\r\n");
	#endif

	return HM10_Clone_EC_OK;
}

HM10_Clone_Status get_hm10clone_pin(uint8_t *pin)
{
	resp_attempts = 0; // Reset the response attempts counter.
	return send_get_pin_cmd(pin); // Get the HM-10 Clone Device's Pin.
}

static HM10_Clone_Status send_get_pin_cmd(uint8_t *pin)
{
	/* Flush the UART's RX before starting. */
	HAL_uart_rx_flush();

	/** <b>Local variable ret:</b> Return value of either a HAL function or a @ref HM10_Clone_Status function type. */
	int16_t  ret;

	/* Populate the HM-10 Clone Device's Get Pin Command into the Tx/Rx Buffer. */
	#if ETX_OTA_VERBOSE
		printf("Sending Get Pin Command to HM-10 Clone BLE Device...\r\n");
	#endif
	TxRx_Buffer[0] = 'A';
	TxRx_Buffer[1] = 'T';
	TxRx_Buffer[2] = '+';
	TxRx_Buffer[3] = 'P';
	TxRx_Buffer[4] = 'I';
	TxRx_Buffer[5] = 'N';
	TxRx_Buffer[6] = '\r';
	TxRx_Buffer[7] = '\n';

	/* Send the HM-10 Clone Device's Get Pin Command. */
	ret = HAL_UART_Transmit(p_huart, TxRx_Buffer, HM10_CLONE_GET_PIN_CMD_SIZE, HM10_CLONE_CUSTOM_HAL_TIMEOUT);
	ret = HAL_ret_handler(ret);
	if (ret != HAL_OK)
	{
		if (resp_attempts == 0)
		{
			resp_attempts++;
			#if ETX_OTA_VERBOSE
				printf("WARNING: Attempt %d to transmit Get Pin Command to HM-10 Clone BLE Device has failed.\r\n", resp_attempts);
			#endif
			ret = send_get_pin_cmd(pin);
		}
		#if ETX_OTA_VERBOSE
			else
			{
				printf("ERROR: Last attempt for transmitting the Get Pin Command to HM-10 Clone BLE Device has failed.\r\n");
			}
		#endif
		return ret;
	}

	/* Receive the HM-10 Clone Device's Get Pin Response. */
	ret = HAL_UART_Receive(p_huart, TxRx_Buffer, HM10_CLONE_PIN_RESPONSE_SIZE, HM10_CLONE_CUSTOM_HAL_TIMEOUT);
	ret = HAL_ret_handler(ret);
	if (ret != HAL_OK)
	{
		if (resp_attempts == 0)
		{
			resp_attempts++;
			#if ETX_OTA_VERBOSE
				printf("WARNING: Attempt %d to receive the Get Pin Response from HM-10 Clone BLE Device has failed.\r\n", resp_attempts);
			#endif
			ret = send_get_pin_cmd(pin);
		}
		#if ETX_OTA_VERBOSE
			else
			{
				printf("ERROR: A Get Pin Response from the HM-10 Clone BLE Device was expected, but none was received (HM-10 Clone Exception code = %d)\r\n", ret);
			}
		#endif
		return ret;
	}

	/* Validate the HM-10 Clone Device's Get Pin Response. */
	/** <b>Local variable pin_resp_size_without_pin_cr_and_lf:</b> Size in bytes of the Pin Response from the HM-10 Clone BLE device but without considering the length of the pin value and without the Carriage Return and New Line bytes. */
	uint8_t pin_resp_size_without_pin_cr_and_lf = HM10_CLONE_PIN_RESPONSE_SIZE - HM10_CLONE_PIN_VALUE_SIZE - CR_AND_LF_SIZE;
	/** <b>Local variable bytes_compared:</b> Counter for the bytes that have been compared and validated to match between the received Pin Response (which should be stored in @ref TxRx_Buffer buffer ) and the expected Pin Response (i.e., @ref HM10_Clone_Pin_resp ). */
	uint8_t bytes_compared = 0;
	for (; bytes_compared<pin_resp_size_without_pin_cr_and_lf; bytes_compared++)
	{
		if (TxRx_Buffer[bytes_compared] != HM10_Clone_Pin_resp[bytes_compared])
		{
			#if ETX_OTA_VERBOSE
				printf("ERROR: A Pin Response from the HM-10 Clone BLE Device was expected, but something else was received instead.\r\n");
			#endif
			return HM10_Clone_EC_ERR;
		}
	}
	for (uint8_t current_pin_character=0; current_pin_character<HM10_CLONE_PIN_VALUE_SIZE; current_pin_character++)
	{
		switch (TxRx_Buffer[bytes_compared])
		{
			case Number_0_in_ASCII:
			case Number_1_in_ASCII:
			case Number_2_in_ASCII:
			case Number_3_in_ASCII:
			case Number_4_in_ASCII:
			case Number_5_in_ASCII:
			case Number_6_in_ASCII:
			case Number_7_in_ASCII:
			case Number_8_in_ASCII:
			case Number_9_in_ASCII:
				break;
			default:
				#if ETX_OTA_VERBOSE
					printf("ERROR: Expected a number character value in ASCII code on received pin value at index %d, but the following ASCII value was given instead: %c.\r\n", current_pin_character, TxRx_Buffer[bytes_compared]);
				#endif
				return HM10_Clone_EC_ERR;
		}
		bytes_compared++;
	}
	if ((TxRx_Buffer[bytes_compared]!='\r') || (TxRx_Buffer[bytes_compared+1]!='\n'))
	{
		#if ETX_OTA_VERBOSE
			printf("ERROR: A Pin Response from the HM-10 Clone BLE Device was expected, but something else was received instead.\r\n");
		#endif
		return HM10_Clone_EC_ERR;
	}

	/* Pass the BLE Pin from the Buffer that is storing it into the \p pin param. */
	memcpy(pin, &TxRx_Buffer[pin_resp_size_without_pin_cr_and_lf], HM10_CLONE_PIN_VALUE_SIZE);

	#if ETX_OTA_VERBOSE
		printf("DONE: The BLE Pin was successfully received from the HM-10 Clone BLE Device.\r\n");
	#endif

	return HM10_Clone_EC_OK;
}

HM10_Clone_Status set_hm10clone_pin_code_mode(HM10_Clone_Pin_Code_Mode pin_code_mode)
{
	resp_attempts = 0; // Reset the response attempts counter.
	return send_set_type_cmd(pin_code_mode); // Send the HM-10 Clone Device's Type Command with the desired pin code mode to set in it.
}

static HM10_Clone_Status send_set_type_cmd(HM10_Clone_Pin_Code_Mode pin_code_mode)
{
	/* Flush the UART's RX before starting. */
	HAL_uart_rx_flush();

	/* Validating given pin code mode. */
	switch (pin_code_mode)
	{
		case HM10_Clone_Pin_Code_DISABLED:
		case HM10_Clone_Pin_Code_ENABLED:
			break;
		default:
			#if ETX_OTA_VERBOSE
				printf("ERROR: An invalid pin code mode value has been given: %c_ASCII.\r\n", pin_code_mode);
			#endif
			return HM10_Clone_EC_ERR;
	}

	/** <b>Local variable ret:</b> Return value of either a HAL function or a @ref HM10_Clone_Status function type. */
	int16_t  ret;

	/* Populate the HM-10 Clone Device's Type Command into the Tx/Rx Buffer. */
	#if ETX_OTA_VERBOSE
		printf("Sending Type Command to HM-10 Clone BLE Device...\r\n");
	#endif
	TxRx_Buffer[0] = 'A';
	TxRx_Buffer[1] = 'T';
	TxRx_Buffer[2] = '+';
	TxRx_Buffer[3] = 'T';
	TxRx_Buffer[4] = 'Y';
	TxRx_Buffer[5] = 'P';
	TxRx_Buffer[6] = 'E';
	TxRx_Buffer[7] = pin_code_mode;
	TxRx_Buffer[8] = '\r';
	TxRx_Buffer[9] = '\n';

	/* Send the HM-10 Clone Device's Role Command. */
	ret = HAL_UART_Transmit(p_huart, TxRx_Buffer, HM10_CLONE_SET_TYPE_CMD_SIZE, HM10_CLONE_CUSTOM_HAL_TIMEOUT);
	ret = HAL_ret_handler(ret);
	if (ret != HAL_OK)
	{
		if (resp_attempts == 0)
		{
			resp_attempts++;
			#if ETX_OTA_VERBOSE
				printf("WARNING: Attempt %d to transmit Type Command to HM-10 Clone BLE Device has failed.\r\n", resp_attempts);
			#endif
			ret = send_set_type_cmd(pin_code_mode);
		}
		#if ETX_OTA_VERBOSE
			else
			{
				printf("ERROR: Last attempt for transmitting the Type Command to HM-10 Clone BLE Device has failed.\r\n");
			}
		#endif
		return ret;
	}

	/* Receive the HM-10 Clone Device's Type Response. */
	ret = HAL_UART_Receive(p_huart, TxRx_Buffer, HM10_CLONE_TYPE_RESPONSE_SIZE, HM10_CLONE_CUSTOM_HAL_TIMEOUT);
	ret = HAL_ret_handler(ret);
	if (ret != HAL_OK)
	{
		if (resp_attempts == 0)
		{
			resp_attempts++;
			#if ETX_OTA_VERBOSE
				printf("WARNING: Attempt %d to receive Type Response from HM-10 Clone BLE Device has failed.\r\n", resp_attempts);
			#endif
			ret = send_set_type_cmd(pin_code_mode);
		}
		#if ETX_OTA_VERBOSE
			else
			{
				printf("ERROR: A Type Response from the HM-10 Clone BLE Device was expected, but none was received (HM-10 Clone Exception code = %d)\r\n", ret);
			}
		#endif
		return ret;
	}

	/* Validate the HM-10 Clone Device's Type Response. */
	/** <b>Local variable type_resp_size_without_pin_cr_and_lf:</b> Size in bytes of the Type Response from the HM-10 Clone BLE device but without considering the length of the Type value and without the Carriage Return and New Line bytes. */
	uint8_t type_resp_size_without_pin_cr_and_lf = HM10_CLONE_TYPE_RESPONSE_SIZE - 3;
	/** <b>Local variable bytes_compared:</b> Counter for the bytes that have been compared and validated to match between the received Type Response (which should be stored in @ref TxRx_Buffer buffer ) and the expected Type Response (i.e., @ref HM10_Clone_Type_resp ). */
	uint8_t bytes_compared = 0;
	for (; bytes_compared<type_resp_size_without_pin_cr_and_lf; bytes_compared++)
	{
		if (TxRx_Buffer[bytes_compared] != HM10_Clone_Type_resp[bytes_compared])
		{
			#if ETX_OTA_VERBOSE
				printf("ERROR: A Type Response from the HM-10 Clone BLE Device was expected, but something else was received instead at index %d. The received value was %c_ASCII and the expected value is %c_ASCII.\r\n", bytes_compared, TxRx_Buffer[bytes_compared], HM10_Clone_Type_resp[bytes_compared]);
			#endif
			return HM10_Clone_EC_ERR;
		}
	}
	if (TxRx_Buffer[bytes_compared++] != pin_code_mode)
	{
		#if ETX_OTA_VERBOSE
		printf("ERROR: A Type Response from the HM-10 Clone BLE Device was expected, but something else was received instead at index %d. The received value was %c_ASCII and the expected value is %c_ASCII.\r\n", bytes_compared-1, TxRx_Buffer[bytes_compared-1], pin_code_mode);
		#endif
		return HM10_Clone_EC_ERR;
	}
	if ((TxRx_Buffer[bytes_compared]!='\r') || (TxRx_Buffer[bytes_compared+1]!='\n'))
	{
		#if ETX_OTA_VERBOSE
		printf("ERROR: A Type Response from the HM-10 Clone BLE Device was expected, but something else was received instead at index %d and %d. The received values were %c_ASCII and %c_ASCII respectively and the expected values were %c_ASCII and %c_ASCII respectively.\r\n", bytes_compared, bytes_compared+1, TxRx_Buffer[bytes_compared], TxRx_Buffer[bytes_compared+1], '\r', '\n');
		#endif
		return HM10_Clone_EC_ERR;
	}

	/* Receive the HM-10 Clone Device's OK Response. */
	ret = HAL_UART_Receive(p_huart, TxRx_Buffer, HM10_CLONE_OK_RESPONSE_SIZE, HM10_CLONE_CUSTOM_HAL_TIMEOUT);
	ret = HAL_ret_handler(ret);
	if (ret != HAL_OK)
	{
		if (resp_attempts == 0)
		{
			resp_attempts++;
			#if ETX_OTA_VERBOSE
				printf("WARNING: Attempt %d to receive OK Response from HM-10 Clone BLE Device has failed.\r\n", resp_attempts);
			#endif
			ret = send_set_type_cmd(pin_code_mode);
		}
		#if ETX_OTA_VERBOSE
			else
			{
				printf("ERROR: An OK Response from the HM-10 Clone BLE Device was expected, but none was received (HM-10 Clone Exception code = %d)\r\n", ret);
			}
		#endif
		return ret;
	}

	/* Validate the HM-10 Clone Device's OK Response. */
	for (int i=0; i<HM10_CLONE_OK_RESPONSE_SIZE; i++)
	{
		if (TxRx_Buffer[i] != HM10_Clone_OK_resp[i])
		{
			#if ETX_OTA_VERBOSE
				printf("ERROR: An OK Response from the HM-10 Clone BLE Device was expected, but something else was received instead.\r\n");
			#endif
			return HM10_Clone_EC_ERR;
		}
	}

	#if ETX_OTA_VERBOSE
		printf("DONE: A Type Command was successfully sent to the HM-10 Clone BLE Device.\r\n");
	#endif

	return HM10_Clone_EC_OK;
}

HM10_Clone_Status get_hm10clone_pin_code_mode(HM10_Clone_Pin_Code_Mode *pin_code_mode)
{
	resp_attempts = 0; // Reset the response attempts counter.
	return send_get_type_cmd(pin_code_mode); // Get the HM-10 Clone Device's Pin Code Mode.
}

static HM10_Clone_Status send_get_type_cmd(HM10_Clone_Pin_Code_Mode *pin_code_mode)
{
	/* Flush the UART's RX before starting. */
	HAL_uart_rx_flush();

	/** <b>Local variable ret:</b> Return value of either a HAL function or a @ref HM10_Clone_Status function type. */
	int16_t  ret;

	/* Populate the HM-10 Clone Device's Get Type Command into the Tx/Rx Buffer. */
	#if ETX_OTA_VERBOSE
		printf("Sending Get Type Command to HM-10 Clone BLE Device...\r\n");
	#endif
	TxRx_Buffer[0] = 'A';
	TxRx_Buffer[1] = 'T';
	TxRx_Buffer[2] = '+';
	TxRx_Buffer[3] = 'T';
	TxRx_Buffer[4] = 'Y';
	TxRx_Buffer[5] = 'P';
	TxRx_Buffer[6] = 'E';
	TxRx_Buffer[7] = '\r';
	TxRx_Buffer[8] = '\n';

	/* Send the HM-10 Clone Device's Get Type Command. */
	ret = HAL_UART_Transmit(p_huart, TxRx_Buffer, HM10_CLONE_GET_TYPE_CMD_SIZE, HM10_CLONE_CUSTOM_HAL_TIMEOUT);
	ret = HAL_ret_handler(ret);
	if (ret != HAL_OK)
	{
		if (resp_attempts == 0)
		{
			resp_attempts++;
			#if ETX_OTA_VERBOSE
				printf("WARNING: Attempt %d to transmit Get Type Command to HM-10 Clone BLE Device has failed.\r\n", resp_attempts);
			#endif
			ret = send_get_type_cmd(pin_code_mode);
		}
		#if ETX_OTA_VERBOSE
			else
			{
				printf("ERROR: Last attempt for transmitting the Get Type Command to HM-10 Clone BLE Device has failed.\r\n");
			}
		#endif
		return ret;
	}

	/* Receive the HM-10 Clone Device's Get Type Response. */
	ret = HAL_UART_Receive(p_huart, TxRx_Buffer, HM10_CLONE_TYPE_RESPONSE_SIZE, HM10_CLONE_CUSTOM_HAL_TIMEOUT);
	ret = HAL_ret_handler(ret);
	if (ret != HAL_OK)
	{
		if (resp_attempts == 0)
		{
			resp_attempts++;
			#if ETX_OTA_VERBOSE
				printf("WARNING: Attempt %d to receive the Get Type Response from HM-10 Clone BLE Device has failed.\r\n", resp_attempts);
			#endif
			ret = send_get_type_cmd(pin_code_mode);
		}
		#if ETX_OTA_VERBOSE
			else
			{
				printf("ERROR: A Get Type Response from the HM-10 Clone BLE Device was expected, but none was received (HM-10 Clone Exception code = %d)\r\n", ret);
			}
		#endif
		return ret;
	}

	/* Validate the HM-10 Clone Device's Get Type Response. */
	/** <b>Local variable type_resp_size_without_type_cr_and_lf:</b> Size in bytes of the Type Response from the HM-10 Clone BLE device but without considering the length of the Type value and without the Carriage Return and New Line bytes. */
	uint8_t type_resp_size_without_type_cr_and_lf = HM10_CLONE_TYPE_RESPONSE_SIZE - 3;
	/** <b>Local variable bytes_compared:</b> Counter for the bytes that have been compared and validated to match between the received Type Response (which should be stored in @ref TxRx_Buffer buffer ) and the expected Type Response (i.e., @ref HM10_Clone_Type_resp ). */
	uint8_t bytes_compared = 0;
	for (; bytes_compared<type_resp_size_without_type_cr_and_lf; bytes_compared++)
	{
		if (TxRx_Buffer[bytes_compared] != HM10_Clone_Type_resp[bytes_compared])
		{
			#if ETX_OTA_VERBOSE
				printf("ERROR: A Type Response from the HM-10 Clone BLE Device was expected, but something else was received instead.\r\n");
			#endif
			return HM10_Clone_EC_ERR;
		}
	}
	switch (TxRx_Buffer[bytes_compared])
	{
		case HM10_Clone_Pin_Code_DISABLED:
		case HM10_Clone_Pin_Code_ENABLED:
			break;
		default:
			#if ETX_OTA_VERBOSE
				printf("ERROR: An invalid pin code mode value has been given: %c_ASCII.\r\n", TxRx_Buffer[bytes_compared]);
			#endif
			return HM10_Clone_EC_ERR;
	}
	bytes_compared++;
	if ((TxRx_Buffer[bytes_compared]!='\r') || (TxRx_Buffer[bytes_compared+1]!='\n'))
	{
		#if ETX_OTA_VERBOSE
			printf("ERROR: A Type Response from the HM-10 Clone BLE Device was expected, but something else was received instead.\r\n");
		#endif
		return HM10_Clone_EC_ERR;
	}

	/* Pass the BLE Pin Code Mode from the Buffer that is storing it into the \p pin_code_mode param. */
	*pin_code_mode = TxRx_Buffer[type_resp_size_without_type_cr_and_lf];

	#if ETX_OTA_VERBOSE
		printf("DONE: The BLE Pin Code Mode was successfully received from the HM-10 Clone BLE Device.\r\n");
	#endif

	return HM10_Clone_EC_OK;
}

HM10_Clone_Status send_hm10clone_ota_data(uint8_t *ble_ota_data, uint16_t size, uint32_t timeout)
{
	/** <b>Local variable ret:</b> Return value of either a HAL function or a @ref HM10_Clone_Status function type. */
	int16_t  ret;

	/* Send the requested data Over the Air (OTA) via the HM-10 Clone BLE Device. */
	ret = HAL_UART_Transmit(p_huart, ble_ota_data, size, timeout);
	ret = HAL_ret_handler(ret);

	return ret;
}

HM10_Clone_Status get_hm10clone_ota_data(uint8_t *ble_ota_data, uint16_t size, uint32_t timeout)
{
	/** <b>Local variable ret:</b> Return value of either a HAL function or a @ref HM10_Clone_Status function type. */
	int16_t  ret;

	/* Receive the HM-10 Clone Device's BLE data that is received Over the Air (OTA), if there is any. */
	ret = HAL_UART_Receive(p_huart, ble_ota_data, size, timeout);
	ret = HAL_ret_handler(ret);

	return ret;
}

static void HAL_uart_rx_flush()
{
	/** <b>Local variable ret:</b> Return value of either a HAL function type. */
	HAL_StatusTypeDef  ret;

	/* Receive the HM-10 Clone Device's BLE data that is received Over the Air (OTA), if there is any. */
	ret = HAL_UART_Receive(p_huart, TxRx_Buffer, 1, HM10_CLONE_CUSTOM_HAL_TIMEOUT);
	if (ret != HAL_TIMEOUT)
	{
		HAL_uart_rx_flush();
	}
}

static HM10_Clone_Status HAL_ret_handler(HAL_StatusTypeDef HAL_status)
{
  switch (HAL_status)
    {
  	  case HAL_BUSY:
	  case HAL_TIMEOUT:
		return HM10_Clone_EC_NR;
	  case HAL_ERROR:
		return HM10_Clone_EC_ERR;
	  default:
		return HAL_status;
    }
}

/** @} */
