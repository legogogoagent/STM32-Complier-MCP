"""GCC/LD Error Parser - GCC和LD错误解析器

解析编译和链接阶段的错误信息，提供结构化的错误数据。
支持GCC编译器错误、LD链接器错误、以及make系统错误。
"""

import re
from typing import List, Dict, Any, Optional
from dataclasses import dataclass
from enum import Enum


class ErrorSeverity(Enum):
    """错误严重级别"""
    ERROR = "error"
    WARNING = "warning"
    NOTE = "note"


class ErrorType(Enum):
    """错误类型"""
    COMPILER = "compiler"      # GCC编译错误
    LINKER = "linker"          # LD链接错误
    TOOLCHAIN = "toolchain"    # 工具链错误
    SYSTEM = "system"          # 系统错误
    MAKE = "make"              # Make错误


@dataclass
class ParsedError:
    """解析后的错误结构"""
    type: ErrorType
    severity: ErrorSeverity
    file: str                  # 相对路径
    line: int                  # 行号
    col: int                   # 列号
    message: str               # 错误消息
    raw: str                   # 原始行文本
    code: str = ""             # 错误代码（如果有）
    suggestion: str = ""       # 建议（如果有）


def parse_gcc_error(line: str, workspace: str = "") -> Optional[ParsedError]:
    """解析GCC编译错误
    
    格式: file:line:col: severity: message
    示例: Core/Src/main.c:42:10: error: 'Foo' undeclared
    
    Args:
        line: 错误行文本
        workspace: 工作目录路径（用于路径归一化）
        
    Returns:
        ParsedError对象或None
    """
    # GCC错误正则表达式
    # 匹配: file:line:col: severity: message
    # 支持: error, warning, note, fatal error
    pattern = r'^(.*?):(\d+):(\d+):\s*(fatal error|error|warning|note):\s*(.+)$'
    match = re.match(pattern, line)
    
    if not match:
        return None
    
    file_path = match.group(1)
    line_num = int(match.group(2))
    col_num = int(match.group(3))
    severity_str = match.group(4).lower()
    message = match.group(5).strip()
    
    # 确定严重级别
    severity_map = {
        'error': ErrorSeverity.ERROR,
        'fatal error': ErrorSeverity.ERROR,
        'warning': ErrorSeverity.WARNING,
        'note': ErrorSeverity.NOTE,
    }
    severity = severity_map.get(severity_str, ErrorSeverity.ERROR)
    
    # 归一化路径
    if workspace:
        file_path = normalize_path(file_path, workspace)
    
    return ParsedError(
        type=ErrorType.COMPILER,
        severity=severity,
        file=file_path,
        line=line_num,
        col=col_num,
        message=message,
        raw=line
    )


def parse_ld_error(line: str, workspace: str = "") -> Optional[ParsedError]:
    """解析LD链接错误
    
    常见格式:
    - undefined reference to `symbol'
    - multiple definition of `symbol'
    - cannot find -llibrary
    - section `.text' will not fit in region `FLASH'
    
    Args:
        line: 错误行文本
        workspace: 工作目录路径
        
    Returns:
        ParsedError对象或None
    """
    # 链接错误通常没有文件位置信息
    undefined_pattern = r"undefined reference to [`'](\w+)'"
    multiple_def_pattern = r"multiple definition of [`'](\w+)'"
    cannot_find_pattern = r"cannot find -(\w+)"
    section_overflow_pattern = r"section [`'](\w+)' will not fit in region [`'](\w+)'"
    
    message = line.strip()
    file_path = ""
    line_num = 0
    col_num = 0
    
    if re.search(undefined_pattern, line):
        match = re.search(undefined_pattern, line)
        symbol = match.group(1)
        message = f"未定义的引用: {symbol}"
    elif re.search(multiple_def_pattern, line):
        match = re.search(multiple_def_pattern, line)
        symbol = match.group(1)
        message = f"多重定义: {symbol}"
    elif re.search(cannot_find_pattern, line):
        match = re.search(cannot_find_pattern, line)
        lib = match.group(1)
        message = f"找不到库: -l{lib}"
    elif re.search(section_overflow_pattern, line):
        match = re.search(section_overflow_pattern, line)
        section = match.group(1)
        region = match.group(2)
        message = f"段溢出: {section} 超出 {region} 区域"
    else:
        # 其他链接错误
        if "error" in line.lower() or "undefined" in line.lower():
            message = line.strip()
        else:
            return None
    
    return ParsedError(
        type=ErrorType.LINKER,
        severity=ErrorSeverity.ERROR,
        file=file_path,
        line=line_num,
        col=col_num,
        message=message,
        raw=line
    )


def parse_make_error(line: str) -> Optional[ParsedError]:
    """解析Make错误
    
    常见格式:
    - make: *** [target] Error 1
    - make: *** No rule to make target 'xxx'
    
    Args:
        line: 错误行文本
        
    Returns:
        ParsedError对象或None
    """
    # Make错误模式
    make_error_pattern = r"make:\s*\*\*\*\s*\[(.*?)\]\s*Error\s*(\d+)"
    no_rule_pattern = r"make:\s*\*\*\*\s*No rule to make target ['\"](.+?)['\"]"
    
    if re.search(make_error_pattern, line):
        match = re.search(make_error_pattern, line)
        target = match.group(1)
        error_code = match.group(2)
        return ParsedError(
            type=ErrorType.MAKE,
            severity=ErrorSeverity.ERROR,
            file="",
            line=0,
            col=0,
            message=f"Make目标失败: {target} (错误码 {error_code})",
            raw=line
        )
    elif re.search(no_rule_pattern, line):
        match = re.search(no_rule_pattern, line)
        target = match.group(1)
        return ParsedError(
            type=ErrorType.MAKE,
            severity=ErrorSeverity.ERROR,
            file="",
            line=0,
            col=0,
            message=f"缺少make规则: {target}",
            raw=line
        )
    
    return None


def parse_toolchain_error(line: str) -> Optional[ParsedError]:
    """解析工具链错误
    
    常见格式:
    - arm-none-eabi-gcc: error: ...
    - command not found
    
    Args:
        line: 错误行文本
        
    Returns:
        ParsedError对象或None
    """
    # 工具链命令未找到
    if "command not found" in line:
        return ParsedError(
            type=ErrorType.TOOLCHAIN,
            severity=ErrorSeverity.ERROR,
            file="",
            line=0,
            col=0,
            message="工具链命令未找到",
            raw=line
        )
    
    # GCC工具链错误
    gcc_error_pattern = r"arm-none-eabi-(gcc|g\+\+|ld|as):\s*error:\s*(.+)"
    if re.search(gcc_error_pattern, line):
        match = re.search(gcc_error_pattern, line)
        tool = match.group(1)
        message = match.group(2)
        return ParsedError(
            type=ErrorType.TOOLCHAIN,
            severity=ErrorSeverity.ERROR,
            file="",
            line=0,
            col=0,
            message=f"工具链错误 ({tool}): {message}",
            raw=line
        )
    
    return None


def normalize_path(file_path: str, workspace: str) -> str:
    """归一化路径
    
    将绝对路径转换为相对于workspace的相对路径
    
    Args:
        file_path: 原始文件路径
        workspace: 工作目录路径
        
    Returns:
        归一化后的相对路径
    """
    from pathlib import Path
    
    try:
        file_path_obj = Path(file_path).resolve()
        workspace_obj = Path(workspace).resolve()
        
        # 尝试获取相对路径
        try:
            relative = file_path_obj.relative_to(workspace_obj)
            return str(relative)
        except ValueError:
            # 如果不在workspace下，返回原始路径
            return file_path
    except Exception:
        return file_path


def parse_build_log(log_content: str, workspace: str = "") -> List[ParsedError]:
    """解析完整的编译日志
    
    解析编译日志中的所有错误，按严重级别排序。
    
    Args:
        log_content: 编译日志内容
        workspace: 工作目录路径
        
    Returns:
        ParsedError列表，按严重级别排序（errors在前）
    """
    errors = []
    lines = log_content.splitlines()
    
    for line in lines:
        line = line.strip()
        if not line:
            continue
        
        # 尝试解析不同类型的错误
        error = None
        
        # 1. 尝试解析GCC编译错误
        error = parse_gcc_error(line, workspace)
        
        # 2. 如果不是编译错误，尝试链接错误
        if not error:
            error = parse_ld_error(line, workspace)
        
        # 3. 如果不是链接错误，尝试Make错误
        if not error:
            error = parse_make_error(line)
        
        # 4. 如果不是Make错误，尝试工具链错误
        if not error:
            error = parse_toolchain_error(line)
        
        if error:
            errors.append(error)
    
    # 按严重级别排序：ERROR > WARNING > NOTE
    severity_order = {
        ErrorSeverity.ERROR: 0,
        ErrorSeverity.WARNING: 1,
        ErrorSeverity.NOTE: 2,
    }
    errors.sort(key=lambda e: severity_order.get(e.severity, 99))
    
    return errors


def errors_to_dict(errors: List[ParsedError]) -> List[Dict[str, Any]]:
    """将ParsedError列表转换为字典列表
    
    Args:
        errors: ParsedError列表
        
    Returns:
        字典列表
    """
    return [
        {
            "type": error.type.value,
            "severity": error.severity.value,
            "file": error.file,
            "line": error.line,
            "col": error.col,
            "message": error.message,
            "raw": error.raw,
            "code": error.code,
            "suggestion": error.suggestion,
        }
        for error in errors
    ]


def get_error_summary(errors: List[ParsedError]) -> Dict[str, Any]:
    """获取错误摘要统计
    
    Args:
        errors: ParsedError列表
        
    Returns:
        错误统计信息
    """
    error_count = sum(1 for e in errors if e.severity == ErrorSeverity.ERROR)
    warning_count = sum(1 for e in errors if e.severity == ErrorSeverity.WARNING)
    note_count = sum(1 for e in errors if e.severity == ErrorSeverity.NOTE)
    
    # 按文件分组
    files_with_errors = {}
    for error in errors:
        file_key = error.file if error.file else "(全局)"
        if file_key not in files_with_errors:
            files_with_errors[file_key] = []
        files_with_errors[file_key].append(error)
    
    return {
        "total": len(errors),
        "errors": error_count,
        "warnings": warning_count,
        "notes": note_count,
        "files_affected": len(files_with_errors),
        "file_list": list(files_with_errors.keys())[:10],  # 前10个文件
    }


def format_error_for_display(error: ParsedError) -> str:
    """格式化错误用于显示
    
    Args:
        error: ParsedError对象
        
    Returns:
        格式化后的错误字符串
    """
    if error.file:
        location = f"{error.file}:{error.line}"
        if error.col > 0:
            location += f":{error.col}"
    else:
        location = "(全局)"
    
    severity_emoji = {
        ErrorSeverity.ERROR: "❌",
        ErrorSeverity.WARNING: "⚠️",
        ErrorSeverity.NOTE: "ℹ️",
    }.get(error.severity, "❓")
    
    return f"{severity_emoji} [{error.type.value.upper()}] {location}\n   {error.message}"


# 向后兼容的函数
parse_error_line = parse_gcc_error
