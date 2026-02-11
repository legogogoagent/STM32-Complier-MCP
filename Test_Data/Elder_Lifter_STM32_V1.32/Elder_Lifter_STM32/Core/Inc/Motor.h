#define VER_MOTOR	1
#define HOR_MOTOR	2
#define TILT_MOTOR	3
#define LATCH_MOTOR 	4


void Motor_Init(void);
void Motor_PWM_output(uint16_t pwm1, uint16_t pwm2, uint16_t pwm3, uint16_t pwm4);
void Motor_Set_Output(uint8_t motor, int8_t output);
void Motor_Stop(void);
void Motor_Timeout_Handler(void);
void Motor_Position_Limiter(void);
void Motor_Set_Position_Limiter(uint8_t enable);
