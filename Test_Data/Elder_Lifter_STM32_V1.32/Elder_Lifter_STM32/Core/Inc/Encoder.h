#ifndef INC_ENCODER_H_
#define INC_ENCODER_H_

#define MAX_ENCODER 3

typedef struct
{
uint8_t	Last_Addr;          	//Last Get value address
uint16_t Value[MAX_ENCODER]; 	//Encoder value
uint16_t Zero_Offset[MAX_ENCODER];
uint16_t Raw_Value[MAX_ENCODER];
} Encoder_TypeDef;

void Encoder_Init(void);
void Encoder_Debug(bool enable);
void Encoder_GetValue(void);
uint16_t Encoder_Serial_Handler(uint8_t* rx_data, uint16_t len);
uint16_t Encoder_Read(uint8_t id);
void Encoder_Show_Value(void);
uint8_t Encoder_Ready(void);
void Encoder_Set_Pause(uint16_t time);
void Encoder_Write_Calibrate(void);
void Encoder_Show_Offset(void);

extern Encoder_TypeDef encoder;

#endif /* INC_ENCODER_H_ */
