"""Flasher Router - 智能选择和管理烧录器

自动选择本地或远程烧录器，提供统一的烧录器管理。
支持渐进式实现：
- Phase 1: 仅本地ST-Link
- Phase 2: 添加ESP32远程烧录
"""

import asyncio
from typing import Optional, List, Dict
from dataclasses import dataclass

from mcp_flash.base_flasher import BaseFlasher, FlasherType
from mcp_flash.local_flasher import LocalOpenOCDFlasher
from mcp_flash.esp32_remote_flasher import ESP32RemoteFlasher


@dataclass
class FlasherInfo:
    """烧录器信息"""
    name: str
    type: FlasherType
    host: str
    available: bool
    target_connected: bool = False
    target_info: dict = None


class FlasherRouter:
    """烧录器路由器
    
    管理本地和远程烧录器，自动选择最佳可用烧录器。
    
    Usage:
        router = FlasherRouter()
        
        # 获取最佳可用烧录器
        flasher = await router.get_best_flasher()
        if flasher:
            result = await flasher.flash_firmware(data)
        
        # 列出所有可用烧录器
        available = await router.list_available()
        for info in available:
            print(f"{info.name}: {'可用' if info.available else '不可用'}")
    """
    
    def __init__(
        self,
        prefer_local: bool = True,
        remote_hosts: List[str] = None,
        auto_detect: bool = True
    ):
        self.prefer_local = prefer_local
        self.remote_hosts = remote_hosts or []
        self.auto_detect = auto_detect
        
        # 初始化烧录器
        self._local = LocalOpenOCDFlasher()
        self._remotes = [
            ESP32RemoteFlasher(host) 
            for host in self.remote_hosts
        ]
        
        # 缓存检测结果
        self._cache = {}
        self._cache_ttl = 5.0  # 缓存5秒
    
    async def get_best_flasher(self) -> Optional[BaseFlasher]:
        """获取最佳可用烧录器
        
        Returns:
            可用的烧录器实例，如果没有则返回None
        """
        # Phase 1: 仅本地
        if await self._local.is_available():
            return self._local
        
        # Phase 2: 尝试远程（当前占位符返回False）
        if not self.prefer_local:
            for remote in self._remotes:
                if await remote.is_available():
                    return remote
        
        # 本地优先模式下，最后尝试远程
        if self.prefer_local:
            for remote in self._remotes:
                if await remote.is_available():
                    return remote
        
        return None
    
    async def get_flasher_by_type(
        self, 
        flasher_type: FlasherType
    ) -> Optional[BaseFlasher]:
        """根据类型获取烧录器
        
        Args:
            flasher_type: 烧录器类型
            
        Returns:
            对应类型的烧录器
        """
        if flasher_type == FlasherType.LOCAL_OPENOCD:
            if await self._local.is_available():
                return self._local
        elif flasher_type == FlasherType.REMOTE_ESP32:
            for remote in self._remotes:
                if await remote.is_available():
                    return remote
        
        return None
    
    async def list_all(self) -> List[FlasherInfo]:
        """列出所有烧录器及其状态
        
        Returns:
            烧录器信息列表
        """
        result = []
        
        # 本地烧录器
        local_available = await self._local.is_available()
        local_info = FlasherInfo(
            name="Local ST-Link",
            type=FlasherType.LOCAL_OPENOCD,
            host="localhost",
            available=local_available
        )
        
        if local_available:
            target = await self._local.detect_target()
            local_info.target_connected = target.connected
            local_info.target_info = {
                "device_id": target.device_id,
                "name": target.name,
                "family": target.family
            }
        
        result.append(local_info)
        
        # 远程烧录器
        for remote in self._remotes:
            remote_available = await remote.is_available()
            remote_info = FlasherInfo(
                name=f"ESP32 Remote ({remote.host})",
                type=FlasherType.REMOTE_ESP32,
                host=remote.host,
                available=remote_available
            )
            result.append(remote_info)
        
        return result
    
    async def list_available(self) -> List[FlasherInfo]:
        """列出所有可用烧录器
        
        Returns:
            可用的烧录器信息列表
        """
        all_flashers = await self.list_all()
        return [f for f in all_flashers if f.available]
    
    async def detect_all_targets(self) -> Dict[str, dict]:
        """检测所有烧录器连接的目标
        
        Returns:
            烧录器名称到目标信息的映射
        """
        result = {}
        
        # 检测本地
        local_target = await self._local.detect_target()
        if local_target.connected:
            result["local"] = {
                "device_id": local_target.device_id,
                "name": local_target.name,
                "family": local_target.family,
                "voltage": local_target.voltage
            }
        
        # Phase 2: 检测远程
        # for remote in self._remotes:
        #     target = await remote.detect_target()
        #     if target.connected:
        #         result[remote.host] = {...}
        
        return result
    
    def add_remote_host(self, host: str):
        """添加远程烧录器主机
        
        Args:
            host: ESP32主机地址
        """
        remote = ESP32RemoteFlasher(host)
        self._remotes.append(remote)
    
    def remove_remote_host(self, host: str):
        """移除远程烧录器主机
        
        Args:
            host: ESP32主机地址
        """
        self._remotes = [r for r in self._remotes if r.host != host]
    
    async def health_check(self) -> Dict[str, any]:
        """健康检查
        
        Returns:
            健康状态报告
        """
        report = {
            "local_available": False,
            "remote_count": len(self._remotes),
            "remote_available": 0,
            "targets_detected": 0,
            "recommendation": ""
        }
        
        # 检查本地
        if await self._local.is_available():
            report["local_available"] = True
            target = await self._local.detect_target()
            if target.connected:
                report["targets_detected"] += 1
        
        # 检查远程
        for remote in self._remotes:
            if await remote.is_available():
                report["remote_available"] += 1
        
        # 生成建议
        if report["local_available"]:
            report["recommendation"] = "使用本地ST-Link"
        elif report["remote_available"] > 0:
            report["recommendation"] = f"使用远程ESP32 ({report['remote_available']} 个可用)"
        else:
            report["recommendation"] = "未找到可用烧录器。请检查ST-Link连接或配置远程ESP32"
        
        return report


# 全局路由器实例
_default_router = None


def get_default_router() -> FlasherRouter:
    """获取默认路由器实例"""
    global _default_router
    if _default_router is None:
        _default_router = FlasherRouter()
    return _default_router


def reset_router():
    """重置路由器（用于测试）"""
    global _default_router
    _default_router = None
