
#define MAX_SERIAL_RX_BUF 128
typedef struct
{
uint8_t  Rx_Ready:1;        //Rx packet ready flag
uint8_t  Rx_Buf[MAX_SERIAL_RX_BUF];
uint16_t Len;
} USB_Serial_TypeDef;

void USB_Serial_Init(void);
void USB_Serial_Tx(uint8_t* data, uint16_t len);
void USB_Serial_Rx_ISR(uint8_t* data, uint32_t len);
unsigned char USB_Serial_Handler();
int _write(int file, char *ptr, int len);
