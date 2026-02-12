"""STM32 MCP Build Server - MCP编译服务器包

提供STM32固件编译的MCP工具，包括:
- build_firmware: 编译固件
- gcc_parse: GCC错误解析
"""

__version__ = "0.2.0"
__author__ = "STM32 MCP Team"

from .stm32_build_server import build_firmware

__all__ = ["build_firmware"]
