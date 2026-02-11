#include "User_Flash.h"
#include "main.h"
#include "stm32f1xx_hal_flash_ex.h"

// FLASH一个扇区数据的缓存
unsigned short FlashBuffer[STM32FLASH_PAGE_SIZE/2] = {0};

/**
 * @brief 内部FLASH读取
 *
 * @param addrStart [IN]读取的地址
 * @param pData     [OUT]指向需要操作的数据
 * @param dataLen   [IN]数据字节长度
 * @return int 读出成功的字节数
 */
int Internal_ReadFlash(unsigned int addrStart,void *pData,unsigned int dataLen)
{
    unsigned int nread = dataLen;
    unsigned char *pBuffer = (unsigned char *)pData;
    const unsigned char *pAddr = (const unsigned char *)addrStart;

    /* 检测地址和数据合法性 */
    if( (!pData) || (addrStart < STM32FLASH_BASE) || (addrStart > STM32FLASH_END) )
    {
        return 0;
    }

    /* 读取数据不少于4字节时，每次读4字节 */
    while((nread >= sizeof(unsigned int)) && ( ((unsigned int)pAddr) <= (STM32FLASH_END - 4) ) )
    {
        *(unsigned int *)pBuffer = *(unsigned int *)pAddr;
        pBuffer += sizeof(unsigned int);
        pAddr += sizeof(unsigned int);
        nread -= sizeof(unsigned int);
    }

    /* 读取剩余不足4字节的数据 */
    while(nread && ( ((unsigned int)pAddr)<STM32FLASH_END ))
    {
        *pBuffer++ = *pAddr++;
        nread--;
    }

    return dataLen - nread;
}




/**
 * @brief 内部Flash页擦除
 *
 * @param pageAddress   [IN]擦除起始地址
 * @param nbPages       [IN]擦除页数
 * @return int 0-OK !0-Err
 */
static int Internal_ErasePage(unsigned int pageAddress,unsigned int nbPages)
{
	uint32_t pageError = 0;

    FLASH_EraseInitTypeDef eraseInit;
    eraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
    eraseInit.PageAddress = pageAddress;
    eraseInit.Banks = FLASH_BANK_1;
    eraseInit.NbPages = nbPages;

    if(HAL_FLASHEx_Erase(&eraseInit,&pageError) != HAL_OK)
    {
        return -1;
    }
    return 0;
}


/**
 * @brief 内部Flash无检查写入(半字写入)
 *
 * @param addrStart [IN]写入的地址
 * @param pData     [IN]执行需要操作的数据
 * @param dataLen   [IN]写入数据的半字数
 * @return unsigned int 实际写入的字节数
 */
static unsigned int Internal_WriteFlashNoCHeck(unsigned int addrStart,const unsigned short *pData,unsigned int dataLen)
{
    unsigned int nwrite = dataLen;              //记录剩余要写入的数据量
    unsigned int addrmax = STM32FLASH_END - 2;  //记录写入的最大FLASH地址

    while(nwrite)
    {
        /* 地址非法检查 */
        if(addrStart > addrmax)
        {
            break;
        }

        HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,addrStart,*pData);
        if( (*(volatile unsigned short*)addrStart) != *pData )
        {
            break;
        }

        nwrite--;
        pData++;
        addrStart += 2;

    }

    return (dataLen - nwrite);
}


/**
 * @brief 内部Flash写入
 *
 * @param addrStart     [IN]写入的地址
 * @param pData         [IN]指向需要操作的数据
 * @param dataLen       [IN]写入数据半字数
 * @return unsigned int 实际写入的字节数
 */
int Internal_WriteFlash(unsigned int addrStart,const unsigned short *pData,unsigned int dataLen)
{
    unsigned int i = 0;
    unsigned int pagepos = 0;       //页位置
    unsigned int pageoff = 0;       //页内偏移地址
    unsigned int pagefre = 0;       //页内空余空间
    unsigned int offset  = 0;       //Address在Flash中的偏移
    unsigned int nwrite = dataLen;  //记录剩余要写入的数据量
    const unsigned short *pBuffer = (const unsigned short *)pData;

    /* 检测非法地址 */
    if( (addrStart < STM32FLASH_BASE) || (addrStart > (STM32FLASH_END - 2)) || (dataLen == 0) || (pData == NULL) )
    {
        return 0;
    }

    /* 解锁FLASH */
    HAL_FLASH_Unlock();


    //计算写入地址在FLASH中的偏移地址
    offset = addrStart - STM32FLASH_BASE;
    //计算当前页位置
    pagepos = offset/STM32FLASH_PAGE_SIZE;
    //计算要写入数据的起始地址在当前页内的偏移地址(半字写入)
    pageoff = ((offset%STM32FLASH_PAGE_SIZE)>>1);
    //计算当前页内空余空间(半字写入)
    pagefre = ((STM32FLASH_PAGE_SIZE>>1) - pageoff);
    //要写入的数量低于当前页空余量
    if(nwrite <= pagefre)
    {
        pagefre = nwrite;
    }

    while(nwrite !=0)
    {
        /* 检查是否超页 */
        if(pagepos >= STM32FLASH_PAGE_NUM)
        {
            //写入地址超出FLASH最大位置
            break;
        }

        /* 读取一页 */
        Internal_ReadFlash(STM32FLASH_BASE + pagepos*STM32FLASH_PAGE_SIZE,FlashBuffer,STM32FLASH_PAGE_SIZE);

        /* 检查是否需要擦除 */
        for(i=0;i<pagefre;i++)
        {
            if( *(FlashBuffer + pageoff + i) != 0xFFFF ) // FLASH擦除后默认内容全为0xFF
            {
                break;
            }
        }

        if(i < pagefre)
        {
            /* 需要擦除 */
            unsigned int count = 0;
            unsigned int index = 0;
            uint32_t PageError = 0;

            FLASH_EraseInitTypeDef pEraseInit;

            /* 擦除一页 */
            pEraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
            pEraseInit.PageAddress = STM32FLASH_BASE+pagepos*STM32FLASH_PAGE_SIZE;
            pEraseInit.Banks = FLASH_BANK_1;
            pEraseInit.NbPages = 1;
            if(HAL_FLASHEx_Erase(&pEraseInit,&PageError) != HAL_OK)
            {
                break;
            }

            /* 复制缓存 */
            for(index = 0;index<pagefre;index++)
            {
                *(FlashBuffer+pageoff+index) = *(pBuffer+index);
            }

            /* 写回FLASH */
            count = Internal_WriteFlashNoCHeck(STM32FLASH_BASE+pagepos*STM32FLASH_PAGE_SIZE,FlashBuffer,(STM32FLASH_PAGE_SIZE>>1));
            if(count != (STM32FLASH_PAGE_SIZE>>1))
            {
                nwrite -= count;
                break;
            }

        }else{
            /* 无需擦除，直接写 */
            unsigned int count = Internal_WriteFlashNoCHeck(addrStart,pBuffer,pagefre);
            if(count != pagefre)
            {
                nwrite -= count;
                break;
            }
        }


        pBuffer += pagefre;             //读取地址递增
        addrStart += (pagefre<<1);      //写入地址递增
        nwrite -= pagefre;              //更新剩余未写入数据量

        pagepos++;                      //下一页
        pageoff = 0;                    //页内偏移地址置零

        /* 根据剩余量计算下次写入数据量 */
        pagefre = nwrite >= (STM32FLASH_PAGE_SIZE>>1) ? (STM32FLASH_PAGE_SIZE>>1) : nwrite;
    }

    /* 解锁FLASH */
    HAL_FLASH_Lock();

    return ((dataLen - nwrite)<<1);
}

void Flash_Test(void)
{
	uint8_t i=0;
	//读数据
	unsigned short r_arr[114]={0,1,2,3,4,5};
	for(i=0;i<114;i++)
		r_arr[i]=i;
	unsigned int calidata_addr = ((unsigned int)0x0800FC00);	//last page;;
	i=0;
	if(i<100)
	{
	    //读取数据
	    Internal_ReadFlash(calidata_addr,r_arr,sizeof(r_arr));
		calidata_addr+=40;
	    i++;
	}

	//写数据
	unsigned short t_arr[114]={10,11,12,13,14,15};
	for(i=0;i<114;i++)
		t_arr[i]=i+10;
	unsigned int writedata_addr = ((unsigned int)0x0800FC00);	//last page;
	i=0;
	if(i<100)
	{
		//写入数据
		Internal_WriteFlash(writedata_addr,(unsigned short*)&t_arr[0],sizeof(t_arr)/2);
		writedata_addr+= 40;
		i++;
	}

}

