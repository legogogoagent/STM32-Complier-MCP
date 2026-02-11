# STM32 Makefile - Generated for Elder_Lifter_STM32
# Target: STM32F103CBTx (Cortex-M3, 128KB Flash, 20KB RAM)
# Toolchain: arm-none-eabi-gcc

# ============================================
# 目标配置
# ============================================
TARGET = Elder_Lifter_STM32
MCU = STM32F103xB

# ============================================
# 工具链
# ============================================
PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
AR = $(PREFIX)ar
LD = $(PREFIX)gcc
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

# ============================================
# 编译标志
# ============================================
CPU = -mcpu=cortex-m3
FPU =
FLOAT-ABI =
MCU_FLAGS = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# 预处理器定义
AS_DEFS =
C_DEFS = -D$(MCU) -DUSE_HAL_DRIVER

# 头文件包含路径
AS_INCLUDES =
C_INCLUDES = \
-ICore/Inc \
-IDrivers/STM32F1xx_HAL_Driver/Inc \
-IDrivers/STM32F1xx_HAL_Driver/Inc/Legacy \
-IDrivers/CMSIS/Device/ST/STM32F1xx/Include \
-IDrivers/CMSIS/Include

# 编译标志
CFLAGS = $(MCU_FLAGS) $(C_DEFS) $(C_INCLUDES) -O2 -Wall -fdata-sections -ffunction-sections -g -gdwarf-2
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

ASFLAGS = $(MCU_FLAGS) $(AS_DEFS) $(AS_INCLUDES) -Wall -fdata-sections -ffunction-sections

# 链接标志
LDFLAGS = $(MCU_FLAGS) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

# 链接脚本
LDSCRIPT = STM32F103CBTX_FLASH.ld

# 库
LIBS = -lc -lm -lnosys
LIBDIR =

# ============================================
# 源文件
# ============================================

# ASM源文件
ASM_SOURCES = \
Core/Startup/startup_stm32f103cbtx.s

# C源文件 - Core
C_SOURCES_CORE = \
Core/Src/main.c \
Core/Src/stm32f1xx_it.c \
Core/Src/stm32f1xx_hal_msp.c \
Core/Src/system_stm32f1xx.c \
Core/Src/syscalls.c \
Core/Src/sysmem.c \
Core/Src/Lifter_Task.c \
Core/Src/Lifter_Main.c \
Core/Src/Motor.c \
Core/Src/Modbus.c \
Core/Src/BMS.c \
Core/Src/Encoder.c \
Core/Src/GPIO.c \
Core/Src/User_Timer.c \
Core/Src/User_Uart.c \
Core/Src/User_ADC.c \
Core/Src/User_Flash.c \
Core/Src/Debug_Serial.c \
Core/Src/Panel_Serial.c \
Core/Src/LED_RS485.c \
Core/Src/Sound_RS485.c \
Core/Src/USB_Serial.c \
Core/Src/Actuator_Control.c

# C源文件 - HAL Library
C_SOURCES_HAL = \
Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal.c \
Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_cortex.c \
Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc.c \
Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc_ex.c \
Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c \
Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio_ex.c \
Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_dma.c \
Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash.c \
Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash_ex.c \
Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_pwr.c \
Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_tim.c \
Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_tim_ex.c \
Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_uart.c \
Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_adc.c \
Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_adc_ex.c \
Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_exti.c

# 合并所有C源文件
C_SOURCES = $(C_SOURCES_CORE) $(C_SOURCES_HAL)

# ============================================
# 构建目录和输出
# ============================================
BUILD_DIR = build

# 对象文件
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))

OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

# 依赖文件
DEPS = $(OBJECTS:.o=.d)

# ============================================
# 构建规则
# ============================================

# 默认目标
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

# 创建构建目录
$(BUILD_DIR):
	mkdir -p $@

# 编译C文件
$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

# 编译ASM文件
$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(ASFLAGS) $< -o $@

# 链接生成ELF
$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(LD) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

# 生成HEX
$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf | $(BUILD_DIR)
	$(HEX) $< $@

# 生成BIN
$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf | $(BUILD_DIR)
	$(BIN) $< $@

# ============================================
# 清理
# ============================================
clean:
	rm -rf $(BUILD_DIR)

# ============================================
# 调试目标
# ============================================
print-%:
	@echo $* = $($*)

# 包含依赖文件
-include $(DEPS)

.PHONY: all clean print-%
