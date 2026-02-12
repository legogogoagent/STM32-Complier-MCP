"""STM32 MCP Build Server - MCP编译服务器包

提供STM32固件编译的MCP工具，包括:
- build_firmware: 编译固件
- parse_gcc_errors: 解析GCC错误
- gcc_parse: GCC错误解析模块
"""

__version__ = "0.3.0"
__author__ = "STM32 MCP Team"

from .stm32_build_server import build_firmware, parse_gcc_errors
from .gcc_parse import (
    parse_build_log,
    errors_to_dict,
    get_error_summary,
    format_error_for_display,
    ParsedError,
    ErrorType,
    ErrorSeverity,
)

__all__ = [
    "build_firmware",
    "parse_gcc_errors",
    "parse_build_log",
    "errors_to_dict",
    "get_error_summary",
    "format_error_for_display",
    "ParsedError",
    "ErrorType",
    "ErrorSeverity",
]
