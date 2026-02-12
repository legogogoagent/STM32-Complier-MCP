"""STM32 MCP Flash Server - MCP烧录服务器包

提供STM32固件烧录的MCP工具，包括:
- flash_firmware: 烧录固件到MCU
- check_programmer: 检查烧录器状态
"""

__version__ = "0.4.0"
__author__ = "STM32 MCP Team"

from .stm32_flash_server import flash_firmware, check_programmer, get_flash_info

__all__ = ["flash_firmware", "check_programmer", "get_flash_info"]
