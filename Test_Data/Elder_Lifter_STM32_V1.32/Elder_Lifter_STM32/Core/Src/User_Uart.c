// Includes ------------------------------------------------------------------
#include "main.h"
#include "Lifter_Main.h"
#include "User_Uart.h"
#include "Encoder.h"
#include "Panel_Serial.h"
#include "USB_Serial.h"
#include "Debug_Serial.h"
#include "BMS.h"

// Definition ---------------------------------------------------------


// Private variables ---------------------------------------------------------
UART_RxTypeDef UART_1;
UART_RxTypeDef UART_2;
UART_RxTypeDef UART_3;
uint32_t Print_TimeStamp, Print_DeltaTime;

/* Private function prototypes -----------------------------------------------*/
void User_memcpy(uint8_t *dest,  uint8_t * src, uint16_t size);
void User_UART1_Config(void);
void User_UART2_Config(void);
void User_UART3_Config(void);
uint16_t User_UART_Shift_Buffer(uint8_t* buf, uint16_t* remain_len, uint16_t len);


extern DMA_HandleTypeDef hdma_memtomem_dma1_stream6;

// Code ---------------------------------------------------------

void User_UART_Init(void)
{
	User_UART1_Config();
	User_UART2_Config();
	User_UART3_Config();
	Panel_Serial_Init();
	Encoder_Init();
}

void User_UART1_Config(void)
{
	__HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
	__HAL_UART_ENABLE_IT(&huart1, UART_IT_TC);
	HAL_UART_Receive_DMA(&huart1, UART_1.Rx_DMA_Buf, RxPacketBuf_Size);
}

void User_UART2_Config(void)
{
	//HAL_UART_Receive_IT(&huart2, (uint8_t *)msg_uart2, 1);

	__HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);
	__HAL_UART_ENABLE_IT(&huart2, UART_IT_TC);
	HAL_UART_Receive_DMA(&huart2, UART_2.Rx_DMA_Buf, RxPacketBuf_Size);
}

void User_UART3_Config(void)
{
	__HAL_UART_ENABLE_IT(&huart3, UART_IT_IDLE);
	__HAL_UART_ENABLE_IT(&huart3, UART_IT_TC);
	HAL_UART_Receive_DMA(&huart3, UART_3.Rx_DMA_Buf, RxPacketBuf_Size);
}

void User_UART_Rx_Handler(void)
{
bool used;
	if(UART_1.Rx_PacketReady!=0)
	{
//		printf("Uart1 Rx packet %d bytes\n",UART_1.Rx_PacketLenght);
    	while(UART_1.Rx_PacketLenght > 0)
    	{
    		used=false;

    		if(User_UART_Shift_Buffer(UART_1.RX_Data, &UART_1.Rx_PacketLenght, Debug_Serial_Handler(UART_1.RX_Data, UART_1.Rx_PacketLenght))>0)
    			used=true;	//mark it for waste 1 byte buffer


    		//this should place at the last
    		if((UART_1.Rx_PacketLenght > 0)&&(used==false))
    			User_UART_Shift_Buffer(UART_1.RX_Data, &UART_1.Rx_PacketLenght, 1);	//waste 1 byte only
    	}
		UART_1.Rx_PacketReady=0;

	}

    if(UART_2.Rx_PacketReady!=0)
    {
//		printf("Uart2 Rx packet %d bytes\n",UART_2.Rx_PacketLenght);
    	while(UART_2.Rx_PacketLenght > 0)
    	{
    		used=false;

    		if(User_UART_Shift_Buffer(UART_2.RX_Data, &UART_2.Rx_PacketLenght, Encoder_Serial_Handler(UART_2.RX_Data, UART_2.Rx_PacketLenght))>0)
    			used=true;	//mark it for waste 1 byte buffer

    		if(User_UART_Shift_Buffer(UART_2.RX_Data, &UART_2.Rx_PacketLenght, BMS_Serial_Handler(UART_2.RX_Data, UART_2.Rx_PacketLenght))>0)
    			used=true;	//mark it for waste 1 byte buffer

    		//this should place at the last
    		if((UART_2.Rx_PacketLenght > 0)&&(used==false))
    			User_UART_Shift_Buffer(UART_2.RX_Data, &UART_2.Rx_PacketLenght, 1);	//waste 1 byte only
    	}
		UART_2.Rx_PacketReady=0;
    }

	if(UART_3.Rx_PacketReady!=0)
	{
//		printf("Uart3 Rx packet %d bytes\n",UART_3.Rx_PacketLenght);
		while(UART_3.Rx_PacketLenght > 0)
		{
			used=false;
    		if(User_UART_Shift_Buffer(UART_3.RX_Data, &UART_3.Rx_PacketLenght, Panel_Serial_Handler(UART_3.RX_Data, UART_3.Rx_PacketLenght))>0)
    			used=true;	//mark it for waste 1 byte buffer


			//this should place at the last
			if((UART_3.Rx_PacketLenght > 0)&&(used==false))
				User_UART_Shift_Buffer(UART_3.RX_Data, &UART_3.Rx_PacketLenght, 1);	//waste 1 byte only
		}
		UART_3.Rx_PacketReady=0;
	}

}

uint16_t User_UART_Shift_Buffer(uint8_t* buf, uint16_t* remain_len, uint16_t len)
{
uint16_t tail;
	tail=*remain_len;
	//remove data from buffer, and shift number of len data on buffer
	if(len > tail)
	{
		len = tail;
	}
	for(uint16_t i=0; (i+len)<tail; i++)
	{
		buf[i] = buf[i+len];
	}
	*remain_len -= len;
	return len;
}

void User_UART_PrintTimeStamp(void)
{
uint32_t temp32;

	temp32=HAL_GetTick();
	if(temp32>Print_TimeStamp)
		Print_DeltaTime=temp32-Print_TimeStamp;
	else
		Print_DeltaTime=Print_TimeStamp-temp32;
	Print_TimeStamp=temp32;
	printf("LSCM_@DB dt =%dms\n",(int)Print_DeltaTime);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart == &huart1)
	{
		//printf("LSCM_@DB Uart1 Rx buffer full\n");
        HAL_UART_Receive_DMA(&huart1,UART_1.Rx_DMA_Buf,RxPacketBuf_Size);
	}

	if (huart == &huart2)
	{
		//printf("LSCM_@DB Uart2 Rx buffer full\n");
        HAL_UART_Receive_DMA(&huart2,UART_2.Rx_DMA_Buf,RxPacketBuf_Size);
	}

	if (huart == &huart3)
	{
		//printf("LSCM_@DB Uart3 Rx buffer full\n");
        HAL_UART_Receive_DMA(&huart3,UART_3.Rx_DMA_Buf,RxPacketBuf_Size);
	}

}

void UartRxDMA_IdleInterruptHandler(UART_HandleTypeDef *huart)
{
    if((__HAL_UART_GET_FLAG(&huart1,UART_FLAG_IDLE) != RESET))
    {
        __HAL_UART_CLEAR_IDLEFLAG(&huart1);
        HAL_UART_DMAStop(&huart1);
        if(UART_1.Rx_PacketReady==0)
        {
            UART_1.Rx_PacketLenght =  RxPacketBuf_Size -  __HAL_DMA_GET_COUNTER(huart1.hdmarx);
            if(UART_1.Rx_PacketLenght>RxPacketBuf_Size)
            {
            	UART_1.Rx_PacketLenght=RxPacketBuf_Size;
            }
            User_memcpy(UART_1.RX_Data, UART_1.Rx_DMA_Buf, UART_1.Rx_PacketLenght);
            UART_1.Rx_PacketReady=1;
        }
        else
        {
    		//if(uart_debug!=0)
    		//	printf("LSCM_@DB Uart1 Rx packet missing.\n");
        }
        HAL_UART_Receive_DMA(&huart1,UART_1.Rx_DMA_Buf,RxPacketBuf_Size);
    }
    if((__HAL_UART_GET_FLAG(&huart2,UART_FLAG_IDLE) != RESET))
    {
        __HAL_UART_CLEAR_IDLEFLAG(&huart2);
        HAL_UART_DMAStop(&huart2);
        if(UART_2.Rx_PacketReady==0)
        {
            UART_2.Rx_PacketLenght =  RxPacketBuf_Size -  __HAL_DMA_GET_COUNTER(huart2.hdmarx);
            if(UART_2.Rx_PacketLenght>RxPacketBuf_Size)
            {
            	UART_2.Rx_PacketLenght=RxPacketBuf_Size;
            }
            User_memcpy(UART_2.RX_Data, UART_2.Rx_DMA_Buf, UART_2.Rx_PacketLenght);
            UART_2.Rx_PacketReady=1;
        }
        else
        {
    		//if(uart_debug!=0)
    		//	printf("LSCM_@DB Uart2 Rx packet missing.\n");
        }
        HAL_UART_Receive_DMA(&huart2,UART_2.Rx_DMA_Buf,RxPacketBuf_Size);
    }

    if((__HAL_UART_GET_FLAG(&huart3,UART_FLAG_IDLE) != RESET))
    {
        __HAL_UART_CLEAR_IDLEFLAG(&huart3);
        HAL_UART_DMAStop(&huart3);
        if(UART_3.Rx_PacketReady==0)
        {
            UART_3.Rx_PacketLenght =  RxPacketBuf_Size -  __HAL_DMA_GET_COUNTER(huart3.hdmarx);
            if(UART_3.Rx_PacketLenght>RxPacketBuf_Size)
            {
            	UART_3.Rx_PacketLenght=RxPacketBuf_Size;
            }
            User_memcpy(UART_3.RX_Data, UART_3.Rx_DMA_Buf, UART_3.Rx_PacketLenght);
            UART_3.Rx_PacketReady=1;
        }
        else
        {
    		//if(uart_debug!=0)
    		//	printf("LSCM_@DB Uart3 Rx packet missing.\n");
        }
        HAL_UART_Receive_DMA(&huart3,UART_3.Rx_DMA_Buf,RxPacketBuf_Size);
    }

}


void User_memcpy(uint8_t *dest,  uint8_t * src, uint16_t size)
{
uint16_t i;
	if(size>RxPacketBuf_Size)
		size=RxPacketBuf_Size;
	for(i=0;i<size;i++)
	{
		dest[i]=src[i];
	}
}



