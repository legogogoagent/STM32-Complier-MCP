#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

void Lifter_Init(void);
void Lifter_Main(void);
void Lifter_20Hz_ISR(void);
void Write_Lifter_Setting(void);
unsigned char Read_Lifter_Setting(void);
void Lifter_Layer_Update(char layer);
void Lifter_Layer_ForceUpdate(void);
void Lifter_Auto_Off_Handler(void);
void Lifter_Auto_Off_Update(void);
void Write_Lifter_Setting(void);
void Lifter_Auto_Lock_Foot_Handler(void);
void Lifter_Set_Delay_Off(uint16_t delay);

// Definition ---------------------------------------------------
#define LIFTER_INIT		0
#define LIFTER_IDLE		1
#define LIFTER_UP		2
#define LIFTER_DOWN		3
#define LIFTER_ESTOP	4
#define LIFTER_ERROR	5



extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;
extern bool Lifter_Error_Debug;

