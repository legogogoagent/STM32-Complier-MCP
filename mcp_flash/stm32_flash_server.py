"""STM32 MCP Flash Server - FastMCP Server Implementation for Flashing

提供STM32固件烧录的MCP工具，支持ST-Link和OpenOCD。
"""

import os
import subprocess
import re
from pathlib import Path
from typing import Optional, List, Dict, Any
from datetime import datetime

from fastmcp import FastMCP

# 初始化MCP服务器
mcp = FastMCP("stm32-flash-server")

# 配置常量
DEFAULT_PROGRAMMER = "stlink"
DEFAULT_INTERFACE = "swd"
DEFAULT_TIMEOUT = 120
MAX_TIMEOUT = 600


@mcp.tool()
def flash_firmware(
    workspace: str,
    hex_file: str = "",
    programmer: str = "stlink",
    interface: str = "swd",
    verify: bool = True,
    reset: bool = True,
    timeout_sec: int = 120,
) -> Dict[str, Any]:
    """烧录固件到STM32 MCU
    
    使用OpenOCD或ST-Link将hex文件烧录到目标MCU。
    
    Args:
        workspace: 工程根目录绝对路径
        hex_file: hex文件路径（相对于workspace/out/，或绝对路径）
        programmer: 烧录器类型 (stlink, jlink, openocd)
        interface: 接口类型 (swd, jtag)
        verify: 是否验证烧录
        reset: 烧录后是否复位
        timeout_sec: 烧录超时秒数
        
    Returns:
        {
            "ok": bool,
            "exit_code": int,
            "programmer": str,
            "hex_file": str,
            "stdout": str,
            "stderr": str,
            "device_id": str,
            "duration_sec": float,
        }
    """
    start_time = datetime.now()
    
    try:
        # 参数验证
        if timeout_sec < 10 or timeout_sec > MAX_TIMEOUT:
            return {"ok": False, "error": f"timeout_sec必须在10-{MAX_TIMEOUT}之间"}
        
        workspace_path = Path(workspace).resolve()
        if not workspace_path.exists():
            return {"ok": False, "error": f"工作目录不存在: {workspace}"}
        
        # 确定hex文件路径
        if not hex_file:
            # 自动查找out目录下的hex文件
            out_dir = workspace_path / "out" / "artifacts"
            hex_files = list(out_dir.glob("*.hex"))
            if not hex_files:
                return {"ok": False, "error": "未找到hex文件，请指定hex_file参数"}
            hex_file_path = hex_files[0]
        elif os.path.isabs(hex_file):
            hex_file_path = Path(hex_file)
        else:
            # 相对路径，先尝试workspace/out/，再尝试相对路径
            hex_file_path = workspace_path / "out" / "artifacts" / hex_file
            if not hex_file_path.exists():
                hex_file_path = workspace_path / hex_file
        
        if not hex_file_path.exists():
            return {"ok": False, "error": f"hex文件不存在: {hex_file_path}"}
        
        # 根据烧录器类型构建命令
        if programmer in ["stlink", "openocd"]:
            cmd = _build_openocd_cmd(
                str(hex_file_path),
                programmer,
                interface,
                verify,
                reset
            )
        elif programmer == "jlink":
            return {"ok": False, "error": "J-Link支持尚未实现"}
        else:
            return {"ok": False, "error": f"不支持的烧录器类型: {programmer}"}
        
        # 执行烧录
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=timeout_sec
        )
        
        # 计算耗时
        duration = (datetime.now() - start_time).total_seconds()
        
        # 解析设备ID（从输出中提取）
        device_id = _parse_device_id(result.stdout + result.stderr)
        
        return {
            "ok": result.returncode == 0,
            "exit_code": result.returncode,
            "programmer": programmer,
            "interface": interface,
            "hex_file": str(hex_file_path),
            "stdout": result.stdout,
            "stderr": result.stderr,
            "device_id": device_id,
            "duration_sec": duration,
        }
        
    except subprocess.TimeoutExpired:
        return {
            "ok": False,
            "error": f"烧录超时（>{timeout_sec}秒）",
            "duration_sec": timeout_sec,
        }
    except Exception as e:
        return {
            "ok": False,
            "error": f"烧录异常: {str(e)}",
        }


def _build_openocd_cmd(
    hex_file: str,
    programmer: str,
    interface: str,
    verify: bool,
    reset: bool
) -> List[str]:
    """构建OpenOCD烧录命令
    
    Args:
        hex_file: hex文件绝对路径
        programmer: 烧录器类型
        interface: 接口类型
        verify: 是否验证
        reset: 是否复位
        
    Returns:
        OpenOCD命令列表
    """
    # 选择接口配置文件
    if programmer == "stlink":
        interface_cfg = "interface/stlink.cfg"
    elif programmer == "jlink":
        interface_cfg = "interface/jlink.cfg"
    else:
        interface_cfg = f"interface/{interface}.cfg"
    
    # 目标配置文件（STM32F1系列）
    target_cfg = "target/stm32f1x.cfg"
    
    # 构建program命令
    program_cmd = f"program {hex_file}"
    if verify:
        program_cmd += " verify"
    if reset:
        program_cmd += " reset"
    program_cmd += " exit"
    
    return [
        "openocd",
        "-f", interface_cfg,
        "-f", target_cfg,
        "-c", program_cmd
    ]


def _parse_device_id(output: str) -> str:
    """从OpenOCD输出中解析设备ID
    
    Args:
        output: OpenOCD输出文本
        
    Returns:
        设备ID字符串或空字符串
    """
    # 匹配: device id = 0x20036410
    pattern = r"device id\s*=\s*(0x[0-9a-fA-F]+)"
    match = re.search(pattern, output)
    if match:
        return match.group(1)
    return ""


@mcp.tool()
def check_programmer(
    programmer: str = "stlink",
) -> Dict[str, Any]:
    """检查烧录器状态
    
    检测烧录器是否连接并可识别。
    
    Args:
        programmer: 烧录器类型 (stlink, jlink)
        
    Returns:
        {
            "ok": bool,
            "connected": bool,
            "programmer": str,
            "device_id": str,
            "target_voltage": str,
            "details": dict,
        }
    """
    result = {
        "ok": False,
        "connected": False,
        "programmer": programmer,
        "device_id": "",
        "target_voltage": "",
        "details": {}
    }
    
    try:
        if programmer == "stlink":
            # 使用OpenOCD检测ST-Link
            cmd = [
                "openocd",
                "-f", "interface/stlink.cfg",
                "-f", "target/stm32f1x.cfg",
                "-c", "init; exit"
            ]
            
            proc = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=10
            )
            
            output = proc.stdout + proc.stderr
            
            # 检查是否连接成功
            if "STLINK" in output and "device id" in output.lower():
                result["connected"] = True
                result["ok"] = True
                result["device_id"] = _parse_device_id(output)
                
                # 解析目标电压
                voltage_match = re.search(r"Target voltage:\s*([\d.]+)", output)
                if voltage_match:
                    result["target_voltage"] = voltage_match.group(1) + "V"
                
                result["details"]["output"] = output[:500]  # 前500字符
            else:
                result["details"]["error"] = "未检测到ST-Link或目标设备"
                result["details"]["output"] = output[:500]
                
        elif programmer == "jlink":
            result["details"]["error"] = "J-Link检测尚未实现"
        else:
            result["details"]["error"] = f"不支持的烧录器类型: {programmer}"
            
    except subprocess.TimeoutExpired:
        result["details"]["error"] = "检测超时"
    except Exception as e:
        result["details"]["error"] = str(e)
    
    return result


@mcp.tool()
def get_flash_info() -> Dict[str, Any]:
    """获取Flash服务器信息
    
    Returns:
        服务器版本、支持的烧录器等信息
    """
    return {
        "name": "stm32-flash-server",
        "version": "0.4.0",
        "default_programmer": DEFAULT_PROGRAMMER,
        "default_interface": DEFAULT_INTERFACE,
        "default_timeout": DEFAULT_TIMEOUT,
        "supported_programmers": ["stlink", "openocd"],
        "supported_interfaces": ["swd", "jtag"],
        "supported_formats": [".hex", ".bin", ".elf"],
    }


def main():
    """主函数 - 启动MCP服务器"""
    mcp.run()


if __name__ == "__main__":
    main()
