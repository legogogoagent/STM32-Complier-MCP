#!/bin/bash
# STM32 Flash Tool - 使用OpenOCD烧录hex文件
# Usage: ./flash.sh <hex_file> [interface]

set -e

HEX_FILE=${1:-"build/Elder_Lifter_STM32.hex"}
INTERFACE=${2:-"stlink"}

# 检查文件是否存在
if [ ! -f "$HEX_FILE" ]; then
    echo "Error: Hex file not found: $HEX_FILE"
    exit 1
fi

echo "========================================"
echo "STM32 Flash Tool"
echo "========================================"
echo "Hex file: $HEX_FILE"
echo "Interface: $INTERFACE"
echo "========================================"

# 根据接口选择配置文件
case $INTERFACE in
    stlink|stlink-v2)
        INTERFACE_CFG="interface/stlink.cfg"
        ;;
    stlink-v2-1)
        INTERFACE_CFG="interface/stlink-v2-1.cfg"
        ;;
    jlink)
        INTERFACE_CFG="interface/jlink.cfg"
        ;;
    *)
        echo "Error: Unknown interface: $INTERFACE"
        echo "Supported: stlink, stlink-v2-1, jlink"
        exit 1
        ;;
esac

# 目标配置文件（STM32F1系列）
TARGET_CFG="target/stm32f1x.cfg"

# 执行烧录
echo "Connecting to target..."
echo "Programming..."

openocd -f $INTERFACE_CFG -f $TARGET_CFG \
    -c "program $HEX_FILE verify reset exit" 2>&1

echo "========================================"
echo "✓ Flash completed successfully!"
echo "========================================"
