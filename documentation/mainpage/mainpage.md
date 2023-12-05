This documentation generated with Doxygen contains all the information required to be able to understand the variables,
definitions and functions provided to enable the implementer of this driver library to use the AT-09 zs040 Bluetooth
module in a desired application. This library was written in C programming language and it is meant to be used for
STMicroelectronics microcontrollers and microprocessors together with the libraries provided by that manufacturer.

Feel free to review the other tabs of this offline documentation in order to learn more about the features of that this
library has to offer and any considerations that you might need to have in mind before considering to acquire and
implement an AT-09 Bluetooth device into your project. Also, know that the documentation shown here is limited to only
what you will need to know to be able to use this library. However, in the case that you are interested in contributing
to this project, know that all the functions, variables and definitions of the source file (i.e., the .c file) of this
library are also documented with Doxygen syntax/format even though that their documentation is not shown here.

On the other hand, for reaching out a code example for implementing this library, review the detailed description of the
<a href=file:///C:/Users/cmira/OneDrive/Escritorio/GitHub/AT-09_zs040_ble_driver/documentation/html/df/d51/group__hm10__ble__clone.html#details>AT-09 CTFZ54812 ZS-040 Bluetooth Clone module</a>.

Last but not least, the configurations and details of the microcontroller used for testing and validating this library
can be reviewed at the
<a href=https://github.com/Mortrack/AT-09_zs040_ble_driver/blob/main/documentation/pdfs/STM32CubeMX_configurations_report.pdf>STM32CubeMX_configurations_report.pdf</a>
file, where it is important to highlight that the UART1 was used for the printf() function of the stdio.h C library and
that the UART3 was used for the AT-09 device.

<i><b><u>NOTE:</u></b> To be able to link the printf() function to the UART1 as it was done during the testings and
validations made for this library, make sure to copy-paste/implement the following code into your main.c file:</i>

```c
/*@brief	Compiler definition to be able to use the @ref printf function from stdio.h library in order to print
 *          characters via the UART1 Peripheral but by using that @ref printf function.
 *
 * @return	The \p ch param.
 *
 * author	César Miranda Meza
 * @date	July 07, 2023
 */
#ifdef __GNUC__
/* With GCC, small printf (option LD Linker->Libraries->Small printf set to 'Yes') calls __io_putchar(). */
int __io_putchar(int ch)
#else
int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
{
/* Place your implementation of fputc here. */
/* NOTE: The characters written into the UART1 Protocol will be looped until the end of transmission. */
HAL_UART_Transmit(&huart1, (uint8_t *) &ch, 1, HAL_MAX_DELAY);
return ch;
}
```
