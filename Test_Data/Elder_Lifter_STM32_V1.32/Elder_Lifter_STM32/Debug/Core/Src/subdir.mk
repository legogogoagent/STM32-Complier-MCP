################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/Actuator_Control.c \
../Core/Src/BMS.c \
../Core/Src/Debug_Serial.c \
../Core/Src/Encoder.c \
../Core/Src/GPIO.c \
../Core/Src/LED_RS485.c \
../Core/Src/Lifter_Main.c \
../Core/Src/Lifter_Task.c \
../Core/Src/Modbus.c \
../Core/Src/Motor.c \
../Core/Src/Panel_Serial.c \
../Core/Src/Sound_RS485.c \
../Core/Src/USB_Serial.c \
../Core/Src/User_ADC.c \
../Core/Src/User_Flash.c \
../Core/Src/User_Timer.c \
../Core/Src/User_Uart.c \
../Core/Src/main.c \
../Core/Src/stm32f1xx_hal_msp.c \
../Core/Src/stm32f1xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32f1xx.c 

OBJS += \
./Core/Src/Actuator_Control.o \
./Core/Src/BMS.o \
./Core/Src/Debug_Serial.o \
./Core/Src/Encoder.o \
./Core/Src/GPIO.o \
./Core/Src/LED_RS485.o \
./Core/Src/Lifter_Main.o \
./Core/Src/Lifter_Task.o \
./Core/Src/Modbus.o \
./Core/Src/Motor.o \
./Core/Src/Panel_Serial.o \
./Core/Src/Sound_RS485.o \
./Core/Src/USB_Serial.o \
./Core/Src/User_ADC.o \
./Core/Src/User_Flash.o \
./Core/Src/User_Timer.o \
./Core/Src/User_Uart.o \
./Core/Src/main.o \
./Core/Src/stm32f1xx_hal_msp.o \
./Core/Src/stm32f1xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32f1xx.o 

C_DEPS += \
./Core/Src/Actuator_Control.d \
./Core/Src/BMS.d \
./Core/Src/Debug_Serial.d \
./Core/Src/Encoder.d \
./Core/Src/GPIO.d \
./Core/Src/LED_RS485.d \
./Core/Src/Lifter_Main.d \
./Core/Src/Lifter_Task.d \
./Core/Src/Modbus.d \
./Core/Src/Motor.d \
./Core/Src/Panel_Serial.d \
./Core/Src/Sound_RS485.d \
./Core/Src/USB_Serial.d \
./Core/Src/User_ADC.d \
./Core/Src/User_Flash.d \
./Core/Src/User_Timer.d \
./Core/Src/User_Uart.d \
./Core/Src/main.d \
./Core/Src/stm32f1xx_hal_msp.d \
./Core/Src/stm32f1xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32f1xx.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/Actuator_Control.cyclo ./Core/Src/Actuator_Control.d ./Core/Src/Actuator_Control.o ./Core/Src/Actuator_Control.su ./Core/Src/BMS.cyclo ./Core/Src/BMS.d ./Core/Src/BMS.o ./Core/Src/BMS.su ./Core/Src/Debug_Serial.cyclo ./Core/Src/Debug_Serial.d ./Core/Src/Debug_Serial.o ./Core/Src/Debug_Serial.su ./Core/Src/Encoder.cyclo ./Core/Src/Encoder.d ./Core/Src/Encoder.o ./Core/Src/Encoder.su ./Core/Src/GPIO.cyclo ./Core/Src/GPIO.d ./Core/Src/GPIO.o ./Core/Src/GPIO.su ./Core/Src/LED_RS485.cyclo ./Core/Src/LED_RS485.d ./Core/Src/LED_RS485.o ./Core/Src/LED_RS485.su ./Core/Src/Lifter_Main.cyclo ./Core/Src/Lifter_Main.d ./Core/Src/Lifter_Main.o ./Core/Src/Lifter_Main.su ./Core/Src/Lifter_Task.cyclo ./Core/Src/Lifter_Task.d ./Core/Src/Lifter_Task.o ./Core/Src/Lifter_Task.su ./Core/Src/Modbus.cyclo ./Core/Src/Modbus.d ./Core/Src/Modbus.o ./Core/Src/Modbus.su ./Core/Src/Motor.cyclo ./Core/Src/Motor.d ./Core/Src/Motor.o ./Core/Src/Motor.su ./Core/Src/Panel_Serial.cyclo ./Core/Src/Panel_Serial.d ./Core/Src/Panel_Serial.o ./Core/Src/Panel_Serial.su ./Core/Src/Sound_RS485.cyclo ./Core/Src/Sound_RS485.d ./Core/Src/Sound_RS485.o ./Core/Src/Sound_RS485.su ./Core/Src/USB_Serial.cyclo ./Core/Src/USB_Serial.d ./Core/Src/USB_Serial.o ./Core/Src/USB_Serial.su ./Core/Src/User_ADC.cyclo ./Core/Src/User_ADC.d ./Core/Src/User_ADC.o ./Core/Src/User_ADC.su ./Core/Src/User_Flash.cyclo ./Core/Src/User_Flash.d ./Core/Src/User_Flash.o ./Core/Src/User_Flash.su ./Core/Src/User_Timer.cyclo ./Core/Src/User_Timer.d ./Core/Src/User_Timer.o ./Core/Src/User_Timer.su ./Core/Src/User_Uart.cyclo ./Core/Src/User_Uart.d ./Core/Src/User_Uart.o ./Core/Src/User_Uart.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/stm32f1xx_hal_msp.cyclo ./Core/Src/stm32f1xx_hal_msp.d ./Core/Src/stm32f1xx_hal_msp.o ./Core/Src/stm32f1xx_hal_msp.su ./Core/Src/stm32f1xx_it.cyclo ./Core/Src/stm32f1xx_it.d ./Core/Src/stm32f1xx_it.o ./Core/Src/stm32f1xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32f1xx.cyclo ./Core/Src/system_stm32f1xx.d ./Core/Src/system_stm32f1xx.o ./Core/Src/system_stm32f1xx.su

.PHONY: clean-Core-2f-Src

