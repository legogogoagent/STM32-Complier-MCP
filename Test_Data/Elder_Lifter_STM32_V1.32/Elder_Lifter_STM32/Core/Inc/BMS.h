typedef struct
{
uint8_t	percent;          	//battery remain percent
float voltage;
float current;
bool charging;
} BMS_TypeDef;

void BMS_Init(void);
void BMS_Debug(bool enable);
uint8_t BMS_Read_Percent(void);
uint8_t BMS_Ready(void);
void BMS_GetValue(void);
uint16_t BMS_Serial_Handler(uint8_t* rx_data, uint16_t len);
void BMS_Show_Value(void);
uint8_t BMS_Read_Voltage(void);
