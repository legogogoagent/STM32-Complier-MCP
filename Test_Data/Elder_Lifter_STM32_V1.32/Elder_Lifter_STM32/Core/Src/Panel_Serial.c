// Includes ------------------------------------------------------------------
#include "main.h"
#include "Lifter_Main.h"
#include "Modbus.h"
#include "Encoder.h"
#include "Motor.h"
#include "Lifter_Task.h"
#include "Actuator_Control.h"
#include "GPIO.h"
#include "Panel_Serial.h"
#include "USB_Serial.h"
#include "LED_RS485.h"

extern UART_HandleTypeDef huart3;

// Definition ---------------------------------------------------------
//#define CHECK_SAME_PACKET
#define SKIP_CHECK_CRC

#define Panel_Tx_Port huart3		//115200 8N1

#define Panel_TxBuf_Size  32
#define Panel_RxBuf_Size  64
#define Panel_Serial_Header_Size 4
#define Panel_Serial_PacketLen 4
#define Panel_Serial_OPCODE  Panel_Serial_Header_Size +1  //header + packet length
#define Panel_Serial_PACKET_SEQ  Panel_Serial_Header_Size +2  //header + packet length + opcode
#define Panel_Serial_DATA  Panel_Serial_OPCODE +4  //header + packet length
#define Panel_Serial_Overhead_Size Panel_Serial_Header_Size+3   //header  + packet length + checksum

// Variables ---------------------------------------------------------
bool Panel_Serial_debug;
char Panel_Serial_Header[Panel_Serial_Header_Size]={'L','S','C','M'};

uint8_t Panel_Serial_PacketSeq, Panel_Serial_TxBuffer[Panel_TxBuf_Size], Panel_Serial_Debug;
uint16_t Panel_Serial_Timestamp;
uint8_t Panel_Serial_PacketSeqImage;

// Subroutine -----------------------------------------------------
void Panel_Serial_Sendout(uint8_t byte);
uint8_t Panel_Serial_CheckPacket(uint8_t *rx_data, uint8_t len, uint8_t *opcode);  //return packet length
uint8_t Panel_Serial_CheckSamePacket(uint8_t seq);
void Panel_Serial_SendACK(uint8_t opcode);
void Panel_Serial_SendNACK(uint8_t opcode);
void Panel_Serial_SendEncoder(void);

// Code -----------------------------------------------------------
void Panel_Serial_Init(void)
{
	Panel_Serial_debug=1;
	printf("Panel_Serial_Init\n");
	Panel_Serial_PacketSeqImage=0xff;
}

/********************************************************************
 * void Panel_Serial_Handler(void)
 * @brief  Check and process serial packet from serial port,
 * 			should call this function in every main loop
 *
 *******************************************************************/
uint16_t Panel_Serial_Handler(uint8_t* rx_data, uint16_t len)
{
uint8_t opcode, temp_speed;
uint16_t temp_v, temp_h, temp_t;
	len = Panel_Serial_CheckPacket(rx_data, len, &opcode);
	if(len!=0)
	{
#ifdef CHECK_SAME_PACKET
		if(Panel_Serial_CheckSamePacket(rx_data[Panel_Serial_PACKET_SEQ]))
			opcode=0; //Ignore Same Packet
#endif
		switch (opcode)
		{
			case '@':   //redirect debug message to printf
				USB_Serial_Tx(rx_data,len);
				break;

			case 0:
				if((Lifter_Error_Debug!=0) || (Panel_Serial_debug!=0))
					printf("Ignore same panel serial packet, seq:%d\n",rx_data[Panel_Serial_PACKET_SEQ]);
				Panel_Serial_SendNACK(opcode); //reject packet
				break;

			case 1:	//Get Encoder
//				if((Lifter_Error_Debug!=0) || (Panel_Serial_debug!=0))
//					printf("Get Encoder Data %d, %d, %d\n",encoder.Value[0], encoder.Value[1], encoder.Value[2]);
				Panel_Serial_SendEncoder();
				break;

			case 2:	//Set single motor
				if((Lifter_Error_Debug!=0) || (Panel_Serial_debug!=0))
					printf("set Motor: %d = %d\n",rx_data[9], (int8_t)rx_data[10]);
				Panel_Serial_SendACK(opcode);
				HAL_Delay(10);
				switch(rx_data[9])
				{
					case 1:
						Actuator_Set_Output(VER_MOTOR, (int8_t)rx_data[10]);
						LED_RS485_Color(LED_YELLOW_COLOR,10);
						break;
					case 2:
						Actuator_Set_Output(TILT_MOTOR, (int8_t)rx_data[10]);
						LED_RS485_Color(LED_YELLOW_COLOR,10);
						break;
					case 3:
						Actuator_Set_Output(HOR_MOTOR, (int8_t)rx_data[10]);
						LED_RS485_Color(LED_YELLOW_COLOR,10);
						break;
					case 4:
						Motor_Set_Output(LATCH_MOTOR, (int8_t)rx_data[10]);
						break;
					default:
						break;
				}
				break;
			case 4:	//push waypoints in bufer
				Lifter_Auto_Off_Update();
				if(Panel_Serial_CheckSamePacket(rx_data[Panel_Serial_PACKET_SEQ])==0)
				{
					temp_speed = rx_data[10];
					temp_v = (((uint16_t)rx_data[11]<<8)+rx_data[12]);
					temp_h = (((uint16_t)rx_data[13]<<8)+rx_data[14]);
					temp_t = (((uint16_t)rx_data[15]<<8)+rx_data[16]);
					if((Lifter_Error_Debug!=0) || (Panel_Serial_debug!=0))
						printf("Panel serial push waypoint speed= %d, v=%d, h=%d, t=%d\n", temp_speed, temp_v, temp_h, temp_t);
					Task_Push_Waypoint(temp_speed, temp_v, temp_h, temp_t);
				}
				Panel_Serial_SendACK(opcode);
				break;


			case 5:	//Clear waypoints
				Panel_Serial_SendACK(opcode);
				Lifter_Auto_Off_Update();
				if((Lifter_Error_Debug!=0) || (Panel_Serial_debug!=0))
					printf("Panel serial clear waypoint\n");
				Task_Clear_Waypoint();
//				Encoder_Set_Pause(60);	//pause get encoder for 3 second
				break;

			case 6:	//Stop all motor
				if((Lifter_Error_Debug!=0) || (Panel_Serial_debug!=0))
					printf("Stop all motor\n");
				Motor_Stop();
//				Encoder_Set_Pause(5);	//pause get encoder for 0.1 second
				Panel_Serial_SendACK(opcode);
				HAL_Delay(10);
				LED_RS485_Color(LED_GREEN_COLOR,10);
				break;

			case 7:	//Get system status
				if((Lifter_Error_Debug!=0) || (Panel_Serial_debug!=0))
					printf("Get System Status\n");
				Panel_Serial_SendStatus();
				break;

			case 8:	//Set waypoint speed
				Panel_Serial_SendACK(opcode);
				if((Lifter_Error_Debug!=0) || (Panel_Serial_debug!=0))
					printf("Panel serial set total speed up= %d, down=%d, repeat=%d, stand=%d, vol=%d\n", rx_data[10], rx_data[11], rx_data[12], rx_data[13], rx_data[14]);
				Task_Set_Speed(rx_data[10], rx_data[11]);
				Task_Set_Repeat(rx_data[12]);
				Task_Set_StandTime(rx_data[13]);
				Task_Set_Volume(rx_data[14]);
				break;

			case 9:	//Set lifter to home position
				Lifter_Auto_Off_Update();
				Panel_Serial_SendACK(opcode);
				if(rx_data[9]==1)
				{
					if((Lifter_Error_Debug!=0) || (Panel_Serial_debug!=0))
						printf("Set lifter to home position\n");
					Task_Move_Home();
				}
				else
				{
					if((Lifter_Error_Debug!=0) || (Panel_Serial_debug!=0))
						printf("Set lifter to home position stop\n");
					Task_Move_Stop();
				}
				break;

			case 10:	//Set lifter to repeat training
				Lifter_Auto_Off_Update();
				Panel_Serial_SendACK(opcode);
				if(rx_data[9]==1)
				{
					if((Lifter_Error_Debug!=0) || (Panel_Serial_debug!=0))
						printf("Set lifter repeat training start\n");
					Task_Repeat();
				}
				else
				{
					if((Lifter_Error_Debug!=0) || (Panel_Serial_debug!=0))
						printf("Set lifter repeat training stop\n");
					Task_Repeat_Pause();
				}
				break;

			case 11:	//Set lifter delay off
				if((Lifter_Error_Debug!=0) || (Panel_Serial_debug!=0))
					printf("Set lifter power off after %d second\n",rx_data[9]);
				Lifter_Set_Delay_Off((uint16_t)rx_data[9]);
				Panel_Serial_SendACK(opcode);
				break;

			default:
				if((Lifter_Error_Debug!=0) || (Panel_Serial_debug!=0))
					printf("Panel serial unknown command %d\n", opcode);
				break;
		}
		return len;
	}
	return 0;
}

uint8_t Panel_Serial_CheckPacket(uint8_t *rx_data, uint8_t len, uint8_t *opcode)  //return packet length
{
uint8_t i,packet_len;
	if(len<Panel_Serial_Header_Size)
		return 0;
	for(i=0; i<Panel_Serial_Header_Size; i++)
	{
		if(rx_data[i]!=Panel_Serial_Header[i])
			return 0;
	}
	//Serial.print("CheckPacket size:");  Serial.print(len, DEC); Serial.print(" len:");  Serial.print((rx_data[i]+Panel_Serial_Overhead_Size), DEC);Serial.println();
	if((i>=Panel_Serial_Header_Size)&&(len>=(rx_data[i]+Panel_Serial_Overhead_Size)))  //all header matched and packet size enough
	{
		packet_len=(rx_data[i]+Panel_Serial_Overhead_Size);
#ifdef SKIP_CHECK_CRC
		if(1)
#else
		if(Modbus_CheckCRC(rx_data,packet_len-2))
#endif
		{
			*opcode=rx_data[Panel_Serial_OPCODE];
			return packet_len;
		}
	}
	else if((i>=Panel_Serial_Header_Size)&&(rx_data[i]=='@')&&(rx_data[i+1]=='@')) //for debug message
	{
		for(;i<len;i++)
		{
			if(((rx_data[i-2]==0x0d)&&(rx_data[i-1]==0x0a)) || ((rx_data[i]!=0x0d)&&(rx_data[i]!=0x0a)&&(rx_data[i]<31)&&(rx_data[i]>128)) || ((rx_data[i]==Panel_Serial_Header[0])&&(rx_data[i+1]==Panel_Serial_Header[1])&&(rx_data[i+2]==Panel_Serial_Header[2])&&(rx_data[i+3]==Panel_Serial_Header[3])))
				break;
		}
		*opcode='@';
		return i;
	}
	return 0;
}

uint8_t Panel_Serial_CheckSamePacket(uint8_t seq)
{
	if(seq!=Panel_Serial_PacketSeqImage)
	{
		Panel_Serial_PacketSeqImage=seq;
		return 0;
	}
	else
	{
		return 1;
	}
}

void Panel_Serial_SendACK(uint8_t opcode)
{
	Panel_Serial_TxBuffer[4]=5;     //msgLength
	Panel_Serial_TxBuffer[5]=opcode;     //opcode
	//[6]=Packet Sequence
	//[7]=Timestamp high byte
	//[8]=Timestamp low byte
	Panel_Serial_TxBuffer[9]=1;   //accept=1; reject=0
	Panel_Serial_Sendout(10);
}

void Panel_Serial_SendNACK(uint8_t opcode)
{
	Panel_Serial_TxBuffer[4]=5;     //msgLength
	Panel_Serial_TxBuffer[5]=opcode;     //opcode
	//[6]=Packet Sequence
	//[7]=Timestamp high byte
	//[8]=Timestamp low byte
	Panel_Serial_TxBuffer[9]=0;   //accept=1; reject=0
	Panel_Serial_Sendout(10);
}

void Panel_Serial_Sendout(uint8_t byte)
{
uint16_t CRC16;
	Panel_Serial_Timestamp=(uint16_t)HAL_GetTick();
	Panel_Serial_TxBuffer[0]='L';
	Panel_Serial_TxBuffer[1]='S';
	Panel_Serial_TxBuffer[2]='C';
	Panel_Serial_TxBuffer[3]='M';

	Panel_Serial_TxBuffer[6]=Panel_Serial_PacketSeq++;   //Packet Sequence
	Panel_Serial_TxBuffer[7]=Panel_Serial_Timestamp>>8;   //Timestamp high byte;
	Panel_Serial_TxBuffer[8]=Panel_Serial_PacketSeq&0xff; //Timestamp low byte;

	CRC16=Modbus_CalCRC(Panel_Serial_TxBuffer,byte);
	Panel_Serial_TxBuffer[byte]=CRC16>>8;   //CRC high byte;
	Panel_Serial_TxBuffer[byte+1]=CRC16&0xff; //CRC low byte;

//	HAL_UART_Transmit_IT(&Panel_Tx_Port, Panel_Serial_TxBuffer, byte+2);
    while (__HAL_UART_GET_FLAG (&Panel_Tx_Port, UART_FLAG_TXE) == RESET){}
    while (__HAL_UART_GET_FLAG(&Panel_Tx_Port, UART_FLAG_TC) == RESET){}
    HAL_UART_Transmit_DMA(&Panel_Tx_Port, Panel_Serial_TxBuffer, byte+2);
}

void Panel_Serial_SendEncoder(void)
{
	Panel_Serial_TxBuffer[4]=11;     //msgLength
	Panel_Serial_TxBuffer[5]=1;     //opcode
	//[6]=Packet Sequence
	//[7]=Timestamp high byte
	//[8]=Timestamp low byte
	Panel_Serial_TxBuffer[9]=encoder.Value[0]>>8;   //Packet data
	Panel_Serial_TxBuffer[10]=encoder.Value[0]&0xff;   //Packet data
	Panel_Serial_TxBuffer[11]=encoder.Value[1]>>8;   //Packet data
	Panel_Serial_TxBuffer[12]=encoder.Value[1]&0xff;   //Packet data
	Panel_Serial_TxBuffer[13]=encoder.Value[2]>>8;   //Packet data
	Panel_Serial_TxBuffer[14]=encoder.Value[2]&0xff;   //Packet data
	Panel_Serial_TxBuffer[15]=GPIO_Get_Sensor();   //Sensor IO
	Panel_Serial_TxBuffer[16]=Task_Get_Remain_Repeat_Times();   //Remain repeat times
	Panel_Serial_TxBuffer[17]=BMS_Read_Percent();   //dummy battery
	Panel_Serial_Sendout(18);
}

void Panel_Serial_SendStatus(void)
{
	Panel_Serial_TxBuffer[4]=11;     //msgLength
	Panel_Serial_TxBuffer[5]=7;     //opcode
	//[6]=Packet Sequence
	//[7]=Timestamp high byte
	//[8]=Timestamp low byte
	Panel_Serial_TxBuffer[9]=0;   //System Status
	Panel_Serial_TxBuffer[10]=0;   //Current task ID
	Panel_Serial_TxBuffer[11]=1;   //Current Waypoint
	Panel_Serial_TxBuffer[12]=GPIO_Get_Sensor();   //Sensor IO
	Panel_Serial_TxBuffer[13]=Task_Up_Speed();   //Up Speed
	Panel_Serial_TxBuffer[14]=Task_Down_Speed();   //Down Speed
	Panel_Serial_TxBuffer[15]=BMS_Read_Percent();   //Battery %

	Panel_Serial_Sendout(16);
}
