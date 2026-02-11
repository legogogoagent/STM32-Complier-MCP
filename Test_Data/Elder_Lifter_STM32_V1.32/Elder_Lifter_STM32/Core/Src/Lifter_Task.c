// Includes ------------------------------------------------------------------
#include "main.h"
#include "Lifter_Main.h"
#include "Lifter_Task.h"
#include "Modbus.h"
#include "Encoder.h"
#include "Motor.h"
#include "Actuator_Control.h"
#include "Sound_RS485.h"
#include "LED_RS485.h"
#include "GPIO.h"

extern UART_HandleTypeDef huart2;

#define REPEAT_WAIT 15	//10Hz time base
#define SOS_WAIT 25		//10Hz time base
#define REPEAT_MAX_TIMES 50	//
#define STAND_MAX_TIME 120	//

// Variables ---------------------------------------------------------
Task_TypeDef task;

bool Task_Serial_debug;

// Subroutine -----------------------------------------------------
void Task_Clear_Repeat(void);

// Code -----------------------------------------------------------
void Task_Init(void)
{
	Task_Serial_debug=1;
	task.state = RESET_STATE;
	task.up_speed = 50;	//default 50%
	task.down_speed = 50;	//default 50%
	task.last_waypoint = 0;
	task.last_action = RESET_STATE;
	task.volume = VOLUME_DEFAULT;
	Task_Load_Job();
	Task_Clear_Repeat();
	printf("Lifter_Task_Init\n");

}

void Task_Debug(bool enable)
{
	Task_Serial_debug=enable;
}

void Task_Clear_Waypoint(void)
{
	task.buffer[0] = 0;
	task.total_waypoint = 0;
	task.pointer = 0;
	task.last_action = RESET_STATE;
	Task_Move_Stop();
}

void Task_Push_Waypoint(uint8_t speed, uint16_t vertical, uint16_t horizontal, uint16_t tilt)
{
uint16_t i;
	if(task.buffer[0]<MAX_WAYPOINT)
	{
		task.buffer[0]++;
		i= task.buffer[0]*WAYPOINT_SIZE;	//number of waypoints
		task.buffer[i+1]= speed;			//speed ratio
		task.buffer[i+2]= vertical>>8;		//Vertical Encoder value high byte
		task.buffer[i+3]= vertical & 0xff;	//Vertical Encoder value low byte
		task.buffer[i+4]= horizontal>>8;	//Horizontal Encoder value high byte
		task.buffer[i+5]= horizontal & 0xff;//Horizontal Encoder value low byte
		task.buffer[i+6]= tilt>>8;			//Tilt Encoder value high byte
		task.buffer[i+7]= tilt & 0xff;		//Tilt Encoder value low byte

		task.buffer[i+8]= 0xAA;		//End
		task.buffer[i+9]= 1;		//
		task.total_waypoint = task.buffer[0];
	}
	else
	{
		if((Lifter_Error_Debug!=0) || (Task_Serial_debug!=0))
			printf("Task_Push_Waypoint buffer full\n");
	}
}

/********************************************************************
 * void Task_Load_Waypoint(void)
 * @brief  Load waypoint from buffer
 *
 *******************************************************************/
Waypoint_TypeDef Task_Load_Waypoint(uint8_t pointer)
{
Waypoint_TypeDef wp;
	if(pointer > task.total_waypoint)
	{
//		task.state = FINISH_STATE;
		wp.id = 0;
	}
	else
	{
		wp.id = pointer;
		wp.target_speed = task.buffer[pointer*WAYPOINT_SIZE+1];
		wp.target_V = ((uint16_t)task.buffer[pointer*WAYPOINT_SIZE+2]<<8) + task.buffer[pointer*WAYPOINT_SIZE+3];
		wp.target_H = ((uint16_t)task.buffer[pointer*WAYPOINT_SIZE+4]<<8) + task.buffer[pointer*WAYPOINT_SIZE+5];
		wp.target_T = ((uint16_t)task.buffer[pointer*WAYPOINT_SIZE+6]<<8) + task.buffer[pointer*WAYPOINT_SIZE+7];
	}
	return wp;
}


/********************************************************************
 * void Task_Search_Waypoint(void)
 * @brief  search closest waypoint from buffer
 *
 *******************************************************************/
Waypoint_TypeDef Task_Search_Waypoint(uint16_t vertical_position, uint8_t dir)
{
uint8_t i;
uint16_t target;
	for(i=1; i<=task.total_waypoint; i++)
	{
		target = ((uint16_t)task.buffer[i*WAYPOINT_SIZE+2]<<8) + task.buffer[i*WAYPOINT_SIZE+3];
		if(target > vertical_position)
			break;
	}
	if(i>task.total_waypoint)
		i=task.total_waypoint;

	if(dir==0)	//down direction
	{
		if(i>1)
			i--;
		else
			i=1;
	}
	return Task_Load_Waypoint(i);
}


/********************************************************************
 * void Task_Load_Job(void)
 * @brief  Load task from flash
 *
 *******************************************************************/
void Task_Load_Job(void)
{
	Task_Push_Waypoint(50, 0, 0, 500);	//home
	Task_Push_Waypoint(100, 0, 0, 200);
	Task_Push_Waypoint(100, 15000, 0, 290);
	task.total_waypoint = task.buffer[0];
	if(task.total_waypoint > MAX_WAYPOINT)
		task.total_waypoint = MAX_WAYPOINT;
}

/********************************************************************
 * void Task_Execute_Handler(void)
 * @brief  Execute task buffer waypoint one by one
 * 			should call this function in every main loop
 *
 *******************************************************************/
uint8_t Task_Execute_Handler(void)
{
Waypoint_TypeDef target;

	switch(task.state)
	{
		case RESET_STATE:
			task.state = SEARCH_WAYPOINT_STATE;
			break;
		case SEARCH_WAYPOINT_STATE:
			if(Encoder_Ready()!=0)
			{
				task.pointer = Task_Search_Waypoint(Encoder_Read(VER_MOTOR),0).id;
				task.state = IDLE_STATE;
			}
			break;
		case MOVE_UP_STATE:
			target = Task_Load_Waypoint(task.pointer);
			if(target.id!=0)
			{
				printf("move up to waypoint:%d\n",task.pointer);
				ActuatorSystem_SetTarget(target.target_V, target.target_H, target.target_T);
				task.state = UP_RUNNING_STATE;
				task.last_waypoint = task.pointer;
			}
			else
			{
				if((Lifter_Error_Debug!=0) || (Task_Serial_debug!=0))
					printf("Not move up waypoint found\n");
				task.state = IDLE_STATE;
				GPIO_Set_Key_Led(KEY_LED_2HZ);
			}
			break;
		case MOVE_DOWN_STATE:
			target = Task_Load_Waypoint(task.pointer);
			if(target.id!=0)
			{
				printf("move down to waypoint:%d\n",task.pointer);
				ActuatorSystem_SetTarget(target.target_V, target.target_H, target.target_T);
				task.state = DOWN_RUNNING_STATE;
				task.last_waypoint = task.pointer;
			}
			else
			{
				if((Lifter_Error_Debug!=0) || (Task_Serial_debug!=0))
					printf("Not move down waypoint found\n");
				task.state = IDLE_STATE;
				GPIO_Set_Key_Led(KEY_LED_2HZ);
			}
			break;
		case UP_RUNNING_STATE:
			if((Actuator_Get_State() == SYSTEM_FINISH) || (Actuator_Reach_Look_Ahead_Distance()!=0))
			{
				if(task.pointer < task.total_waypoint)
				{
					if((Lifter_Error_Debug!=0) || (Task_Serial_debug!=0))
						printf("move up waypoint%d reach\n",task.pointer);
					task.pointer++;
					task.state = MOVE_UP_STATE;
				}
				else
				{
					task.state = IDLE_STATE;
					if((Lifter_Error_Debug!=0) || (Task_Serial_debug!=0))
						printf("move up finish\n");
					GPIO_Set_Key_Led(KEY_LED_2HZ);
				}
			}
			break;
		case DOWN_RUNNING_STATE:
			if((Actuator_Get_State() == SYSTEM_FINISH) || (Actuator_Reach_Look_Ahead_Distance()!=0))
			{
				if(task.pointer!=0)
				{
					if((Lifter_Error_Debug!=0) || (Task_Serial_debug!=0))
						printf("move down waypoint%d reach\n",task.pointer);
					task.pointer--;
					task.state = MOVE_DOWN_STATE;
				}
				else
				{
					task.state = IDLE_STATE;
					if((Lifter_Error_Debug!=0) || (Task_Serial_debug!=0))
						printf("move down finish\n");
					GPIO_Set_Key_Led(KEY_LED_2HZ);
				}
			}
			break;
		case REPEAT_RUN_STATE:
			if((Lifter_Error_Debug!=0) || (Task_Serial_debug!=0))
				printf("repeat run start\n");
			//check is at home position or not
			//play sound "back to starting point"
			Sound_RS485_Play(SOUND_START_POINT);
			task.repeat_wait_timer = REPEAT_WAIT;
			task.repeat_flag = 1;
			task.state = REPEAT_HOME_STATE;
			break;

		case REPEAT_HOME_STATE:
			if(task.repeat_wait_timer==0)
			{
				if((Lifter_Error_Debug!=0) || (Task_Serial_debug!=0))
					printf("repeat back to home position\n");
				task.pointer = 1;
				Actuator_Set_Total_Speed((float)(task.down_speed)/100);
				Actuator_Set_Soft_Start();

				target = Task_Load_Waypoint(task.pointer);
				if(target.id!=0)
				{
					printf("move down to home:%d\n",task.pointer);
					ActuatorSystem_SetTarget(target.target_V, target.target_H, target.target_T);
					task.last_waypoint = task.pointer;
					task.last_action = task.state;
					task.state = REPEAT_HOME_RUNNING_STATE;
				}
			}
			break;
		case REPEAT_HOME_RUNNING_STATE:
			if(Actuator_Reach_Look_Ahead_Distance()!=0)
			{
				task.last_action = task.state;
				task.state = REPEAT_HOME_FINISH_STATE;
				Sound_RS485_Play(SOUND_I_AM_READY);
				if((Lifter_Error_Debug!=0) || (Task_Serial_debug!=0))
					printf("reach home position of repeat mode\n");
			}
			break;

		case REPEAT_UP_STATE:
			if(task.repeat_wait_timer==0)
			{
				if((Lifter_Error_Debug!=0) || (Task_Serial_debug!=0))
					printf("repeat move up\n");

				target = Task_Load_Waypoint(task.pointer);
				if(target.id!=0)
				{
					printf("repeat move up to waypoint:%d\n",task.pointer);
//					ActuatorSystem_SetTarget(target.target_V, target.target_H, target.target_T);
					ActuatorSystem_SetTarget_withoutInit(target.target_V, target.target_H, target.target_T);
					task.last_waypoint = task.pointer;
					task.last_action = task.state;
					task.state = REPEAT_UP_RUNNING_STATE;
				}
				else
				{
					if((Lifter_Error_Debug!=0) || (Task_Serial_debug!=0))
						printf("repeat mode no move up waypoint found\n");
					task.state = REPEAT_UP_FINISH_STATE;
				}
			}
			break;
		case REPEAT_UP_RUNNING_STATE:
			if((Actuator_Get_State() == SYSTEM_FINISH) || (Actuator_Reach_Look_Ahead_Distance()!=0))
			{
				if(task.pointer < task.total_waypoint)
				{
					if((Lifter_Error_Debug!=0) || (Task_Serial_debug!=0))
						printf("repeat move up waypoint%d reach\n",task.pointer);
					task.pointer++;
					task.state = REPEAT_UP_STATE;
				}
				else
					task.state = REPEAT_UP_FINISH_STATE;

			}
			break;

		case REPEAT_UP_FINISH_STATE:
			if(task.last_action != REPEAT_UP_FINISH_STATE)
			{
				task.last_action = REPEAT_UP_FINISH_STATE;
//				Sound_RS485_Play(SOUND_UP_FINISH);
				if((Lifter_Error_Debug!=0) || (Task_Serial_debug!=0))
					printf("repeat move up finish\n");
			}
			task.state = REPEAT_STAND_STATE;
			break;

		case REPEAT_DOWN_STATE:
			if(task.repeat_wait_timer==0)
			{
				if((Lifter_Error_Debug!=0) || (Task_Serial_debug!=0))
					printf("repeat move down\n");

				target = Task_Load_Waypoint(task.pointer);
				if(target.id!=0)
				{
					printf("repeat move down to waypoint:%d\n",task.pointer);
					ActuatorSystem_SetTarget(target.target_V, target.target_H, target.target_T);
					task.last_waypoint = task.pointer;
					task.last_action = task.state;
					task.state = REPEAT_DOWN_RUNNING_STATE;
				}
				else
				{
					if((Lifter_Error_Debug!=0) || (Task_Serial_debug!=0))
						printf("repeat mode no move down waypoint found\n");
					task.state = REPEAT_DOWN_FINISH_STATE;
				}
			}
			break;
		case REPEAT_DOWN_RUNNING_STATE:
			if((Actuator_Get_State() == SYSTEM_FINISH) || (Actuator_Reach_Look_Ahead_Distance()!=0))
			{
				if(task.pointer!=0)
				{
					if((Lifter_Error_Debug!=0) || (Task_Serial_debug!=0))
						printf("repeat move down waypoint%d reach\n",task.pointer);
					task.pointer--;
					task.state = REPEAT_DOWN_STATE;
				}
				else
					task.state = REPEAT_DOWN_FINISH_STATE;

			}
			break;

		case REPEAT_DOWN_FINISH_STATE:
			if(task.last_action != REPEAT_DOWN_FINISH_STATE)
			{
				task.last_action = REPEAT_DOWN_FINISH_STATE;
				if(--task.repeat_remain_times==0)
				{
					task.state = REPEAT_FINISH_STATE;
					Sound_RS485_Play(SOUND_TRAINING_COMPLETE);
					task.repeat_wait_timer = REPEAT_WAIT;
				}
				else
				{
					Sound_RS485_Play(SOUND_END_POINT);
					if((Lifter_Error_Debug!=0) || (Task_Serial_debug!=0))
						printf("repeat move down finish, remain %d times\n",task.repeat_remain_times);
				}
			}
			break;

		case REPEAT_STAND_STATE:
			task.state = REPEAT_STAND_WAITING_STATE;
			Sound_RS485_Play(SOUND_UP_FINISH);
			task.repeat_wait_timer = task.repeat_stand_time*10;
			if((Lifter_Error_Debug!=0) || (Task_Serial_debug!=0))
				printf("repeat stand up %d sec\n",task.repeat_stand_time);
			break;

		case REPEAT_STAND_WAITING_STATE:
			if(task.repeat_wait_timer==0)	//wait for stand time
			{
				task.state = REPEAT_STAND_FINISH_STATE;
				Sound_RS485_Play(SOUND_STAND_FINISH);
				if((Lifter_Error_Debug!=0) || (Task_Serial_debug!=0))
					printf("repeat stand finish\n");
			}
			break;

		case REPEAT_STAND_FINISH_STATE:
			break;

		case REPEAT_FINISH_STATE:
			if(task.repeat_wait_timer==0)	//wait for sound playing
			{
				if((Lifter_Error_Debug!=0) || (Task_Serial_debug!=0))
					printf("repeat finish\n");
				Task_Clear_Repeat();
				Task_Move_Stop();
			}
			break;

		case MOVE_HOME_STATE:
			if(task.repeat_wait_timer==0)
			{
				if((Lifter_Error_Debug!=0) || (Task_Serial_debug!=0))
					printf("running to home position\n");
				Actuator_Set_Total_Speed(0.3);	//half speed
				Actuator_Set_Soft_Start();
				ActuatorSystem_SetTarget(0, 0, 500);	//home position
				task.state = MOVE_HOME_RUNNING_STATE;
			}

			break;
		case MOVE_HOME_RUNNING_STATE:
			if(Actuator_Get_State() == SYSTEM_FINISH)
			{
				if((Lifter_Error_Debug!=0) || (Task_Serial_debug!=0))
					printf("move to home position reach\n");
				task.state = IDLE_STATE;
				Sound_RS485_Play(SOUND_END_POINT);
			}
			break;

		default:
			break;
	}
	return 0;
}

void Task_Move_Up(void)
{
	if(task.state == IDLE_STATE)
	{
		printf("last waypoint:%d\n",task.last_waypoint);
		if(task.last_action == MOVE_UP_STATE)
			task.pointer = task.last_waypoint;
		else if(task.last_action == MOVE_DOWN_STATE)
			task.pointer = task.last_waypoint+1;
		else
			task.pointer = Task_Search_Waypoint(Encoder_Read(VER_MOTOR),1).id;	//up search

		if(task.pointer > task.total_waypoint)
			task.pointer = task.total_waypoint;
		Actuator_Set_Total_Speed((float)(task.up_speed)/100);
		Actuator_Set_Soft_Start();
		task.state = MOVE_UP_STATE;
		task.last_action = MOVE_UP_STATE;
	}
}

void Task_Move_Down(void)
{
	if(task.state == IDLE_STATE)
	{
		printf("last waypoint:%d\n",task.last_waypoint);
		if(task.last_action == MOVE_DOWN_STATE)
			task.pointer = task.last_waypoint;
		else if(task.last_action == MOVE_UP_STATE)
			task.pointer = task.last_waypoint-1;
		else
			task.pointer = Task_Search_Waypoint(Encoder_Read(VER_MOTOR),0).id;		//down search
		if(task.pointer < 1)
			task.pointer = 1;
		Actuator_Set_Total_Speed((float)(task.down_speed)/100);
		Actuator_Set_Soft_Start();
		task.state = MOVE_DOWN_STATE;
		task.last_action = MOVE_DOWN_STATE;
	}
}

void Task_Move_Home(void)
{
	if(task.state == IDLE_STATE)
	{
		if((Lifter_Error_Debug!=0) || (Task_Serial_debug!=0))
			printf("move to home position\n");
		Sound_RS485_Play(SOUND_HOME_POINT);
		task.repeat_wait_timer = REPEAT_WAIT;
		task.state = MOVE_HOME_STATE;
	}
}

void Task_Move_Stop(void)
{
	ActuatorSystem_Stop();
	task.state = IDLE_STATE;
}

void Task_Set_Speed(uint8_t up_speed, uint8_t down_speed)
{
	if(up_speed>100)
		up_speed = 100;
	if(down_speed>100)
		down_speed = 100;
	task.up_speed = up_speed;
	task.down_speed = down_speed;
}

uint8_t Task_Up_Speed(void)
{
	return task.up_speed;
}

uint8_t Task_Down_Speed(void)
{
	return task.down_speed;
}

void Task_Clear_Repeat(void)
{
	task.repeat_flag = 0;
	task.repeat_remain_times = 0;
	task.repeat_wait_timer = 0;
}

void Task_Set_Repeat(uint8_t repeat)
{
	if(repeat > REPEAT_MAX_TIMES)
		repeat = 0;
	Task_Move_Stop();
	task.repeat_flag=0;
	task.repeat_remain_times = repeat;

}

void Task_Set_StandTime(uint8_t time)
{
	if(time > STAND_MAX_TIME)
		time = 0;
	task.repeat_stand_time = time;

}

void Task_Set_Volume(uint8_t vol)
{
	if(vol > 100)
		vol = 50;	//back to default
	task.volume = vol;
	Sound_RS485_Volume(task.volume);
}


void Task_Repeat(void)
{
	switch(task.state)
	{
		case IDLE_STATE:
			if((task.repeat_flag==0)&&(task.repeat_remain_times!=0))
			{
				task.last_action = task.state;
				task.state = REPEAT_RUN_STATE;

			}
			break;
		case REPEAT_HOME_PAUSE_STATE:
			task.last_action = task.state;
			task.state = REPEAT_HOME_STATE;
			break;
		case REPEAT_HOME_FINISH_STATE:
			Sound_RS485_Play(SOUND_UP);
			Actuator_Set_Total_Speed((float)(task.up_speed)/100);
			Actuator_Set_Soft_Start();
			task.repeat_wait_timer = REPEAT_WAIT;
			task.last_action = task.state;
			task.state = REPEAT_UP_STATE;
			break;

		case REPEAT_UP_PAUSE_STATE:
			task.last_action = task.state;
			task.state = REPEAT_UP_STATE;
			break;

		case REPEAT_STAND_FINISH_STATE:	//REPEAT_UP_FINISH_STATE:
			Actuator_Set_Total_Speed((float)(task.down_speed)/100);
			Actuator_Set_Soft_Start();
			task.repeat_wait_timer = REPEAT_WAIT;
			task.last_action = task.state;
			task.state = REPEAT_DOWN_STATE;
			Sound_RS485_Play(SOUND_DOWN);
			break;

		case REPEAT_DOWN_PAUSE_STATE:
			task.last_action = task.state;
			task.state = REPEAT_DOWN_STATE;
			break;

		case REPEAT_DOWN_FINISH_STATE:
			Actuator_Set_Total_Speed((float)(task.up_speed)/100);
			Actuator_Set_Soft_Start();
			task.pointer = 1;
			task.repeat_wait_timer = REPEAT_WAIT;
			task.last_action = task.state;
			task.state = REPEAT_UP_STATE;
			Sound_RS485_Play(SOUND_UP);
			break;
	}
}

void Task_Repeat_Pause(void)
{
	ActuatorSystem_Stop();
	task.last_action = task.state;
	if((task.state==REPEAT_HOME_RUNNING_STATE)||(task.state==REPEAT_HOME_STATE))
		task.state = REPEAT_HOME_PAUSE_STATE;
	if((task.state==REPEAT_UP_RUNNING_STATE)||(task.state==REPEAT_UP_STATE))
		task.state = REPEAT_UP_PAUSE_STATE;
	if((task.state==REPEAT_DOWN_RUNNING_STATE)||(task.state==REPEAT_DOWN_STATE))
		task.state = REPEAT_DOWN_PAUSE_STATE;

}

void Task_SOS(void)
{
	ActuatorSystem_Stop();
	if(task.repeat_wait_timer == 0)		//avoid interrupt previous sound
	{
		task.repeat_wait_timer = SOS_WAIT;
		Sound_RS485_Volume(VOLUME_SOS);
		Sound_RS485_Play(SOUND_HELP);
		LED_RS485_Color(LED_RED_COLOR,10);
	}
}

void Task_Repeat_Timer_Handler(void)
{
	if(task.repeat_wait_timer!=0)
		task.repeat_wait_timer--;
	if(task.repeat_wait_timer==1)	//need to remove later
		Sound_RS485_Volume(task.volume);

}

uint8_t Task_Get_Remain_Repeat_Times(void)
{
	return task.repeat_remain_times;
}
