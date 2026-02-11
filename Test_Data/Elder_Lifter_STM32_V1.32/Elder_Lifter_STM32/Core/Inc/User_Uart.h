#define RxPacketBuf_Size 128	//64	//64

#define RxPacketBuf_Bulk_Size 2048	//

typedef struct
{
uint8_t  Rx_PacketReady:1;        //IDLE receive flag
uint16_t Rx_PacketLenght;          //receive length
uint8_t  Rx_DMA_Buf[RxPacketBuf_Size]; //DMA receive buffer
uint8_t  RX_Data[RxPacketBuf_Size]; //Received Packet buffer
uint16_t Rx_MinPacketLenght;			//Minimum receive packet length for DMA Idle mode
} UART_RxTypeDef;

typedef struct
{
uint8_t  Rx_PacketReady:1;        //IDLE receive flag
uint16_t Rx_PacketLenght;          //receive length
uint8_t  Rx_DMA_Buf[RxPacketBuf_Bulk_Size]; //DMA receive buffer
uint8_t  RX_Data[RxPacketBuf_Bulk_Size]; //Received Packet buffer
uint16_t Rx_MinPacketLenght;			//Minimum receive packet length for DMA Idle mode
} UART_Bulk_RxTypeDef;

/* variables ---------------------------------------------------------*/
//extern UART_HandleTypeDef huart2;



/* function prototypes -----------------------------------------------*/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void UartRxDMA_IdleInterruptHandler(UART_HandleTypeDef *huart);
void User_UART_Init(void);
void User_UART_Rx_Handler(void);
void User_UART_PrintTimeStamp(void);
