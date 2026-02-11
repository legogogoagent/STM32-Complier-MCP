// Includes ------------------------------------------------------------------
#include "main.h"
#include "Lifter_Main.h"
#include "Motor.h"
#include "Encoder.h"

// Definition ---------------------------------------------------------


#define PWM1_PORT	GPIOA
#define PWM1_D1_PIN	GPIO_PIN_5
#define PWM1_D2_PIN	GPIO_PIN_4

#define PWM2_PORT	GPIOB
#define PWM2_D1_PIN	GPIO_PIN_13
#define PWM2_D2_PIN	GPIO_PIN_12

#define PWM3_PORT	GPIOC
#define PWM3_D1_PIN	GPIO_PIN_13
#define PWM3_D2_PIN	GPIO_PIN_13
#define PWM3_DIR_PIN	GPIO_PIN_13

#define PWM4_PORT	GPIOC
#define PWM4_D1_PIN	GPIO_PIN_14
#define PWM4_D2_PIN	GPIO_PIN_14
#define PWM4_DIR_PIN	GPIO_PIN_14

#define MAX_PWM_DUTY 95
#define MIN_PWM_DUTY 20	//35	//20

#define MOTOR_TIMEOUT_MS	1000

// Variables ---------------------------------------------------------

bool Motor_debug;
uint32_t Motor_timer;
uint8_t Motor_Position_Limiter_Enable;

typedef struct {
	uint32_t run_timer;
	int8_t pwm1;
	int8_t pwm2;
	int8_t pwm3;
	int8_t pwm4;
	uint16_t max_position1;
	uint16_t min_position1;
	uint16_t max_position2;
	uint16_t min_position2;
	uint16_t max_position4;
	uint16_t min_position4;


} Motor_Typedef;

Motor_Typedef motor;

// Subroutine -----------------------------------------------------


// Code -----------------------------------------------------------
void Motor_Init(void)
{
	Motor_debug=0;
	Motor_timer=0;
	Motor_Set_Position_Limiter(1);

	motor.max_position1 = 513;	//tilt
	motor.min_position1 = 75;	//0;

	motor.max_position2 = 16223;	//vertical
	motor.min_position2 = 0;

	motor.max_position4 = 6077;	//horizontal
	motor.min_position4 = 0;

	motor.run_timer = 0;

	printf("Motor_Init\n");

	HAL_TIM_PWM_Start(&htim3,TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim3,TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim3,TIM_CHANNEL_3);
	HAL_TIM_PWM_Start(&htim3,TIM_CHANNEL_4);
	Motor_Stop();

}

void Motor_Stop(void)
{
    TIM3->CCR1 = 0;
    TIM3->CCR2 = 0;
    TIM3->CCR3 = 0;
    TIM3->CCR4 = 0;
    motor.pwm1 = 0;
    motor.pwm2 = 0;
    motor.pwm3 = 0;
    motor.pwm4 = 0;

}

void Motor_Set_Output(uint8_t motor_id, int8_t output)
{
uint8_t dir=0;
uint16_t pwm;
	motor.run_timer	= HAL_GetTick();
	if(output>=0)
		dir=1;
	pwm = abs(output);
	if(pwm > MAX_PWM_DUTY)
		pwm = MAX_PWM_DUTY;
	if((pwm < MIN_PWM_DUTY)&&(pwm!=0))
		pwm = MIN_PWM_DUTY;
	switch(motor_id)
	{
	case 3:	//pwm1 //PA6 //TILT_MOTOR
	    motor.pwm1 = output;
		if(pwm == 0)	//stop
		{
			HAL_GPIO_WritePin(PWM1_PORT, PWM1_D1_PIN, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(PWM1_PORT, PWM1_D2_PIN, GPIO_PIN_RESET);
		    TIM3->CCR1 = 0;	//PA6
		}
		else
		{
			if(dir<=0)	//forward
			{
				HAL_GPIO_WritePin(PWM1_PORT, PWM1_D1_PIN, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(PWM1_PORT, PWM1_D2_PIN, GPIO_PIN_SET);
			}
			else		//backward
			{
				HAL_GPIO_WritePin(PWM1_PORT, PWM1_D1_PIN, GPIO_PIN_SET);
				HAL_GPIO_WritePin(PWM1_PORT, PWM1_D2_PIN, GPIO_PIN_RESET);
			}
		    TIM3->CCR1 = TIM3->ARR*pwm/100;	//PA6
		}
		break;
	case 1:	//pwm2 //PA7 //VER_MOTOR
	    motor.pwm2 = output;
		if(pwm == 0)	//stop
		{
			HAL_GPIO_WritePin(PWM2_PORT, PWM2_D1_PIN, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(PWM2_PORT, PWM2_D2_PIN, GPIO_PIN_RESET);
			HAL_Delay(50);
		    TIM3->CCR2 = 0;	//PA7
		}
		else
		{
			if(dir>0)	//forward
			{
				HAL_GPIO_WritePin(PWM2_PORT, PWM2_D1_PIN, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(PWM2_PORT, PWM2_D2_PIN, GPIO_PIN_SET);

			}
			else		//backward
			{
				HAL_GPIO_WritePin(PWM2_PORT, PWM2_D1_PIN, GPIO_PIN_SET);
				HAL_GPIO_WritePin(PWM2_PORT, PWM2_D2_PIN, GPIO_PIN_RESET);
			}
		    TIM3->CCR2 = TIM3->ARR*pwm/100;	//PA7
		}
		break;
	case 4:	//pwm3  //PB0  //LATCH_MOTOR
	    motor.pwm3 = output;
		if(pwm == 0)	//stop
		{
//			HAL_GPIO_WritePin(PWM3_PORT, PWM3_D1_PIN, GPIO_PIN_RESET);
//			HAL_GPIO_WritePin(PWM3_PORT, PWM3_D2_PIN, GPIO_PIN_RESET);
		    TIM3->CCR3 = 0;	//PB0
		}
		else
		{
			if(dir>0)	//forward
			{
//				HAL_GPIO_WritePin(PWM3_PORT, PWM3_D1_PIN, GPIO_PIN_SET);
//				HAL_GPIO_WritePin(PWM3_PORT, PWM3_D2_PIN, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(PWM3_PORT, PWM3_DIR_PIN, GPIO_PIN_RESET);
			}
			else		//backward
			{
//				HAL_GPIO_WritePin(PWM3_PORT, PWM3_D1_PIN, GPIO_PIN_RESET);
//				HAL_GPIO_WritePin(PWM3_PORT, PWM3_D2_PIN, GPIO_PIN_SET);
				HAL_GPIO_WritePin(PWM3_PORT, PWM3_DIR_PIN, GPIO_PIN_SET);
			}
		    TIM3->CCR3 = TIM3->ARR*pwm/100;	//PB0
		}
		break;
	case 2:	//pwm4	//PB1	//HOR_MOTOR
	    motor.pwm4 = output;
		if(pwm == 0)	//stop
		{
//			HAL_GPIO_WritePin(PWM4_PORT, PWM4_D1_PIN, GPIO_PIN_RESET);
//			HAL_GPIO_WritePin(PWM4_PORT, PWM4_D2_PIN, GPIO_PIN_RESET);
		    TIM3->CCR4 = 0;	//PB1
		}
		else
		{
			if(dir<=0)	//forward
			{
//				HAL_GPIO_WritePin(PWM4_PORT, PWM4_D1_PIN, GPIO_PIN_SET);
//				HAL_GPIO_WritePin(PWM4_PORT, PWM4_D2_PIN, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(PWM4_PORT, PWM4_DIR_PIN, GPIO_PIN_SET);
			}
			else		//backward
			{
//				HAL_GPIO_WritePin(PWM4_PORT, PWM4_D1_PIN, GPIO_PIN_RESET);
//				HAL_GPIO_WritePin(PWM4_PORT, PWM4_D2_PIN, GPIO_PIN_SET);
				HAL_GPIO_WritePin(PWM4_PORT, PWM4_DIR_PIN, GPIO_PIN_RESET);
			}
		    TIM3->CCR4 = TIM3->ARR*pwm/100;	//PB1
		}
		break;
	default:
		break;

	}

}

void Motor_PWM_output(uint16_t pwm1, uint16_t pwm2, uint16_t pwm3, uint16_t pwm4)
{
    TIM3->CCR1 = TIM3->ARR*pwm1/100;
    TIM3->CCR2 = TIM3->ARR*pwm2/100;
    TIM3->CCR3 = TIM3->ARR*pwm3/100;
    TIM3->CCR4 = TIM3->ARR*pwm4/100;
}

void Motor_Position_Limiter(void)
{
uint16_t position;
	if(Motor_Position_Limiter_Enable !=0)
	{
		position = Encoder_Read(TILT_MOTOR);
		if(((motor.pwm1 > 0) && (position >= motor.max_position1)) || ((motor.pwm1 < 0) && (position <= motor.min_position1)))
		{
			TIM3->CCR1 = 0;
			motor.pwm1 = 0;
			printf("Tilt Motor reached limit, pos=%d\n", position);
		}

		position = Encoder_Read(VER_MOTOR);
		if(((motor.pwm2 > 0) && (position >= motor.max_position2)) || ((motor.pwm2 < 0) && (position <= motor.min_position2)))
		{
			TIM3->CCR2 = 0;
			motor.pwm2 = 0;
			printf("Vertical Motor reached limit, pos=%d\n", position);
		}

		position = Encoder_Read(HOR_MOTOR);
		if(((motor.pwm4 > 0) && (position >= motor.max_position4)) || ((motor.pwm4 < 0) && (position <= motor.min_position4)))
		{
			TIM3->CCR4 = 0;
			motor.pwm4 = 0;
			printf("Horizontal Motor reached limit, pos=%d\n", position);
		}
	}
}

void Motor_Timeout_Handler(void)
{
    if(((HAL_GetTick() - motor.run_timer) > MOTOR_TIMEOUT_MS) && (motor.run_timer!=0))
    {
    	motor.run_timer=0;
    	Motor_Stop();
    	printf("Motor_Timeout\n");
    }
}

void Motor_Set_Position_Limiter(uint8_t enable)
{
	if(enable==0)
		Motor_Position_Limiter_Enable = 0;
	else
		Motor_Position_Limiter_Enable = 1;
}
