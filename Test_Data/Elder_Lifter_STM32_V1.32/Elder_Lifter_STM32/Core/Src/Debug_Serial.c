// Includes ------------------------------------------------------------------
#include "main.h"
#include "Lifter_Main.h"
#include "stdarg.h"
#include "Encoder.h"
#include "Motor.h"
#include "User_Uart.h"
#include "Actuator_Control.h"
#include "Debug_Serial.h"
#include "Lifter_Task.h"
#include "GPIO.h"
#include "Sound_RS485.h"
#include "LED_RS485.h"

extern UART_HandleTypeDef huart1;

// Definition ---------------------------------------------------------
#define Debug_Tx huart1		//115200 8N1

// Variables ---------------------------------------------------------
uint8_t Debug_Enable;

// Subroutine -----------------------------------------------------
unsigned char Extract_Keyword(const char *keyword, float*data, uint8_t *rx_data);


// Code -----------------------------------------------------------
void Debug_Init(void)
{
	printf("Debug_Serial_Init\n");
	Debug_Enable=0;

}

/********************************************************************
 * void Debug_Serial_Handler(void)
 * @brief  Check and process serial packet from serial port,
 * 			should call this function in every main loop
 *
 *******************************************************************/
uint16_t Debug_Serial_Handler(uint8_t* rx_data, uint16_t len)
{
float temp_float;
	if(Extract_Keyword("debug:",&temp_float, rx_data))
	{
		if(temp_float==1)
			Debug_Enable=1;
		else
			Debug_Enable=0;
		printf("set debug= %d \n",Debug_Enable);
		return len;	//throw all buffer when match
	}

	if(Debug_Enable!=0)
	{
		if(Extract_Keyword("encoder:",&temp_float, rx_data))
		{
			printf("get encoder \n");
			Encoder_Show_Value();
			return len;	//throw all buffer when match
		}

		if(Extract_Keyword("til:",&temp_float, rx_data))
		{
			printf("set tile motor %d \n",(int8_t)temp_float);
			Motor_Set_Output(TILT_MOTOR, (int8_t)temp_float);
			return len;	//throw all buffer when match
		}
		if(Extract_Keyword("ver:",&temp_float, rx_data))
		{
			printf("set vertical motor %d \n",(int8_t)temp_float);
			Motor_Set_Output(VER_MOTOR, (int8_t)temp_float);
			return len;	//throw all buffer when match
		}
		if(Extract_Keyword("hor:",&temp_float, rx_data))
		{
			printf("set horizontal motor %d \n",(int8_t)temp_float);
			Motor_Set_Output(HOR_MOTOR, (int8_t)temp_float);
			return len;	//throw all buffer when match
		}
		if(Extract_Keyword("lat:",&temp_float, rx_data))
		{
			printf("set latch motor %d \n",(int8_t)temp_float);
			Motor_Set_Output(LATCH_MOTOR, (int8_t)temp_float);
			return len;	//throw all buffer when match
		}
		if(Extract_Keyword("led:",&temp_float, rx_data))
		{
			printf("set led color %X \n",(uint32_t)temp_float);
			LED_RS485_Color((uint32_t)temp_float,10);
			return len;	//throw all buffer when match
		}
		if(Extract_Keyword("sound:",&temp_float, rx_data))
		{
			printf("play sound %d \n",(uint8_t)temp_float);
			Sound_RS485_Play((uint8_t)temp_float);
			return len;	//throw all buffer when match
		}

		if(Extract_Keyword("volume:",&temp_float, rx_data))
		{
			printf("set volume %d \n",(uint8_t)temp_float);
			Sound_RS485_Volume((uint8_t)temp_float);
			return len;	//throw all buffer when match
		}

		if(Extract_Keyword("calibrate:",&temp_float, rx_data))
		{
			printf("encoder calibrate zero %d \n",(uint8_t)temp_float);
			Encoder_Write_Calibrate();
			return len;	//throw all buffer when match
		}
		if(Extract_Keyword("repeat:",&temp_float, rx_data))
		{
			printf("set repeat times %d \n",(uint8_t)temp_float);
			Task_Set_Repeat((uint8_t)temp_float);
			return len;	//throw all buffer when match
		}
		if(Extract_Keyword("pos_limit:",&temp_float, rx_data))
		{
			printf("set position limiter %d \n",(uint8_t)temp_float);
			Motor_Set_Position_Limiter((uint8_t)temp_float);
			return len;	//throw all buffer when match
		}
		if(Extract_Keyword("actuator_debug:",&temp_float, rx_data))
		{
			printf("set actuator_debug %d \n",(uint8_t)temp_float);
			Actuator_Set_Debug((uint8_t)temp_float);
			return len;	//throw all buffer when match
		}

	//	if(Extract_Keyword("buz:",&temp_float, rx_data))
	//	{
	//		printf("set buzzer %d \n",(int8_t)temp_float);
	//		GPIO_Set_Buzzer((uint8_t)temp_float);
	//		return len;	//throw all buffer when match
	//	}
	//	if(Extract_Keyword("pwr:",&temp_float, rx_data))
	//	{
	//		printf("trigger power switch %d \n",(int8_t)temp_float);
	//		GPIO_Trigger_Power_Switch();
	//		return len;	//throw all buffer when match
	//	}
	//
	//	if(Extract_Keyword("vkp:",&temp_float, rx_data))
	//	{
	//		printf("set vertical kp %.1f \n",temp_float);
	//		Set_vkp(temp_float);
	//	}
	//
	//	if(Extract_Keyword("vki:",&temp_float, rx_data))
	//	{
	//		printf("set vertical ki %.1f \n",temp_float);
	//		Set_vki(temp_float);
	//	}
	//
	//	if(Extract_Keyword("vkd:",&temp_float, rx_data))
	//	{
	//		printf("set vertical kd %.1f \n",temp_float);
	//		Set_vkd(temp_float);
	//	}
	//	if(Extract_Keyword("pid:",&temp_float, rx_data))
	//	{
	//		Show_PID();
	//	}
	//	if(Extract_Keyword("job:",&temp_float, rx_data))
	//	{
	//		Task_Load_Job();
	//	}
	}
	return 0;
}

unsigned char Extract_Keyword(const char *keyword, float*data, uint8_t *rx_data)
{
uint8_t i;
size_t len = strlen(keyword);
float temp_float;
	for(i=0; i<len; i++)
	{
		if(rx_data[i]!=keyword[i])
			break;
	}
	if(i>=len)	//all header matched
	{
		temp_float=atof((char*)&rx_data[i]);
		if((temp_float==0)&&((rx_data[i]!='0')))	//handle atof return 0 when convert fail.
			return 0;
		//temp_float=round(temp_float * 1e5) / 1e5;	//limit 5 decimal places
		*data=temp_float;
		return 1;
	}
	return 0;
}

int __io_putchar(int ch) {
	//Normal Mode
	HAL_UART_Transmit(&Debug_Tx, (uint8_t*)&ch, 1, 0xffff);
    return ch;
}

int _write(int file, char *ptr, int len)
{
	int DataIdx;
	for (DataIdx = 0; DataIdx< len; DataIdx++)
	{
		__io_putchar(*ptr++);
	}
	return len;
}

