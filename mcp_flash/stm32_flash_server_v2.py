"""STM32 MCP Flash Server - 更新版 v0.7.0

使用新的烧录器架构，支持本地ST-Link（Phase 1）
预留ESP32远程烧录接口（Phase 2）
"""

import os
import asyncio
from pathlib import Path
from typing import Dict, Any, Optional
from datetime import datetime

from fastmcp import FastMCP

from mcp_flash.mcu_database import (
    get_mcu_info,
    get_target_config_by_family,
    MCUFamily,
    list_supported_mcus,
    get_supported_families,
)
from mcp_flash.flasher_router import FlasherRouter, get_default_router
from mcp_flash.local_flasher import LocalOpenOCDFlasher
from mcp_flash.base_flasher import FlasherType

# 初始化MCP服务器
mcp = FastMCP("stm32-flash-server")

# 版本信息
VERSION = "0.7.0"


@mcp.tool()
async def flash_firmware(
    workspace: str,
    hex_file: str = "",
    auto_detect: bool = True,
    target_family: str = "",
    verify: bool = True,
    reset: bool = True,
    timeout_sec: int = 120,
    prefer_local: bool = True,
) -> Dict[str, Any]:
    """烧录固件到STM32 MCU（新版统一接口）
    
    自动选择本地或远程烧录器，支持MCU自动检测。
    
    Args:
        workspace: 工程根目录绝对路径
        hex_file: hex文件路径（相对于workspace/out/，或绝对路径）
        auto_detect: 是否自动检测MCU类型
        target_family: 目标MCU系列（如"F4"），auto_detect=False时使用
        verify: 是否验证烧录
        reset: 烧录后是否复位
        timeout_sec: 烧录超时秒数
        prefer_local: 是否优先使用本地烧录器
        
    Returns:
        {
            "ok": bool,
            "flasher_type": str,
            "flasher_name": str,
            "device_id": str,
            "mcu_info": dict,
            "duration_sec": float,
            "message": str,
        }
    """
    start_time = datetime.now()
    
    try:
        # 1. 解析hex文件路径
        hex_path = _resolve_hex_path(workspace, hex_file)
        if not hex_path:
            return {"ok": False, "error": "未找到hex文件"}
        
        # 2. 读取固件数据
        with open(hex_path, "rb") as f:
            firmware_data = f.read()
        
        # 3. 获取烧录器
        router = FlasherRouter(prefer_local=prefer_local)
        flasher = await router.get_best_flasher()
        
        if not flasher:
            return {
                "ok": False,
                "error": "未找到可用的烧录器。请检查ST-Link连接",
                "error_code": "NO_FLASHER_AVAILABLE"
            }
        
        # 4. 检测MCU（如果启用）
        target_config = ""
        mcu_info = None
        
        if auto_detect:
            target_info = await flasher.detect_target()
            if target_info.connected:
                target_config = target_info.target_config
                mcu_info = {
                    "name": target_info.name,
                    "family": target_info.family,
                    "device_id": target_info.device_id,
                }
        elif target_family:
            # 手动指定系列
            try:
                family_enum = MCUFamily(f"STM32{target_family.upper()}")
                target_config = get_target_config_by_family(family_enum)
            except ValueError:
                target_config = f"stm32{target_family.lower()}x.cfg"
        
        # 5. 执行烧录
        def progress_callback(percent):
            print(f"  烧录进度: {percent}%")
        
        result = await flasher.flash_firmware(
            firmware_data=firmware_data,
            target_config=target_config,
            verify=verify,
            reset=reset,
            progress_callback=progress_callback
        )
        
        # 6. 构建返回结果
        duration = (datetime.now() - start_time).total_seconds()
        
        return {
            "ok": result.ok,
            "flasher_type": result.flasher_type.value,
            "flasher_name": flasher.name,
            "device_id": result.device_id or (mcu_info.get("device_id") if mcu_info else ""),
            "mcu_info": mcu_info,
            "duration_sec": duration,
            "message": result.message,
            "stdout": result.stdout[:500] if result.stdout else "",
            "stderr": result.stderr[:500] if result.stderr else "",
        }
        
    except Exception as e:
        return {
            "ok": False,
            "error": str(e),
            "error_type": type(e).__name__,
        }


@mcp.tool()
async def detect_mcu(
    flasher_type: str = "auto",
    timeout_sec: int = 10,
) -> Dict[str, Any]:
    """检测连接的MCU
    
    Args:
        flasher_type: 烧录器类型 ("auto", "local", "remote")
        timeout_sec: 检测超时秒数
        
    Returns:
        {
            "ok": bool,
            "detected": bool,
            "device_id": str,
            "mcu_info": dict,
            "flasher_type": str,
            "message": str,
        }
    """
    try:
        router = FlasherRouter()
        
        # 选择烧录器
        if flasher_type == "local":
            flasher = await router.get_flasher_by_type(FlasherType.LOCAL_OPENOCD)
        elif flasher_type == "remote":
            flasher = await router.get_flasher_by_type(FlasherType.REMOTE_ESP32)
        else:
            flasher = await router.get_best_flasher()
        
        if not flasher:
            return {
                "ok": False,
                "detected": False,
                "message": "未找到可用的烧录器",
            }
        
        # 检测目标
        target = await flasher.detect_target()
        
        if not target.connected:
            return {
                "ok": True,
                "detected": False,
                "flasher_type": flasher.flasher_type.value,
                "message": "未检测到MCU，请检查连接",
            }
        
        # 查询MCU数据库
        mcu_db_info = None
        if target.device_id:
            db_info = get_mcu_info(target.device_id)
            if db_info:
                mcu_db_info = {
                    "name": db_info.name,
                    "family": db_info.family.value,
                    "flash_kb": db_info.flash_size_kb,
                    "ram_kb": db_info.ram_size_kb,
                    "core": db_info.core,
                    "description": db_info.description,
                }
        
        return {
            "ok": True,
            "detected": True,
            "device_id": target.device_id,
            "name": target.name or (mcu_db_info["name"] if mcu_db_info else "Unknown"),
            "family": target.family or (mcu_db_info["family"] if mcu_db_info else "Unknown"),
            "target_config": target.target_config,
            "mcu_info": mcu_db_info,
            "flasher_type": flasher.flasher_type.value,
            "message": f"检测到MCU: {target.name or target.device_id}",
        }
        
    except Exception as e:
        return {
            "ok": False,
            "error": str(e),
        }


@mcp.tool()
async def list_flashers() -> Dict[str, Any]:
    """列出所有可用的烧录器
    
    Returns:
        {
            "ok": bool,
            "total": int,
            "available": int,
            "flashers": list,
        }
    """
    try:
        router = FlasherRouter()
        all_flashers = await router.list_all()
        available = [f for f in all_flashers if f.available]
        
        return {
            "ok": True,
            "total": len(all_flashers),
            "available": len(available),
            "flashers": [
                {
                    "name": f.name,
                    "type": f.type.value,
                    "host": f.host,
                    "available": f.available,
                    "target_connected": f.target_connected,
                    "target_info": f.target_info,
                }
                for f in all_flashers
            ],
        }
        
    except Exception as e:
        return {
            "ok": False,
            "error": str(e),
        }


@mcp.tool()
async def health_check() -> Dict[str, Any]:
    """执行健康检查
    
    Returns:
        {
            "ok": bool,
            "status": str,
            "local_available": bool,
            "remote_available": int,
            "targets_detected": int,
            "recommendation": str,
        }
    """
    try:
        router = FlasherRouter()
        report = await router.health_check()
        
        return {
            "ok": True,
            "status": "healthy" if report["local_available"] or report["remote_available"] > 0 else "no_flasher",
            **report
        }
        
    except Exception as e:
        return {
            "ok": False,
            "error": str(e),
        }


@mcp.tool()
async def list_supported_mcus_tool() -> Dict[str, Any]:
    """列出所有支持的MCU"""
    mcus = list_supported_mcus()
    families = get_supported_families()
    
    return {
        "ok": True,
        "total": len(mcus),
        "families": families,
        "mcus": mcus,
    }


@mcp.tool()
def get_flash_info() -> Dict[str, Any]:
    """获取Flash服务器信息"""
    return {
        "name": "stm32-flash-server",
        "version": VERSION,
        "architecture": "unified",
        "phases": {
            "local_stlink": "✅ 已完成",
            "remote_esp32": "⏳ Phase 2",
            "serial_bridge": "⏳ Phase 2",
        },
        "supported_features": [
            "本地ST-Link烧录",
            "MCU自动检测",
            "多目标支持",
            "统一接口",
        ],
        "upcoming_features": [
            "ESP32远程烧录",
            "WebSocket串口",
            "mDNS自动发现",
        ]
    }


def _resolve_hex_path(workspace: str, hex_file: str) -> Optional[Path]:
    """解析hex文件路径"""
    workspace_path = Path(workspace).resolve()
    
    if not hex_file:
        # 自动查找
        out_dir = workspace_path / "out" / "artifacts"
        hex_files = list(out_dir.glob("*.hex"))
        if hex_files:
            return hex_files[0]
        return None
    
    if os.path.isabs(hex_file):
        path = Path(hex_file)
    else:
        path = workspace_path / "out" / "artifacts" / hex_file
        if not path.exists():
            path = workspace_path / hex_file
    
    return path if path.exists() else None


def main():
    mcp.run()


if __name__ == "__main__":
    main()
