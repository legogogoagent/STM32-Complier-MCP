// Includes -----------------------------------------------------
#include "main.h"
#include "Lifter_Main.h"
#include "stdarg.h"
#include "User_Uart.h"
#include "USB_Serial.h"
#include "Debug_Serial.h"

// Definition ---------------------------------------------------
//#define USB_VCP
//#define USB_PRINTF	//printf on USB serial also

#ifdef USB_VCP
#include "usb_device.h"
#include "usbd_cdc_if.h"
#endif

extern UART_HandleTypeDef huart1;

// Private variables --------------------------------------------
USB_Serial_TypeDef usb_serial;

//UART_RxTypeDef UART_1;

// Private function prototypes -----------------------------------

// Code ---------------------------------------------------------
void USB_Serial_Init(void)
{
	usb_serial.Rx_Ready=0;
//#ifndef USB_VCP
//	__HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
//	__HAL_UART_ENABLE_IT(&huart1, UART_IT_TC);
//	HAL_UART_Receive_DMA(&huart1, UART_1.Rx_DMA_Buf, RxPacketBuf_Size);
//#endif
}
void USB_Serial_Tx(uint8_t* data, uint16_t len)
{
//	if (hUsbDevice.dev_state == USBD_STATE_CONFIGURED)
//	  // plugged in & connected, so send this message periodically
#ifdef USB_VCP
	CDC_Transmit_FS(data, len);
#else
    while (__HAL_UART_GET_FLAG (&huart1, UART_FLAG_TXE) == RESET){}
    while (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TC) == RESET){}
	HAL_UART_Transmit_IT(&huart1, data, len);

#endif
}

void USB_Serial_Rx_ISR(uint8_t* data, uint32_t len)	//place at CDC_Receive_FS();
{
	if(usb_serial.Rx_Ready ==0)
	{
		usb_serial.Len = len;
		if(usb_serial.Len > MAX_SERIAL_RX_BUF)
			usb_serial.Len = MAX_SERIAL_RX_BUF;
		memset(usb_serial.Rx_Buf,'\0',MAX_SERIAL_RX_BUF);
		memcpy(usb_serial.Rx_Buf, data, usb_serial.Len);
		memset(data,'\0',usb_serial.Len);
		usb_serial.Rx_Ready = 1;
	}
}


unsigned char USB_Serial_Handler()
{
	if(usb_serial.Rx_Ready != 0)
	{
		Debug_Serial_Handler(usb_serial.Rx_Buf, usb_serial.Len);
		usb_serial.Len=0;
		usb_serial.Rx_Ready=0;	// clean flag
	}
	return 0;
}


#ifdef USB_PRINTF
int __io_putchar(int ch) {
	//Normal Mode
#ifndef USB_VCP
	HAL_UART_Transmit(&huart1, (uint8_t*)&ch, 1, 0xffff);
#endif
    return ch;
}

int _write(int file, char *ptr, int len)
{
	int DataIdx;
#ifdef USB_VCP
	USB_Serial_Tx((uint8_t*) ptr, len);
#else
	for (DataIdx = 0; DataIdx< len; DataIdx++)
	{
		__io_putchar(*ptr++);
	}
#endif

	return len;
}
#endif

