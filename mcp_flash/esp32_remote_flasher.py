"""ESP32远程烧录器实现 - 占位符版本

此模块为ESP32远程烧录器预留接口。
当前版本为占位符，实际功能待ESP32固件完成后实现。

Phase 1 (当前): 返回NotImplementedError，提示用户使用本地烧录
Phase 2 (未来): 实现完整的WebSocket通信
"""

import asyncio
from typing import Optional, Callable

from mcp_flash.base_flasher import (
    BaseFlasher,
    FlashResult,
    MCUTargetInfo,
    FlasherType,
    SerialClient
)


class ESP32RemoteFlasher(BaseFlasher):
    """ESP32远程烧录器
    
    通过WiFi连接ESP32S3烧录服务器，远程控制STM32。
    
    Phase 1: 占位符，提示用户功能即将推出
    Phase 2: 实现完整功能
    """
    
    def __init__(self, host: str, port: int = 80):
        super().__init__(name=f"ESP32Remote-{host}")
        self.host = host
        self.port = port
        self.base_url = f"http://{host}:{port}"
        self.ws_url = f"ws://{host}:{port}"
        self._available = False
    
    @property
    def flasher_type(self) -> FlasherType:
        return FlasherType.REMOTE_ESP32
    
    async def connect(self) -> bool:
        """Phase 1: 返回False，提示功能待实现"""
        return False
    
    async def disconnect(self):
        """Phase 1: 空实现"""
        pass
    
    async def is_available(self) -> bool:
        """Phase 1: 始终返回False"""
        return False
    
    async def detect_target(self) -> MCUTargetInfo:
        """Phase 1: 返回未连接状态"""
        return MCUTargetInfo(connected=False)
    
    async def flash_firmware(
        self,
        firmware_data: bytes,
        target_config: str = "",
        verify: bool = True,
        reset: bool = True,
        progress_callback: Optional[Callable[[int], None]] = None
    ) -> FlashResult:
        """Phase 1: 返回功能未实现错误"""
        return FlashResult(
            ok=False,
            message="ESP32远程烧录功能即将推出，请使用本地ST-Link",
            error_code="NOT_IMPLEMENTED",
            flasher_type=self.flasher_type
        )
    
    async def reset_target(self) -> bool:
        """Phase 1: 返回False"""
        return False


class ESP32SerialClient(SerialClient):
    """ESP32远程串口客户端 - 占位符"""
    
    def __init__(self, host: str, port: int = 80):
        super().__init__()
        self.host = host
        self.port = port
    
    async def connect(self, baudrate: int = 115200) -> bool:
        return False
    
    async def disconnect(self):
        pass
    
    async def write(self, data: bytes) -> int:
        return 0
    
    async def read(self, size: int = 1, timeout: float = 1.0) -> bytes:
        return b""
    
    async def read_until(self, terminator: bytes, timeout: float = 5.0) -> bytes:
        return b""
    
    async def set_baudrate(self, baudrate: int) -> bool:
        return False


class FlasherRouter:
    """烧录器路由器 - 智能选择本地或远程烧录器
    
    优先尝试本地ST-Link，失败后尝试远程ESP32
    """
    
    def __init__(
        self,
        prefer_remote: bool = False,
        remote_hosts = None
    ):
        self.prefer_remote = prefer_remote
        self.remote_hosts = remote_hosts or []
        self._local = LocalOpenOCDFlasher()
        self._remotes = [ESP32RemoteFlasher(host) for host in self.remote_hosts]
        self._selected = None
    
    async def get_flasher(self) -> Optional[BaseFlasher]:
        """获取最佳可用烧录器
        
        Returns:
            可用的烧录器，如果没有则返回None
        """
        # Phase 1: 只使用本地烧录器
        if await self._local.is_available():
            self._selected = self._local
            return self._local
        
        # 本地不可用，提示用户
        return None
    
    async def detect_all(self) -> dict:
        """检测所有可用烧录器
        
        Returns:
            检测结果统计
        """
        result = {
            "local_available": False,
            "remote_available": [],
            "recommended": "local"
        }
        
        # 检测本地
        result["local_available"] = await self._local.is_available()
        
        # Phase 2: 检测远程
        # for remote in self._remotes:
        #     if await remote.is_available():
        #         result["remote_available"].append(remote.host)
        
        return result


# 延迟导入避免循环依赖
from mcp_flash.local_flasher import LocalOpenOCDFlasher
