/* FLASH大小：STM32F103C8T6：64K */
//#define STM32FLASH_SIZE             0x00010000UL
#define STM32FLASH_SIZE             0x00020000UL
/* FLASH起始地址 */
#define STM32FLASH_BASE             0x08000000UL
/* FLASH结束地址 */
#define STM32FLASH_END              (STM32FLASH_BASE|STM32FLASH_SIZE)
/* FLASH页大小：1K */
#define STM32FLASH_PAGE_SIZE        0x400
/* FLASH总页数 */
#define STM32FLASH_PAGE_NUM         (STM32FLASH_SIZE/STM32FLASH_PAGE_SIZE)


#define WRITE_START_ADDR            ((unsigned int)0x08008000)
#define WRITE_END_ADDR              ((unsigned int)0x0800C000)



int Internal_ReadFlash(unsigned int addrStart,void *pData,unsigned int dataLen);

int Internal_WriteFlash(unsigned int addrStart,const unsigned short *pData,unsigned int dataLen);

void Flash_Test(void);


