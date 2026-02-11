// Includes -----------------------------------------------------
#include "main.h"
#include "Lifter_Main.h"
#include "User_Flash.h"
#include "Encoder.h"
#include "User_Uart.h"
#include "Motor.h"
#include "GPIO.h"
#include "Actuator_Control.h"
#include "Lifter_Task.h"
#include "USB_Serial.h"
#include "User_ADC.h"
#include "LED_RS485.h"
#include "Sound_RS485.h"
#include "Modbus.h"
#include "BMS.h"


#define LIFTER_FLASH_ADDR         ((unsigned int)0x0801FC00)	//last page

#define LIFTER_FLASH_SETTING_SIZE	256

#define LIFTER_SUM_ADDR			LIFTER_FLASH_SETTING_SIZE-1

unsigned short Lifter_Setting[LIFTER_FLASH_SETTING_SIZE];


#define MAX_LIFTER_DELAY 90
#define MIN_LIFTER_DELAY 5

#define LIFTER_AUTO_OFF_SEC 3600	//sec

// Private variables --------------------------------------------
extern USB_Serial_TypeDef usb_serial;
uint8_t Lifter_20Hz_Flag, Lifter_10Hz_Flag, Lifter_10Hz_Count, Lifter_1Hz_Flag, Lifter_1Min_Flag, Lifter_1Hz_Count, Lifter_2Hz_Flag, Lifter_2Hz_Count;
uint16_t Lifter_Auto_Off_Timer, Lifter_Delay_Off_Timer, Lifter_1Min_Count;
uint8_t USB_Lifter_Ready,Lifter_Auto_Lock_Foot_Timer,Lifter_Auto_Lock_Foot_Flag;
char msg_buff[100]={0};
bool Lifter_Error_Debug, Lifter_PowerOn_1Sec_Flag;


// Private function prototypes -----------------------------------
void Lifter_20Hz_Handler(void);
void Lifter_1Hz_Handler(void);
void Lifter_2Hz_Handler(void);
void Lifter_1Min_Handler(void);
void Lifter_Key_Handler(void);
void PWM_Init(void);
void PWM_OUTPUT(uint16_t pwm1, uint16_t pwm2, uint16_t pwm3, uint16_t pwm4);
void Lifter_PowerOn_1Sec_Init(void);

// Code ---------------------------------------------------------
void Lifter_Init(void)
{
	printf("Elderly Lifter STM V1.32\n");
	HAL_TIM_Base_Start_IT(&htim2);
	USB_Serial_Init();
	GPIO_Init();
	User_ADC_Init();
	User_UART_Init();
	Motor_Init();
	Actuator_Control_Init();
	Task_Init();
	BMS_Init();
	LED_RS485_Init();
	Sound_RS485_Init();
	USB_Lifter_Ready=0;
	Lifter_Error_Debug=1;
	Lifter_PowerOn_1Sec_Flag=0;
	Lifter_Auto_Off_Update();
}

void Lifter_PowerOn_1Sec_Init(void)
{
	if(Lifter_PowerOn_1Sec_Flag==0)
	{
		Lifter_PowerOn_1Sec_Flag=1;
		Sound_RS485_Volume(VOLUME_DEFAULT);
		HAL_Delay(10);
		if((GPIO_Get_Sensor() & ESTOP_KEY)==0)
			LED_RS485_Color(LED_GREEN_COLOR,10);
		else
			LED_RS485_Color(LED_RED_COLOR,10);
	}
}

void Lifter_20Hz_ISR(void)
{
	Lifter_20Hz_Flag = 1;
	if(++Lifter_1Min_Count >=1200)
	{
		Lifter_1Min_Flag=1;
		Lifter_1Min_Count=0;
	}
	if(++Lifter_1Hz_Count >=20)
	{
		Lifter_1Hz_Flag=1;
		Lifter_1Hz_Count=0;
	}
	if(++Lifter_2Hz_Count >=10)
	{
		Lifter_2Hz_Flag=1;
		Lifter_2Hz_Count=0;
	}
	if(++Lifter_10Hz_Count >=2)
	{
		Lifter_10Hz_Flag=1;
		Lifter_10Hz_Count=0;
	}
}

void Lifter_20Hz_Handler(void)
{
	if(Lifter_20Hz_Flag!=0)
	{
		Lifter_20Hz_Flag=0;
		Encoder_GetValue();
		Scankey_Handler();
		Motor_Position_Limiter();
	}
}

void Lifter_10Hz_Handler(void)
{
	if(Lifter_10Hz_Flag!=0)
	{
		Lifter_10Hz_Flag=0;
		Actuator_Control_Handler();
//		Motor_Timeout_Handler();
		Task_Repeat_Timer_Handler();
		Flash_Handler();
	}
}

void Lifter_1Hz_Handler(void)
{
	if(Lifter_1Hz_Flag!=0)
	{
		Lifter_1Hz_Flag=0;
		Lifter_PowerOn_1Sec_Init();
		Lifter_Auto_Off_Handler();
		Lifter_Auto_Lock_Foot_Handler();
//		Encoder_Show_Value();
//		BMS_Show_Value();
		if(USB_Lifter_Ready!=0)
		{
		}

		if(BMS_Read_Voltage()==0)	//for make sure get battery after startup
		{
			Encoder_Set_Pause(5);	//pause get encoder for 0.5 second
			HAL_Delay(100);
			BMS_GetValue();
		}
	}
}

void Lifter_2Hz_Handler(void)
{
	if(Lifter_2Hz_Flag!=0)
	{
		Lifter_2Hz_Flag=0;
		GPIO_Get_Sensor();
//		Show_Actuator_Speed();
//		Show_ADC();
	}

}

void Lifter_1Min_Handler(void)
{
	if(Lifter_1Min_Flag!=0)
	{
		Lifter_1Min_Flag=0;
		if(ActuatorSystem_GetState()!=SYSTEM_MOVING)
		{
			Encoder_Set_Pause(5);	//pause get encoder for 0.5 second
			HAL_Delay(100);
			BMS_GetValue();
		}
	}
}

void Lifter_Main(void)
{
	Read_Lifter_Setting();
	Encoder_Set_Pause(20);	//pause get encoder for 1 second
	HAL_Delay(100);
	BMS_GetValue();
	while(1)
	{
		USB_Serial_Handler();
		User_UART_Rx_Handler();
		Lifter_20Hz_Handler();
		Lifter_1Hz_Handler();
		Lifter_2Hz_Handler();
		Lifter_10Hz_Handler();
		Lifter_1Min_Handler();
		Lifter_Key_Handler();
		Task_Execute_Handler();
	}
}

void Lifter_Key_Handler(void)
{
uint8_t key;
	key = Scankey_GetKey();
	if(key!=0)
	{
		Lifter_Auto_Off_Update();
		switch(key)
		{
			case UP_KEY|KEY_PRESS:
				printf("up press\n");
//				if((GPIO_Get_Sensor()&(IR_BOTTOM|IR_RIGHT|IR_LEFT))==(IR_RIGHT|IR_LEFT))
				if((GPIO_Get_Sensor()&(ESTOP_KEY|IR_RIGHT|IR_LEFT))==(IR_RIGHT|IR_LEFT))
				{
					GPIO_Set_Key_Led(KEY_LED_ON);
					Task_Move_Up();
				}
				else
					GPIO_Set_Key_Led(KEY_LED_5HZ);
				break;
			case UP_KEY|KEY_RELEASE:
				printf("up release\n");
				GPIO_Set_Key_Led(KEY_LED_OFF);
				Task_Move_Stop();
				break;
			case DOWN_KEY|KEY_PRESS:
				printf("down press\n");
//				if((GPIO_Get_Sensor()&(IR_BOTTOM|IR_RIGHT|IR_LEFT))==(IR_RIGHT|IR_LEFT))
				if((GPIO_Get_Sensor()&(ESTOP_KEY|IR_RIGHT|IR_LEFT))==(IR_RIGHT|IR_LEFT))
				{
					GPIO_Set_Key_Led(KEY_LED_ON);
					Task_Move_Down();
				}
				else
					GPIO_Set_Key_Led(KEY_LED_5HZ);
				break;
			case DOWN_KEY|KEY_RELEASE:
				printf("down release\n");
				GPIO_Set_Key_Led(KEY_LED_OFF);
				Task_Move_Stop();
				break;
			case ESTOP_KEY|KEY_PRESS:
				printf("estop press\n");
				Sound_RS485_Play(SOUND_E_STOP);
				Task_Move_Stop();
				LED_RS485_Color(LED_RED_COLOR,10);
				break;
			case ESTOP_KEY|KEY_RELEASE:
				printf("estop release\n");
				LED_RS485_Color(LED_GREEN_COLOR,10);
				break;
			case RUN_KEY|KEY_PRESS:
				if((GPIO_Get_Sensor()&(ESTOP_KEY|IR_RIGHT|IR_LEFT))==(IR_RIGHT|IR_LEFT))
				{
					printf("run press\n");
					Task_Repeat();
				}
				break;
			case RUN_KEY|KEY_RELEASE:
				printf("run release\n");
				Task_Repeat_Pause();
				break;
			case SOS_KEY|KEY_PRESS:
				printf("sos press\n");
				Task_SOS();
//				Task_Move_Home();	//test
				break;
			case SOS_KEY|KEY_RELEASE:
				printf("sos release\n");
//				Task_Move_Stop();	//test
				break;
			default:
				break;
		}
	}
}

void Lifter_Auto_Off_Handler(void)
{
	if(Lifter_Auto_Off_Timer>0)
	{
		if(--Lifter_Auto_Off_Timer==0)
		{
			printf("Lifter auto power off, %d sec\n",LIFTER_AUTO_OFF_SEC);
			HAL_Delay(10);
			GPIO_Trigger_Power_Switch();
		}
	}
	if(Lifter_Delay_Off_Timer>0)
	{
		if(--Lifter_Delay_Off_Timer==0)
		{
			printf("Lifter power off\n");
			HAL_Delay(10);
			GPIO_Trigger_Power_Switch();
		}
	}
}

void Lifter_Auto_Off_Update(void)
{
	Lifter_Auto_Off_Timer=LIFTER_AUTO_OFF_SEC;
}

void Lifter_Set_Delay_Off(uint16_t delay)
{
	Lifter_Delay_Off_Timer = delay;
}

void Write_Lifter_Setting(void)
{
uint16_t sum;
	sum = 0xFFFF;
	Lifter_Setting[0]= encoder.Zero_Offset[0];
	Lifter_Setting[1]= encoder.Zero_Offset[1];
	Lifter_Setting[2]= encoder.Zero_Offset[2];

	sum = Modbus_CalCRC_16bit(Lifter_Setting,LIFTER_FLASH_SETTING_SIZE-1);
	Lifter_Setting[LIFTER_SUM_ADDR]= sum;

	Internal_WriteFlash(LIFTER_FLASH_ADDR,Lifter_Setting,LIFTER_FLASH_SETTING_SIZE);

}

void Lifter_Auto_Lock_Foot_Handler(void)	//1Hz timebase
{
	if((GPIO_Get_Sensor()&(IR_BOTTOM))==0)
	{
		if(Lifter_Auto_Lock_Foot_Flag==0)
		{
			Lifter_Auto_Lock_Foot_Flag = 1;
			Motor_Set_Output(LATCH_MOTOR, -100);	//Lock foot
			Lifter_Auto_Lock_Foot_Timer = 5;	//5sec
			printf("locking foot\n");
		}
	}
	else
		Lifter_Auto_Lock_Foot_Flag=0;
	if(Lifter_Auto_Lock_Foot_Timer!=0)
	{
		if(--Lifter_Auto_Lock_Foot_Timer==0)
		{
			Motor_Set_Output(LATCH_MOTOR, 0);	//turn off motor
			printf("lock foot finish\n");
		}

	}

}

unsigned char Read_Lifter_Setting(void)
{
uint16_t i;
uint16_t sum;
	sum = 0xFFFF;
	Internal_ReadFlash(LIFTER_FLASH_ADDR,Lifter_Setting,LIFTER_FLASH_SETTING_SIZE*2);
	sum = Modbus_CalCRC_16bit(Lifter_Setting,LIFTER_FLASH_SETTING_SIZE-1);

	if(Lifter_Setting[LIFTER_SUM_ADDR]==sum)	//checksum match
	{
		for(i=0;i<3;i++)
			encoder.Zero_Offset[i] = Lifter_Setting[i];
		Encoder_Show_Offset();
		return 1;
	}
	return 0;
}


