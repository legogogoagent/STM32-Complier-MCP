// Includes ------------------------------------------------------------------
#include "main.h"
#include "Lifter_Main.h"
#include "GPIO.h"


// Definition ---------------------------------------------------------


#define SENSOR_PORT		GPIOB
#define IR_LEFT_PIN		GPIO_PIN_3	//12
#define IR_RIGHT_PIN	GPIO_PIN_4	//13
#define IR_BOTTOM_PIN	GPIO_PIN_5	//14

#define KEY_PORT		GPIOA
#define RUN_KEY_PIN		GPIO_PIN_8
#define SOS_KEY_PIN		GPIO_PIN_9
#define UP_KEY_PIN		GPIO_PIN_10
#define DOWN_KEY_PIN	GPIO_PIN_11
#define ESTOP_PIN		GPIO_PIN_12

#define LED_PORT 		GPIOB
#define KEY_LED_PIN		GPIO_PIN_15

#define BUZZER_PORT 	GPIOC
#define BUZZER_PIN		GPIO_PIN_15

#define PWR_SW_PORT 	GPIOA
#define PWR_SW_PIN		GPIO_PIN_15

#define SENSOR_DEBOUNCE_COUNT 3
#define KEY_DEBOUNCE_COUNT 4
// Variables ---------------------------------------------------------
typedef struct
{
	uint8_t  key;
	uint8_t  new_key;
	uint8_t  key_image;
	uint8_t  key_ready:1;
	uint8_t  debounce;

} Scankey_TypeDef;

typedef struct
{
	uint8_t  state;
	uint8_t  flash_divider;
	bool	 flash;

} LED_TypeDef;

Scankey_TypeDef Scankey;
LED_TypeDef Key_LED;
uint8_t sensor_image, sensor_debounce;

// Subroutine -----------------------------------------------------


// Code -----------------------------------------------------------
void GPIO_Init(void)
{
	printf("GPIO_Init\n");

	GPIO_Set_Key_Led(KEY_LED_OFF);
}

uint8_t GPIO_Get_Sensor(void)
{
uint8_t result=0;
	do
	{
		if(HAL_GPIO_ReadPin(SENSOR_PORT,IR_LEFT_PIN)==0)
			result |= IR_LEFT;
		if(HAL_GPIO_ReadPin(SENSOR_PORT,IR_RIGHT_PIN)==0)
			result |= IR_RIGHT;
		if(HAL_GPIO_ReadPin(SENSOR_PORT,IR_BOTTOM_PIN)==0)
			result |= IR_BOTTOM;
		if(HAL_GPIO_ReadPin(KEY_PORT,UP_KEY_PIN)==0)
			result |= UP_KEY;
		if(HAL_GPIO_ReadPin(KEY_PORT,DOWN_KEY_PIN)==0)
			result |= DOWN_KEY;
		if(HAL_GPIO_ReadPin(KEY_PORT,ESTOP_PIN)==0)
			result |= ESTOP_KEY;

		if(result!=sensor_image)
		{
			sensor_image=result;
			sensor_debounce=SENSOR_DEBOUNCE_COUNT;
		}
		else
		{
			if(sensor_debounce!=0)
				sensor_debounce--;
		}

	}
	while(sensor_debounce!=0);

	return result;
}

uint8_t GPIO_Get_Key(void)
{
uint8_t result=0;
	if(HAL_GPIO_ReadPin(KEY_PORT,UP_KEY_PIN)==0)
		result |= UP_KEY;
	if(HAL_GPIO_ReadPin(KEY_PORT,DOWN_KEY_PIN)==0)
		result |= DOWN_KEY;
	if(HAL_GPIO_ReadPin(KEY_PORT,ESTOP_PIN)==0)
		result |= ESTOP_KEY;
	if(HAL_GPIO_ReadPin(KEY_PORT,RUN_KEY_PIN)==0)
		result |= RUN_KEY;
	if(HAL_GPIO_ReadPin(KEY_PORT,SOS_KEY_PIN)==0)
		result |= SOS_KEY;

	return result;
}

void GPIO_Set_Key_Led(uint8_t led)
{
	Key_LED.state=led;
	if(led==KEY_LED_OFF)
	{
		HAL_GPIO_WritePin(LED_PORT, KEY_LED_PIN, GPIO_PIN_RESET);

	}
	else
	{
		HAL_GPIO_WritePin(LED_PORT, KEY_LED_PIN, GPIO_PIN_SET);
		Key_LED.flash_divider = led;	//for LED flash
		Key_LED.flash = 0;
	}
}


void GPIO_Set_Buzzer(uint8_t buz)
{
	if(buz==0)
		HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
	else
		HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);

}

void GPIO_Trigger_Power_Switch(void)
{
	HAL_GPIO_WritePin(PWR_SW_PORT, PWR_SW_PIN, GPIO_PIN_SET);
	HAL_Delay(100);
	HAL_GPIO_WritePin(PWR_SW_PORT, PWR_SW_PIN, GPIO_PIN_RESET);

}

void Flash_Handler(void)	//10Hz timebase
{
	if(Key_LED.state>1)	//flash mode
	{
		if(--Key_LED.flash_divider==0)
		{
			Key_LED.flash_divider = Key_LED.state;
			if(Key_LED.flash==0)
			{
				Key_LED.flash = 1;
				HAL_GPIO_WritePin(LED_PORT, KEY_LED_PIN, GPIO_PIN_RESET);
			}
			else
			{
				Key_LED.flash = 0;
				HAL_GPIO_WritePin(LED_PORT, KEY_LED_PIN, GPIO_PIN_SET);
			}
		}
	}
}

uint8_t Scankey_Handler(void)
{
	Scankey.key=GPIO_Get_Key();
	if(Scankey.key == Scankey.key_image)
		return 0;		//no key change;
	else
	{
		if(Scankey.debounce!=0)
		{
			Scankey.debounce--;
			return 0;
		}
	}
	Scankey.debounce=KEY_DEBOUNCE_COUNT;
	Scankey.new_key = Scankey.key ^ Scankey.key_image;
	Scankey.key_image = Scankey.key;
	if((Scankey.new_key & Scankey.key) != 0)	//key press
		Scankey.new_key |= KEY_PRESS;
	else
		Scankey.new_key |= KEY_RELEASE;

	Scankey.key_ready=1;
	return Scankey.new_key;
}

uint8_t Scankey_GetKey(void)
{
	if(Scankey.key_ready==0)
		return 0;
	Scankey.key_ready=0;
	return Scankey.new_key;

}

