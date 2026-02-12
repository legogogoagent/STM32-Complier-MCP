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

from mcp_flash.mcu_database import (
    get_mcu_info,
    get_target_config_by_family,
    detect_family_from_id,
    list_supported_mcus,
    get_supported_families,
    MCUFamily,
    STM32_MCU_DATABASE,
    FAMILY_TARGET_MAP,
)

# 初始化MCP服务器
mcp = FastMCP("stm32-flash-server")

# 配置常量
DEFAULT_PROGRAMMER = "stlink"
DEFAULT_INTERFACE = "swd"
DEFAULT_TIMEOUT = 120
MAX_TIMEOUT = 600

# Docker配置
DEFAULT_DOCKER_IMAGE = "stm32-flash-toolchain:latest"
DOCKER_FLASH_TIMEOUT = 180

# 默认目标配置（向后兼容）
DEFAULT_TARGET_CONFIG = "stm32f1x.cfg"


@mcp.tool()
def flash_firmware(
    workspace: str,
    hex_file: str = "",
    programmer: str = "stlink",
    interface: str = "swd",
    verify: bool = True,
    reset: bool = True,
    timeout_sec: int = 120,
    auto_detect: bool = True,
    target_family: str = "",
) -> Dict[str, Any]:
    """烧录固件到STM32 MCU（支持多目标自动检测）
    
    使用OpenOCD或ST-Link将hex文件烧录到目标MCU，支持自动检测MCU类型。
    
    Args:
        workspace: 工程根目录绝对路径
        hex_file: hex文件路径（相对于workspace/out/，或绝对路径）
        programmer: 烧录器类型 (stlink, jlink, openocd)
        interface: 接口类型 (swd, jtag)
        verify: 是否验证烧录
        reset: 烧录后是否复位
        timeout_sec: 烧录超时秒数
        auto_detect: 是否自动检测MCU类型
        target_family: 目标MCU系列（当auto_detect=False时使用，如"F4", "F7", "H7"）
        
    Returns:
        {
            "ok": bool,
            "exit_code": int,
            "programmer": str,
            "hex_file": str,
            "device_id": str,
            "mcu_info": dict,
            "target_config": str,
            "stdout": str,
            "stderr": str,
            "duration_sec": float,
        }
    """
    start_time = datetime.now()
    target_config = DEFAULT_TARGET_CONFIG
    mcu_info = None
    
    try:
        if timeout_sec < 10 or timeout_sec > MAX_TIMEOUT:
            return {"ok": False, "error": f"timeout_sec必须在10-{MAX_TIMEOUT}之间"}
        
        workspace_path = Path(workspace).resolve()
        if not workspace_path.exists():
            return {"ok": False, "error": f"工作目录不存在: {workspace}"}
        
        # 确定hex文件路径
        if not hex_file:
            out_dir = workspace_path / "out" / "artifacts"
            hex_files = list(out_dir.glob("*.hex"))
            if not hex_files:
                return {"ok": False, "error": "未找到hex文件，请指定hex_file参数"}
            hex_file_path = hex_files[0]
        elif os.path.isabs(hex_file):
            hex_file_path = Path(hex_file)
        else:
            hex_file_path = workspace_path / "out" / "artifacts" / hex_file
            if not hex_file_path.exists():
                hex_file_path = workspace_path / hex_file
        
        if not hex_file_path.exists():
            return {"ok": False, "error": f"hex文件不存在: {hex_file_path}"}
        
        if auto_detect:
            detect_result = _detect_mcu_internal(programmer, interface, timeout_sec=10)
            if detect_result["detected"]:
                target_config = detect_result["target_config"]
                mcu_info = detect_result.get("mcu_info")
        elif target_family:
            # 手动指定系列
            family_enum = None
            try:
                family_enum = MCUFamily(f"STM32{target_family.upper()}")
            except ValueError:
                pass
            
            if family_enum:
                target_config = get_target_config_by_family(family_enum)
            else:
                # 直接使用配置文件名
                target_config = f"stm32{target_family.lower()}x.cfg"
        
        # 构建烧录命令
        if programmer in ["stlink", "openocd"]:
            cmd = _build_openocd_cmd(
                str(hex_file_path),
                programmer,
                interface,
                verify,
                reset,
                target_config=target_config
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
        
        # 解析设备ID
        device_id = _parse_device_id(result.stdout + result.stderr)
        
        return {
            "ok": result.returncode == 0,
            "exit_code": result.returncode,
            "programmer": programmer,
            "interface": interface,
            "hex_file": str(hex_file_path),
            "device_id": device_id,
            "mcu_info": mcu_info,
            "target_config": target_config,
            "stdout": result.stdout,
            "stderr": result.stderr,
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
    reset: bool,
    target_config: str = ""
) -> List[str]:
    """构建OpenOCD烧录命令
    
    Args:
        hex_file: hex文件绝对路径
        programmer: 烧录器类型
        interface: 接口类型
        verify: 是否验证
        reset: 是否复位
        target_config: 目标配置文件名（可选，默认使用stm32f1x.cfg）
        
    Returns:
        OpenOCD命令列表
    """
    interface_cfg = _get_interface_config(programmer)
    
    # 使用指定的目标配置或默认配置
    target_cfg = f"target/{target_config}" if target_config else f"target/{DEFAULT_TARGET_CONFIG}"
    
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
def flash_firmware_docker(
    workspace: str,
    hex_file: str = "",
    programmer: str = "stlink",
    interface: str = "swd",
    verify: bool = True,
    reset: bool = True,
    timeout_sec: int = 180,
    docker_image: str = "stm32-flash-toolchain:latest",
) -> Dict[str, Any]:
    """使用Docker容器烧录固件到STM32 MCU
    
    在隔离的Docker容器中执行烧录操作，支持USB设备透传。
    
    Args:
        workspace: 工程根目录绝对路径
        hex_file: hex文件路径（相对于workspace/out/，或绝对路径）
        programmer: 烧录器类型 (stlink, jlink, openocd)
        interface: 接口类型 (swd, jtag)
        verify: 是否验证烧录
        reset: 烧录后是否复位
        timeout_sec: 烧录超时秒数
        docker_image: Docker镜像名称
        
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
            "docker_info": dict,
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
            out_dir = workspace_path / "out" / "artifacts"
            hex_files = list(out_dir.glob("*.hex"))
            if not hex_files:
                return {"ok": False, "error": "未找到hex文件，请指定hex_file参数"}
            hex_file_path = hex_files[0]
        elif os.path.isabs(hex_file):
            hex_file_path = Path(hex_file)
        else:
            hex_file_path = workspace_path / "out" / "artifacts" / hex_file
            if not hex_file_path.exists():
                hex_file_path = workspace_path / hex_file
        
        if not hex_file_path.exists():
            return {"ok": False, "error": f"hex文件不存在: {hex_file_path}"}
        
        # 构建Docker命令
        docker_cmd = _build_docker_flash_cmd(
            workspace=str(workspace_path),
            hex_file=str(hex_file_path),
            programmer=programmer,
            interface=interface,
            verify=verify,
            reset=reset,
            docker_image=docker_image
        )
        
        # 执行Docker烧录
        result = subprocess.run(
            docker_cmd,
            capture_output=True,
            text=True,
            timeout=timeout_sec
        )
        
        # 计算耗时
        duration = (datetime.now() - start_time).total_seconds()
        
        # 解析设备ID
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
            "docker_info": {
                "image": docker_image,
                "privileged": True,
                "usb_passthrough": True,
            }
        }
        
    except subprocess.TimeoutExpired:
        return {
            "ok": False,
            "error": f"Docker烧录超时（>{timeout_sec}秒）",
            "duration_sec": timeout_sec,
        }
    except Exception as e:
        return {
            "ok": False,
            "error": f"Docker烧录异常: {str(e)}",
        }


def _build_docker_flash_cmd(
    workspace: str,
    hex_file: str,
    programmer: str,
    interface: str,
    verify: bool,
    reset: bool,
    docker_image: str
) -> List[str]:
    """构建Docker烧录命令
    
    Args:
        workspace: 工作目录
        hex_file: hex文件绝对路径
        programmer: 烧录器类型
        interface: 接口类型
        verify: 是否验证
        reset: 是否复位
        docker_image: Docker镜像名称
        
    Returns:
        Docker命令列表
    """
    hex_path = Path(hex_file)
    hex_dir = str(hex_path.parent)
    hex_name = hex_path.name
    
    # 构建OpenOCD命令字符串
    program_cmd = f"program /out/{hex_name}"
    if verify:
        program_cmd += " verify"
    if reset:
        program_cmd += " reset"
    program_cmd += " exit"
    
    # 选择接口配置
    if programmer == "stlink":
        interface_cfg = "interface/stlink.cfg"
    elif programmer == "jlink":
        interface_cfg = "interface/jlink.cfg"
    else:
        interface_cfg = f"interface/{interface}.cfg"
    
    target_cfg = "target/stm32f1x.cfg"
    
    # 构建Docker命令
    cmd = [
        "docker", "run",
        "--rm",  # 运行后自动删除容器
        "--privileged",  # 特权模式（USB访问必需）
        "-v", f"{hex_dir}:/out:ro",  # hex文件目录挂载
        "-v", "/dev/bus/usb:/dev/bus/usb",  # USB设备透传
        "--network", "none",  # 禁用网络
        docker_image,
        "openocd",
        "-f", interface_cfg,
        "-f", target_cfg,
        "-c", program_cmd
    ]
    
    return cmd


@mcp.tool()
def check_docker_environment() -> Dict[str, Any]:
    """检查Docker环境是否就绪
    
    检查Docker是否安装、镜像是否存在、USB权限是否配置。
    
    Returns:
        {
            "ok": bool,
            "docker_installed": bool,
            "docker_version": str,
            "image_exists": bool,
            "image_name": str,
            "usb_rules_configured": bool,
            "details": dict,
        }
    """
    result = {
        "ok": False,
        "docker_installed": False,
        "docker_version": "",
        "image_exists": False,
        "image_name": DEFAULT_DOCKER_IMAGE,
        "usb_rules_configured": False,
        "details": {}
    }
    
    try:
        # 检查Docker是否安装
        docker_version = subprocess.run(
            ["docker", "--version"],
            capture_output=True,
            text=True,
            timeout=5
        )
        
        if docker_version.returncode == 0:
            result["docker_installed"] = True
            result["docker_version"] = docker_version.stdout.strip()
        else:
            result["details"]["docker_error"] = "Docker未安装或无法访问"
            return result
        
        # 检查镜像是否存在
        image_check = subprocess.run(
            ["docker", "images", "-q", DEFAULT_DOCKER_IMAGE],
            capture_output=True,
            text=True,
            timeout=5
        )
        
        if image_check.stdout.strip():
            result["image_exists"] = True
        else:
            result["details"]["image_error"] = f"镜像 {DEFAULT_DOCKER_IMAGE} 不存在，请运行: docker build -f docker/flash.Dockerfile -t {DEFAULT_DOCKER_IMAGE} ."
        
        # 检查USB规则文件
        usb_rules_paths = [
            "/etc/udev/rules.d/99-stlink.rules",
            "/etc/udev/rules.d/99-jlink.rules",
            "/etc/udev/rules.d/99-cmsis-dap.rules"
        ]
        
        configured_rules = []
        for rules_path in usb_rules_paths:
            if os.path.exists(rules_path):
                configured_rules.append(os.path.basename(rules_path))
        
        if configured_rules:
            result["usb_rules_configured"] = True
            result["details"]["configured_rules"] = configured_rules
        else:
            result["details"]["usb_error"] = "未配置USB规则，请运行: sudo ./scripts/setup-usb-rules.sh"
        
        # 检查是否可以通过docker访问USB设备
        usb_test = subprocess.run(
            ["docker", "run", "--rm", "--privileged", "-v", "/dev/bus/usb:/dev/bus/usb", "alpine:latest", "ls", "/dev/bus/usb"],
            capture_output=True,
            text=True,
            timeout=10
        )
        
        if usb_test.returncode == 0:
            result["details"]["usb_access"] = "OK"
        else:
            result["details"]["usb_access"] = "受限"
        
        # 总体状态
        result["ok"] = result["docker_installed"] and result["image_exists"]
        
    except subprocess.TimeoutExpired:
        result["details"]["error"] = "检查超时"
    except Exception as e:
        result["details"]["error"] = str(e)
    
    return result


@mcp.tool()
def get_flash_info() -> Dict[str, Any]:
    """获取Flash服务器信息
    
    Returns:
        服务器版本、支持的烧录器等信息
    """
    families = get_supported_families()
    
    return {
        "name": "stm32-flash-server",
        "version": "0.7.0",
        "default_programmer": DEFAULT_PROGRAMMER,
        "default_interface": DEFAULT_INTERFACE,
        "default_timeout": DEFAULT_TIMEOUT,
        "supported_programmers": ["stlink", "openocd", "esp32"],
        "supported_interfaces": ["swd", "jtag", "wifi"],
        "supported_formats": [".hex", ".bin", ".elf"],
        "multi_target": {
            "enabled": True,
            "auto_detect": True,
            "supported_families": [f["name"] for f in families],
            "mcu_database_version": "1.0.0",
            "total_mcus": len(STM32_MCU_DATABASE),
        },
        "docker_support": {
            "enabled": True,
            "default_image": DEFAULT_DOCKER_IMAGE,
            "usb_passthrough": True,
        },
        "remote_support": {
            "enabled": True,
            "esp32_bridge": {
                "enabled": True,
                "default_host": "192.168.4.1",
                "default_port": 4444,
                "description": "通过ESP32 WiFi桥远程烧录STM32"
            }
        }
    }


def _detect_mcu_internal(
    programmer: str = "stlink",
    interface: str = "swd",
    timeout_sec: int = 10,
) -> Dict[str, Any]:
    """内部函数：执行MCU检测"""
    result = {
        "ok": False,
        "detected": False,
        "device_id": "",
        "mcu_info": None,
        "target_config": DEFAULT_TARGET_CONFIG,
        "programmer": programmer,
        "details": {}
    }
    
    try:
        interface_cfg = _get_interface_config(programmer)
        
        cmd = [
            "openocd",
            "-f", interface_cfg,
            "-f", f"target/{DEFAULT_TARGET_CONFIG}",
            "-c", "init",
            "-c", "exit"
        ]
        
        proc = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=timeout_sec
        )
        
        output = proc.stdout + proc.stderr
        result["details"]["raw_output"] = output[:1000]
        
        device_id = _parse_device_id(output)
        
        if device_id:
            result["detected"] = True
            result["device_id"] = device_id
            
            mcu_info = get_mcu_info(device_id)
            if mcu_info:
                result["mcu_info"] = {
                    "name": mcu_info.name,
                    "family": mcu_info.family.value,
                    "flash_kb": mcu_info.flash_size_kb,
                    "ram_kb": mcu_info.ram_size_kb,
                    "core": mcu_info.core,
                    "description": mcu_info.description,
                    "max_clock_mhz": mcu_info.max_clock_mhz,
                }
                result["target_config"] = mcu_info.target_config
                result["ok"] = True
            else:
                family = detect_family_from_id(device_id)
                if family:
                    result["target_config"] = get_target_config_by_family(family)
                    result["mcu_info"] = {
                        "device_id": device_id,
                        "family": family.value,
                        "note": "未知具体型号，使用系列默认配置"
                    }
                    result["ok"] = True
                else:
                    result["details"]["warning"] = f"设备ID {device_id} 不在数据库中，无法识别系列"
                    result["ok"] = True
        else:
            result["details"]["error"] = "无法读取设备ID，请检查连接"
            
    except subprocess.TimeoutExpired:
        result["details"]["error"] = "检测超时"
    except Exception as e:
        result["details"]["error"] = str(e)
    
    return result


@mcp.tool()
def detect_mcu(
    programmer: str = "stlink",
    interface: str = "swd",
    timeout_sec: int = 10,
) -> Dict[str, Any]:
    """自动检测连接的MCU类型"""
    return _detect_mcu_internal(programmer, interface, timeout_sec)


@mcp.tool()
def flash_firmware_esp32(
    workspace: str,
    hex_file: str = "",
    esp32_host: str = "192.168.4.1",
    esp32_port: int = 4444,
    verify: bool = True,
    timeout_sec: int = 300,
) -> Dict[str, Any]:
    """使用ESP32远程桥烧录固件到STM32 MCU
    
    通过WiFi连接的ESP32 Bridge远程烧录STM32固件。
    ESP32通过SWD引脚(GPIO18=SWDIO, GPIO19=SWCLK)连接STM32。
    
    Args:
        workspace: 工程根目录绝对路径
        hex_file: hex/bin文件路径（相对于workspace/out/，或绝对路径）
        esp32_host: ESP32 IP地址 (默认192.168.4.1 AP模式)
        esp32_port: ESP32 TCP端口 (默认4444)
        verify: 是否验证烧录 (ESP32自动验证)
        timeout_sec: 烧录超时秒数 (默认300秒)
        
    Returns:
        {
            "ok": bool,
            "exit_code": int,
            "programmer": "esp32",
            "esp32_host": str,
            "esp32_port": int,
            "hex_file": str,
            "device_id": str,
            "stdout": str,
            "stderr": str,
            "duration_sec": float,
        }
    """
    import time
    from pathlib import Path
    
    start_time = time.time()
    
    try:
        # 检查ESP32客户端可用性
        try:
            import sys
            sys.path.insert(0, str(Path(__file__).parent.parent / 'ESP32_STM32_Bridge' / 'scripts'))
            from esp32_bridge_client import ESP32BridgeClient, BridgeError, FlashError
        except ImportError as e:
            return {
                "ok": False,
                "error": f"ESP32 Bridge客户端不可用: {str(e)}。请确保ESP32_STM32_Bridge/scripts/esp32_bridge_client.py存在",
                "duration_sec": 0,
            }
        
        # 参数验证
        if timeout_sec < 10 or timeout_sec > 600:
            return {"ok": False, "error": f"timeout_sec必须在10-600之间"}
        
        workspace_path = Path(workspace).resolve()
        if not workspace_path.exists():
            return {"ok": False, "error": f"工作目录不存在: {workspace}"}
        
        # 确定固件文件路径
        firmware_path = None
        firmware_format = ""
        
        if not hex_file:
            # 自动查找out目录中的固件
            out_dir = workspace_path / "out" / "artifacts"
            if out_dir.exists():
                # 优先找.bin文件
                bin_files = list(out_dir.glob("*.bin"))
                if bin_files:
                    firmware_path = bin_files[0]
                    firmware_format = "bin"
                else:
                    hex_files = list(out_dir.glob("*.hex"))
                    if hex_files:
                        firmware_path = hex_files[0]
                        firmware_format = "hex"
            
            if not firmware_path:
                return {"ok": False, "error": "未找到固件文件(.bin或.hex)，请指定hex_file参数"}
        elif Path(hex_file).is_absolute():
            firmware_path = Path(hex_file)
        else:
            # 尝试相对路径
            out_dir = workspace_path / "out" / "artifacts" / hex_file
            if out_dir.exists():
                firmware_path = out_dir
            else:
                firmware_path = workspace_path / hex_file
        
        if not firmware_path.exists():
            return {"ok": False, "error": f"固件文件不存在: {firmware_path}"}
        
        # 确定格式
        if not firmware_format:
            if firmware_path.suffix.lower() == ".bin":
                firmware_format = "bin"
            elif firmware_path.suffix.lower() == ".hex":
                firmware_format = "hex"
            else:
                return {"ok": False, "error": f"不支持的固件格式: {firmware_path.suffix}"}
        
        # 连接到ESP32
        stdout_lines = []
        stderr_lines = []
        
        stdout_lines.append(f"连接到ESP32 Bridge {esp32_host}:{esp32_port}...")
        
        try:
            client = ESP32BridgeClient(esp32_host, esp32_port, timeout=30.0)
            client.connect()
            
            version = client.get_version()
            stdout_lines.append(f"连接成功: {version}")
            
            # 读取IDCODE
            idcode = client.read_idcode()
            device_id = f"0x{idcode:08X}"
            stdout_lines.append(f"检测到MCU: {device_id}")
            
        except BridgeError as e:
            stderr_lines.append(f"连接失败: {str(e)}")
            return {
                "ok": False,
                "exit_code": 1,
                "programmer": "esp32",
                "esp32_host": esp32_host,
                "esp32_port": esp32_port,
                "hex_file": str(firmware_path),
                "device_id": "",
                "stdout": "\n".join(stdout_lines),
                "stderr": "\n".join(stderr_lines),
                "duration_sec": time.time() - start_time,
            }
        
        # 读取固件数据
        stdout_lines.append(f"读取固件: {firmware_path.name}")
        
        with open(firmware_path, "rb") as f:
            if firmware_format == "hex":
                # 如果是hex文件，需要先转换为bin
                import struct
                hex_content = f.read().decode('utf-8')
                binary_data = _convert_hex_to_bin(hex_content)
                stdout_lines.append(f"HEX转BIN: {len(binary_data)} bytes")
            else:
                binary_data = f.read()
        
        firmware_size = len(binary_data)
        stdout_lines.append(f"固件大小: {firmware_size} bytes")
        
        # 烧录固件
        stdout_lines.append("开始烧录...")
        
        try:
            client.flash_firmware(binary_data, show_progress=False)
            stdout_lines.append("烧录完成!")
        except FlashError as e:
            stderr_lines.append(f"烧录失败: {str(e)}")
            client.close()
            return {
                "ok": False,
                "exit_code": 2,
                "programmer": "esp32",
                "esp32_host": esp32_host,
                "esp32_port": esp32_port,
                "hex_file": str(firmware_path),
                "device_id": device_id,
                "stdout": "\n".join(stdout_lines),
                "stderr": "\n".join(stderr_lines),
                "duration_sec": time.time() - start_time,
            }
        
        # 关闭连接
        client.close()
        
        duration = time.time() - start_time
        stdout_lines.append(f"总耗时: {duration:.2f}秒")
        
        return {
            "ok": True,
            "exit_code": 0,
            "programmer": "esp32",
            "esp32_host": esp32_host,
            "esp32_port": esp32_port,
            "hex_file": str(firmware_path),
            "device_id": device_id,
            "stdout": "\n".join(stdout_lines),
            "stderr": "\n".join(stderr_lines),
            "duration_sec": duration,
        }
        
    except Exception as e:
        duration = time.time() - start_time
        return {
            "ok": False,
            "exit_code": -1,
            "error": f"ESP32烧录异常: {str(e)}",
            "duration_sec": duration,
        }


def _convert_hex_to_bin(hex_content: str) -> bytes:
    """将Intel HEX格式转换为二进制数据
    
    Args:
        hex_content: HEX文件内容
        
    Returns:
        二进制数据
    """
    import struct
    
    binary_data = bytearray()
    base_address = 0
    
    for line in hex_content.strip().split('\n'):
        line = line.strip()
        if not line.startswith(':'):
            continue
        
        # 解析HEX记录
        # :LLAAAATTDD...CC
        # LL = 数据长度
        # AAAA = 地址
        # TT = 记录类型
        # DD = 数据
        # CC = 校验和
        
        data_len = int(line[1:3], 16)
        address = int(line[3:7], 16)
        record_type = int(line[7:9], 16)
        
        if record_type == 0x00:  # 数据记录
            # 扩展地址处理
            full_address = base_address + address
            
            # 确保binary_data足够长
            while len(binary_data) < full_address + data_len:
                binary_data.append(0xFF)
            
            # 写入数据
            for i in range(data_len):
                byte_val = int(line[9 + i*2:11 + i*2], 16)
                binary_data[full_address + i] = byte_val
                
        elif record_type == 0x04:  # 扩展线性地址记录
            base_address = int(line[9:13], 16) << 16
            
        elif record_type == 0x01:  # 结束记录
            break
    
    return bytes(binary_data)


@mcp.tool()
def list_supported_mcus_tool() -> Dict[str, Any]:
    """列出所有支持的MCU设备
    
    Returns:
        {
            "ok": bool,
            "total": int,
            "families": list,
            "mcus": list,
        }
    """
    mcus = list_supported_mcus()
    families = get_supported_families()
    
    return {
        "ok": True,
        "total": len(mcus),
        "families": families,
        "mcus": mcus,
    }


@mcp.tool()
def discover_esp32_devices(
    subnet: str = "192.168.4",
    timeout_sec: float = 5.0,
) -> Dict[str, Any]:
    """发现本地网络中的ESP32 Bridge设备
    
    扫描指定网段发现ESP32 STM32 Bridge设备。
    ESP32默认在AP模式下创建热点 192.168.4.1。
    
    Args:
        subnet: 要扫描的网段 (默认192.168.4)
        timeout_sec: 每个IP的超时时间
        
    Returns:
        {
            "ok": bool,
            "devices": list[{"ip": str, "port": int, "version": str}],
            "scan_range": str,
            "duration_sec": float,
        }
    """
    import time
    from pathlib import Path
    
    start_time = time.time()
    
    try:
        # 尝试导入ESP32 Bridge客户端
        try:
            import sys
            sys.path.insert(0, str(Path(__file__).parent.parent / 'ESP32_STM32_Bridge' / 'scripts'))
            from esp32_bridge_client import ESP32BridgeDiscovery
        except ImportError as e:
            return {
                "ok": False,
                "error": f"ESP32 Bridge客户端不可用: {str(e)}",
                "devices": [],
                "duration_sec": time.time() - start_time,
            }
        
        # 执行发现
        devices = ESP32BridgeDiscovery.discover(timeout=timeout_sec)
        
        # 格式化结果
        formatted_devices = [
            {
                "ip": ip,
                "port": port,
                "version": version
            }
            for ip, port, version in devices
        ]
        
        return {
            "ok": True,
            "devices": formatted_devices,
            "scan_range": f"{subnet}.2-254",
            "device_count": len(formatted_devices),
            "duration_sec": time.time() - start_time,
        }
        
    except Exception as e:
        return {
            "ok": False,
            "error": f"发现过程异常: {str(e)}",
            "devices": [],
            "duration_sec": time.time() - start_time,
        }


@mcp.tool()
def check_esp32_bridge(
    host: str = "192.168.4.1",
    port: int = 4444,
) -> Dict[str, Any]:
    """检查ESP32 Bridge连接状态
    
    测试与ESP32 Bridge的连通性并获取基本信息。
    
    Args:
        host: ESP32 IP地址
        port: ESP32 TCP端口
        
    Returns:
        {
            "ok": bool,
            "connected": bool,
            "host": str,
            "port": int,
            "bridge_version": str,
            "mcu_connected": bool,
            "mcu_idcode": str,
            "latency_ms": float,
        }
    """
    import time
    from pathlib import Path
    
    start_time = time.time()
    
    try:
        # 尝试导入ESP32 Bridge客户端
        try:
            import sys
            sys.path.insert(0, str(Path(__file__).parent.parent / 'ESP32_STM32_Bridge' / 'scripts'))
            from esp32_bridge_client import ESP32BridgeClient, BridgeError
        except ImportError as e:
            return {
                "ok": False,
                "connected": False,
                "error": f"ESP32 Bridge客户端不可用: {str(e)}",
            }
        
        result = {
            "ok": True,
            "connected": False,
            "host": host,
            "port": port,
            "bridge_version": "",
            "mcu_connected": False,
            "mcu_idcode": "",
            "latency_ms": 0.0,
        }
        
        # 尝试连接
        try:
            client = ESP32BridgeClient(host, port, timeout=10.0)
            client.connect()
            result["connected"] = True
            
            # 获取版本
            result["bridge_version"] = client.get_version()
            
            # 尝试读取MCU IDCODE
            try:
                idcode = client.read_idcode()
                result["mcu_connected"] = True
                result["mcu_idcode"] = f"0x{idcode:08X}"
            except BridgeError:
                result["mcu_connected"] = False
            
            # 关闭连接
            client.close()
            
        except BridgeError as e:
            result["connected"] = False
            result["error"] = str(e)
        
        result["latency_ms"] = (time.time() - start_time) * 1000
        return result
        
    except Exception as e:
        return {
            "ok": False,
            "connected": False,
            "host": host,
            "port": port,
            "error": f"检查异常: {str(e)}",
        }


@mcp.tool()
def get_mcu_database_info() -> Dict[str, Any]:
    """获取MCU数据库信息
    
    Returns:
        {
            "ok": bool,
            "version": str,
            "supported_families": list,
            "mcu_count": int,
            "database_stats": dict,
        }
    """
    families = get_supported_families()
    
    # 统计每个系列的MCU数量
    family_counts = {}
    for mcu in STM32_MCU_DATABASE.values():
        family_name = mcu.family.value
        if family_name not in family_counts:
            family_counts[family_name] = 0
        family_counts[family_name] += 1
    
    return {
        "ok": True,
        "version": "1.0.0",
        "supported_families": [f["name"] for f in families],
        "mcu_count": len(STM32_MCU_DATABASE),
        "database_stats": {
            "total_mcus": len(STM32_MCU_DATABASE),
            "family_count": len(families),
            "family_breakdown": family_counts,
            "target_configs": list(set(FAMILY_TARGET_MAP.values())),
        }
    }


def _get_interface_config(programmer: str) -> str:
    """获取接口配置文件路径
    
    Args:
        programmer: 烧录器类型
        
    Returns:
        接口配置文件名
    """
    if programmer == "stlink":
        return "interface/stlink.cfg"
    elif programmer == "jlink":
        return "interface/jlink.cfg"
    else:
        return f"interface/{programmer}.cfg"


def main():
    mcp.run()


if __name__ == "__main__":
    main()
