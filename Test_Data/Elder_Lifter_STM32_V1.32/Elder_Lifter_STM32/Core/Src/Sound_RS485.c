// Includes ------------------------------------------------------------------
#include "main.h"
#include "Lifter_Main.h"
#include "Sound_RS485.h"
#include "Modbus.h"

extern UART_HandleTypeDef huart3;

// Definition ---------------------------------------------------------
#define Sound_Tx_Port huart3		//115200 8N1

#define Sound_TxBuf_Size  32
#define Sound_RxBuf_Size  32

// Variables ---------------------------------------------------------
bool Sound_RS485_debug;
uint8_t Sound_TxBuf[Sound_TxBuf_Size];

uint8_t Sound_CMD_Volume[8]={0x01, 0x06, 0x00, 0x06, 0x00, 0x10, 0x78, 0x09};
uint8_t Sound_CMD[8]={0x01, 0x06, 0x00, 0x03, 0x00, 0x00, 0x78, 0x09};

// Subroutine -----------------------------------------------------

// Code -----------------------------------------------------------
void Sound_RS485_Init(void)
{
	Sound_RS485_debug=1;
	printf("Sound_RS485_Init\n");
}

//void Sound_RS485_Play (uint8_t sound)
//{
//uint8_t track;
//	if(sound>99)
//		sound = 99;
//	Sound_TxBuf[0]='@';
//	Sound_TxBuf[1]='P';
//	Sound_TxBuf[2]='l';
//	Sound_TxBuf[3]='a';
//	Sound_TxBuf[4]='y';
//	Sound_TxBuf[5]=',';
//	Sound_TxBuf[6]='0';
//	Sound_TxBuf[7]='0';
//	track = sound/10+'0';
//	Sound_TxBuf[8]=track;
//	track = sound%10+'0';
//	Sound_TxBuf[9]=track;
//	Sound_TxBuf[10]=',';
//	Sound_TxBuf[11]='$';
//
////	HAL_UART_Transmit_IT(&Panel_Tx_Port, Panel_Serial_TxBuffer, byte+2);
//	while (__HAL_UART_GET_FLAG (&Sound_Tx_Port, UART_FLAG_TXE) == RESET){}
//	while (__HAL_UART_GET_FLAG(&Sound_Tx_Port, UART_FLAG_TC) == RESET){}
//	HAL_UART_Transmit_DMA(&Sound_Tx_Port, Sound_TxBuf, 12);
//
//}

void Sound_RS485_Play (uint8_t sound)
{
uint16_t CRC16;
	HAL_Delay(10);	//Dummy Delay
	if(sound>99)
		sound = 99;
	Sound_CMD[5]=sound;
	CRC16=Modbus_CalCRC(Sound_CMD,6);
	Sound_CMD[6]=CRC16&0xff; //CRC low byte;
	Sound_CMD[7]=CRC16>>8;   //CRC high byte;

	while (__HAL_UART_GET_FLAG (&Sound_Tx_Port, UART_FLAG_TXE) == RESET){}
	while (__HAL_UART_GET_FLAG(&Sound_Tx_Port, UART_FLAG_TC) == RESET){}
	HAL_UART_Transmit_DMA(&Sound_Tx_Port, Sound_CMD, 8);
	HAL_Delay(10);	//Dummy Delay
}

void Sound_RS485_Volume(uint8_t volume)
{
uint16_t CRC16;
	HAL_Delay(10);	//Dummy Delay
	if(volume>99)
		volume = 99;
	Sound_CMD_Volume[5]=volume;
	CRC16=Modbus_CalCRC(Sound_CMD_Volume,6);
	Sound_CMD_Volume[6]=CRC16&0xff; //CRC low byte;
	Sound_CMD_Volume[7]=CRC16>>8;   //CRC high byte;

	while (__HAL_UART_GET_FLAG (&Sound_Tx_Port, UART_FLAG_TXE) == RESET){}
	while (__HAL_UART_GET_FLAG(&Sound_Tx_Port, UART_FLAG_TC) == RESET){}
	HAL_UART_Transmit_DMA(&Sound_Tx_Port, Sound_CMD_Volume, 8);

	HAL_Delay(20);		//wait for command handle by speaker

}
