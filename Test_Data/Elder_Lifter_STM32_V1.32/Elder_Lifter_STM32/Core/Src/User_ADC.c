// Includes -----------------------------------------------------
#include "main.h"
#include "Lifter_Main.h"
#include <User_ADC.h>

// Definition ---------------------------------------------------

// Private variables --------------------------------------------
uint32_t ADC1_Value;
extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
// Private function prototypes -----------------------------------

// Code ---------------------------------------------------------
void User_ADC_Init(void)
{
	HAL_ADCEx_Calibration_Start(&hadc1);
	HAL_ADCEx_Calibration_Start(&hadc2);
	HAL_ADC_Start(&hadc1);
	HAL_ADC_Start(&hadc2);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	ADC1_Value = HAL_ADC_GetValue(hadc);
}

uint32_t Get_ADC1(void)
{
uint32_t value;
	value=HAL_ADC_GetValue(&hadc1);
	return value;
}

uint32_t Get_ADC2(void)
{
uint32_t value;
	value=HAL_ADC_GetValue(&hadc2);
	return value;
}

void Show_ADC(void)
{
float adc1,adc2,current1,current2;
	adc1=(float)HAL_ADC_GetValue(&hadc1)/4095*3.3;
	current1=(adc1-1.65)/0.132;

	adc2=(float)HAL_ADC_GetValue(&hadc2)/4095*3.3;
	current2=(adc2-1.65)/0.132;

	printf("adc1=%.2f, i1=%.2f, adc2=%.2f, i2=%0.2f\n",adc1, current1, adc2, current2);
}
