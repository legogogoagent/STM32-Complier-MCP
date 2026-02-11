// Includes ------------------------------------------------------------------
#include "main.h"
#include "Lifter_Main.h"
#include "Encoder.h"
#include "Modbus.h"

extern UART_HandleTypeDef huart2;

// Definition ---------------------------------------------------------
#define Encoder_RS485_Tx huart2		//115200 8N1
#define Encoder_RS485_Rx UART_2
#define INVERT_TILT_ENCODER
// Variables ---------------------------------------------------------
uint8_t Encoder_GetValue_CMD[3][8]={{0x01,0x03,0x00,0x00,0x00,0x02,0xC4,0x0B},{0x02,0x03,0x00,0x00,0x00,0x02,0xC4,0x38},{0x03,0x03,0x00,0x00,0x00,0x01,0x85,0xE8}};
uint8_t Encoder1_GetValue_CMD[8]= {0x01,0x03,0x00,0x00,0x00,0x02,0xC4,0x0B};
uint8_t Encoder2_GetValue_CMD[8]= {0x02,0x03,0x00,0x00,0x00,0x02,0xC4,0x38};
uint8_t Encoder3_GetValue_CMD[8]= {0x03,0x03,0x00,0x00,0x00,0x01,0x85,0xE8};

Encoder_TypeDef encoder;
bool Encoder_Serial_debug;
uint16_t Encoder_Pause_Timer;
// Subroutine -----------------------------------------------------
uint8_t Encoder_Serial_CheckPacket(uint8_t *rx_data, uint8_t len, uint8_t addr);


// Code -----------------------------------------------------------
void Encoder_Init(void)
{
	Encoder_Serial_debug=0;
	printf("Encoder_Serial_Init\n");
	encoder.Last_Addr = 0;
	Encoder_Pause_Timer=0;

}

void Encoder_Debug(bool enable)
{
	Encoder_Serial_debug=enable;
}

uint16_t Encoder_Read(uint8_t id)
{
	return encoder.Value[id-1];
}

uint8_t Encoder_Ready(void)
{
	if((encoder.Raw_Value[0]!=0)&&(encoder.Raw_Value[1]!=0)&&(encoder.Raw_Value[2]!=0))
		return 1;
	else
		return 0;
}

uint8_t Encoder_Check_Pause(void)
{
	if(Encoder_Pause_Timer==0)
		return 0;
	Encoder_Pause_Timer--;
		return 1;
}

void Encoder_Set_Pause(uint16_t time)
{
	Encoder_Pause_Timer=time;
}

/********************************************************************
 * void Encoder_GetValue(void)
 * @brief  Encoder response time 700us
 *
 *******************************************************************/
void Encoder_GetValue(void)
{
	if(Encoder_Check_Pause()==0)
	{
		if(++encoder.Last_Addr > MAX_ENCODER)
			encoder.Last_Addr = 1;
	//	HAL_UART_Transmit_IT(&Encoder_RS485_Tx, Encoder_GetValue_CMD[encoder.Last_Addr-1], 8);
		while (__HAL_UART_GET_FLAG (&Encoder_RS485_Tx, UART_FLAG_TXE) == RESET){}
		while (__HAL_UART_GET_FLAG(&Encoder_RS485_Tx, UART_FLAG_TC) == RESET){}
		HAL_UART_Transmit_DMA(&Encoder_RS485_Tx, Encoder_GetValue_CMD[encoder.Last_Addr-1], 8);
	}
}

/********************************************************************
 * void Encoder_Serial_Handler(void)
 * @brief  Check and process serial packet from serial port,
 * 			should call this function in every main loop
 *
 *******************************************************************/
uint16_t Encoder_Serial_Handler(uint8_t* rx_data, uint16_t len)
{
uint32_t value=0;
uint8_t i;

	len = Encoder_Serial_CheckPacket(rx_data, len, encoder.Last_Addr);
	if(len!=0)
	{
		for(i=0;i<rx_data[2];i++)
		{
			value = value << 8;
			value|= rx_data[i+3];
		}
		if(Encoder_Serial_debug!=0)
			printf("Received encoder%d value= %d\n", encoder.Last_Addr, value);
#ifdef INVERT_TILT_ENCODER
		if((encoder.Last_Addr-1)==2)
			value = 4096 - value;
#endif
		encoder.Raw_Value[encoder.Last_Addr-1]=value;

		if(value > encoder.Zero_Offset[encoder.Last_Addr-1])
			value = value - encoder.Zero_Offset[encoder.Last_Addr-1];
		else
			value = 0;

		encoder.Value[encoder.Last_Addr-1]=value;

		return len;
	}
	return 0;
}

uint8_t Encoder_Serial_CheckPacket(uint8_t *rx_data, uint8_t len, uint8_t addr)  //return total packet length
{
uint8_t packet_len=0;
	if((rx_data[0]==addr) && (rx_data[1]==0x03))	//match address and read command
	{
		packet_len=(rx_data[2]+3);
		if(Modbus_CheckCRC(rx_data,packet_len))
			return packet_len+2;
	}
	return 0;
}

void Encoder_Write_Calibrate(void)
{
	encoder.Zero_Offset[0] = encoder.Raw_Value[0];
	encoder.Zero_Offset[1] = encoder.Raw_Value[1];
	encoder.Zero_Offset[2] = encoder.Raw_Value[2];
	printf("Encoder Zero Offset Data: ver:%d, hor:%d, tilt:%d\n",encoder.Zero_Offset[0], encoder.Zero_Offset[1], encoder.Zero_Offset[2]);
	Write_Lifter_Setting();
}

void Encoder_Show_Value(void)
{
	printf("Encoder Data: ver:%d, hor:%d, tilt:%d\n",encoder.Value[0], encoder.Value[1], encoder.Value[2]);
}

void Encoder_Show_Offset(void)
{
	printf("Encoder Offset: ver:%d, hor:%d, tilt:%d\n",encoder.Zero_Offset[0], encoder.Zero_Offset[1], encoder.Zero_Offset[2]);
}
