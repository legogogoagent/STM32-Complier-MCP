// Includes ------------------------------------------------------------------
#include "main.h"
#include "Lifter_Main.h"
#include "BMS.h"

extern UART_HandleTypeDef huart2;

// Definition ---------------------------------------------------------
#define BMS_RS485_Tx huart2		//115200 8N1
#define BMS_RS485_Rx UART_2
// Variables ---------------------------------------------------------
uint8_t BMS_GetValue_CMD[7]= {0xDD, 0xA5, 0x03, 0x00, 0xFF, 0xFD, 0x77};


BMS_TypeDef bms;
bool BMS_Serial_debug;
// Subroutine -----------------------------------------------------
uint8_t BMS_Serial_CheckPacket(uint8_t *rx_data, uint8_t len);
bool BMS_CheckCRC(uint8_t * data, uint8_t total_length);


// Code -----------------------------------------------------------
void BMS_Init(void)
{
	BMS_Serial_debug=1;
	bms.voltage=0;
	bms.charging=0;
	printf("BMS_Serial_Init\n");
}

void BMS_Debug(bool enable)
{
	BMS_Serial_debug=enable;
}

uint8_t BMS_Read_Percent(void)
{
	return bms.percent;
}

uint8_t BMS_Read_Voltage(void)
{
	return bms.voltage;
}

uint8_t BMS_Ready(void)
{
	if(bms.voltage!=0)
		return 1;
	else
		return 0;
}


/********************************************************************
 * void BMS_GetValue(void)
 * @brief  BMS response time xxxus
 *
 *******************************************************************/
void BMS_GetValue(void)
{
	while (__HAL_UART_GET_FLAG (&BMS_RS485_Tx, UART_FLAG_TXE) == RESET){}
	while (__HAL_UART_GET_FLAG(&BMS_RS485_Tx, UART_FLAG_TC) == RESET){}
	HAL_UART_Transmit_DMA(&BMS_RS485_Tx, BMS_GetValue_CMD, 7);
	if(BMS_Serial_debug!=0)
		printf("BMS get value\n");}

/********************************************************************
 * void BMS_Serial_Handler(void)
 * @brief  Check and process serial packet from serial port,
 * 			should call this function in every main loop
 *
 *******************************************************************/
uint16_t BMS_Serial_Handler(uint8_t* rx_data, uint16_t len)
{
	len = BMS_Serial_CheckPacket(rx_data, len);
	if(len!=0)
	{
		bms.voltage = (float)(((uint16_t)rx_data[4]<<8) + rx_data[5])/100;
		bms.current = (float)(((uint16_t)rx_data[6]<<8) + rx_data[7])/100;	//todo: need to check negative value
		bms.percent = rx_data[23];
		BMS_Show_Value();
		return len;
	}
	return 0;
}

uint8_t BMS_Serial_CheckPacket(uint8_t *rx_data, uint8_t len)  //return total packet length
{
uint16_t packet_len=0;
	if((rx_data[0]==0xDD) && (rx_data[1]==0x03) && (rx_data[2]==0x00) && (rx_data[3]==0x26))	//match address and read command
	{
		packet_len=(((uint16_t)(rx_data[2])<<8) + rx_data[3]);
		if(BMS_CheckCRC(rx_data,packet_len))
			return packet_len+7;
	}
	return 0;
}

bool BMS_CheckCRC(uint8_t * data, uint8_t total_length)
{
uint16_t BMS_CRC, Packet_CRC;
	BMS_CRC=Packet_CRC;		//todo: add crc check later

	if(Packet_CRC==BMS_CRC)
		return 1;
	if(BMS_Serial_debug!=0)
		printf("BMS CRC Error\n");
	return 0;
}

void BMS_Show_Value(void)
{
	printf("BMS Data: vol:%.2fV, current:%.2fA, percent:%d\n", bms.voltage, bms.current, bms.percent);
}
