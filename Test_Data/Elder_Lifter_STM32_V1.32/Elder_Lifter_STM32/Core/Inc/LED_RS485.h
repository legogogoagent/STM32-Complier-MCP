#define LED_RED_COLOR 		0xff0000
#define LED_GREEN_COLOR 	0x00ff00
#define LED_YELLOW_COLOR 	0xffff00
#define LED_WHITE_COLOR		0xdfffff
#define LED_OFF_COLOR		0x000000


void LED_RS485_Init(void);
void LED_RS485_Color (uint32_t color, uint8_t duty);
