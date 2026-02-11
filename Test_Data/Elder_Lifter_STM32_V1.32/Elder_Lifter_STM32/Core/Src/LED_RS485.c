// Includes ------------------------------------------------------------------
#include "main.h"
#include "Lifter_Main.h"
#include "LED_RS485.h"

extern UART_HandleTypeDef huart3;

// Definition ---------------------------------------------------------
#define LED_Tx_Port huart3		//115200 8N1

#define LED_TxBuf_Size  32
#define LED_RxBuf_Size  32

// Variables ---------------------------------------------------------
bool LED_RS485_debug;
uint8_t LED_COLOR_CMD[21] = {0xDD, 0x55, 0xEE, 0x00, 0x00, 0x00, 0x01, 0x00, 0x99, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x10, 0x00, 0x00, 0x00, 0xAA, 0xBB};

// Subroutine -----------------------------------------------------

// Code -----------------------------------------------------------
void LED_RS485_Init(void)
{
	LED_RS485_debug=1;
	printf("LED_RS485_Init\n");
}

void LED_RS485_Color (uint32_t color_code, uint8_t duty)
{
uint32_t color;
	color = ((color_code>>16)&0xff)*duty/100;
	LED_COLOR_CMD[16]=(uint8_t)color;	//RED
	color = ((color_code>>8)&0xff)*duty/100;
	LED_COLOR_CMD[17]=(uint8_t)color;	//GREEN
	color = ((color_code)&0xff)*duty/100;
	LED_COLOR_CMD[18]=(uint8_t)color;	//BLUE

//	HAL_UART_Transmit_IT(&Panel_Tx_Port, Panel_Serial_TxBuffer, byte+2);
	while (__HAL_UART_GET_FLAG (&LED_Tx_Port, UART_FLAG_TXE) == RESET){}
	while (__HAL_UART_GET_FLAG(&LED_Tx_Port, UART_FLAG_TC) == RESET){}
	HAL_UART_Transmit_DMA(&LED_Tx_Port, LED_COLOR_CMD, 21);

}

