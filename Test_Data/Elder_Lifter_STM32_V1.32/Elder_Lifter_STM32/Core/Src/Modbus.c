// Includes ------------------------------------------------------------------
#include "main.h"
#include "Lifter_main.h"
#include "Modbus.h"

// Definition ---------------------------------------------------------

// Variables ---------------------------------------------------------
uint8_t modbus_debug=0;

// Subroutine -----------------------------------------------------


// Code -----------------------------------------------------------
bool Modbus_CheckCRC(uint8_t * data, uint8_t total_length)
{
uint16_t Modbus_CRC, Packet_CRC;
	Modbus_CRC=Modbus_CalCRC(data,total_length);
	Packet_CRC=data[total_length+1];	//high order byte
	Packet_CRC=Packet_CRC<<8;
	Packet_CRC|=data[total_length];		//low order byte

	if(Packet_CRC==Modbus_CRC)
		return 1;
	if(modbus_debug!=0)
		printf("LSCM_@DB Modbus CRC Error\n");
	return 0;
}

uint16_t Modbus_CalCRC(uint8_t * data, uint8_t length)
{
int j;
unsigned int reg_crc=0xFFFF;

	while(length--)
	{
		reg_crc  ^=  *data++;
		for(j=0;j<8;j++)
		{
			if(reg_crc  &  0x01)  /*  LSB(b0)=1  */
				reg_crc=(reg_crc>>1)  ^  0xA001;
			else
				reg_crc=reg_crc >>1;
		}
	}
	return reg_crc;
}

uint16_t Modbus_CalCRC_16bit(unsigned short * data, uint16_t length)
{
int j;
unsigned int reg_crc=0xFFFF;

	while(length--)
	{
		reg_crc  ^=  *data++;
		for(j=0;j<8;j++)
		{
			if(reg_crc  &  0x01)  /*  LSB(b0)=1  */
				reg_crc=(reg_crc>>1)  ^  0xA001;
			else
				reg_crc=reg_crc >>1;
		}
	}
	return reg_crc;
}

uint16_t Modbus_CalCRC_Increase(unsigned short data, unsigned int reg_crc)
{
	reg_crc  ^=  data;
	for(int j=0;j<8;j++)
	{
		if(reg_crc  &  0x01)  /*  LSB(b0)=1  */
			reg_crc=(reg_crc>>1)  ^  0xA001;
		else
			reg_crc=reg_crc >>1;
	}
	return reg_crc;
}
