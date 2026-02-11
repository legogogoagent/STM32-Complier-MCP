uint16_t Modbus_CalCRC(uint8_t * data, uint8_t length);
bool Modbus_CheckCRC(uint8_t * data, uint8_t total_length);
uint16_t Modbus_CalCRC_16bit(unsigned short * data, uint16_t length);
uint16_t Modbus_CalCRC_Increase(unsigned short data, unsigned int reg_crc);
