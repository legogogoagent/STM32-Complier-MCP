
#define IR_LEFT		0x10
#define IR_RIGHT	0x20
#define IR_BOTTOM	0x40

#define UP_KEY		1
#define DOWN_KEY	2
#define ESTOP_KEY	4
#define RUN_KEY		8
#define SOS_KEY		0x80

#define KEY_PRESS	0x10
#define KEY_RELEASE	0x20
#define KEY_HOLD	0x40

#define KEY_LED_OFF	0
#define KEY_LED_ON	1
#define KEY_LED_2HZ 5
#define KEY_LED_5HZ 2

void GPIO_Init(void);
uint8_t GPIO_Get_Sensor(void);
uint8_t GPIO_Get_Key(void);
uint8_t Scankey_Handler(void);
uint8_t Scankey_GetKey(void);
void GPIO_Set_Key_Led(uint8_t led);
void GPIO_Set_Buzzer(uint8_t buz);
void GPIO_Trigger_Power_Switch(void);
void Flash_Handler(void);
