"""STM32烧录器抽象基类 - 支持本地和远程烧录器统一接口

此模块定义了烧录器的抽象接口，本地OpenOCD和远程ESP32都实现此接口。
"""

from abc import ABC, abstractmethod
from typing import Optional, Dict, Any, Callable
from dataclasses import dataclass
from enum import Enum


class FlasherType(str, Enum):
    """烧录器类型"""
    LOCAL_OPENOCD = "local_openocd"
    REMOTE_ESP32 = "remote_esp32"
    DOCKER = "docker"


@dataclass
class FlashResult:
    """烧录结果数据类"""
    ok: bool
    device_id: str = ""
    message: str = ""
    stdout: str = ""
    stderr: str = ""
    duration_ms: int = 0
    error_code: str = ""
    flasher_type: FlasherType = FlasherType.LOCAL_OPENOCD


@dataclass
class MCUTargetInfo:
    """MCU目标信息"""
    connected: bool
    device_id: str = ""
    name: str = ""
    family: str = ""
    voltage: float = 0.0
    target_config: str = ""


class BaseFlasher(ABC):
    """烧录器抽象基类
    
    所有烧录器（本地OpenOCD、远程ESP32、Docker）都必须实现此接口。
    这允许上层代码统一处理不同的烧录后端。
    """
    
    def __init__(self, name: str = ""):
        self.name = name or self.__class__.__name__
        self._connected = False
    
    @property
    def flasher_type(self) -> FlasherType:
        """返回烧录器类型"""
        return FlasherType.LOCAL_OPENOCD
    
    @abstractmethod
    async def connect(self) -> bool:
        """连接烧录器
        
        Returns:
            是否连接成功
        """
        pass
    
    @abstractmethod
    async def disconnect(self):
        """断开连接"""
        pass
    
    @abstractmethod
    async def is_available(self) -> bool:
        """检查烧录器是否可用
        
        Returns:
            是否可用（设备已连接且可通信）
        """
        pass
    
    @abstractmethod
    async def detect_target(self) -> MCUTargetInfo:
        """检测目标MCU
        
        Returns:
            MCU目标信息
        """
        pass
    
    @abstractmethod
    async def flash_firmware(
        self,
        firmware_data: bytes,
        target_config: str = "",
        verify: bool = True,
        reset: bool = True,
        progress_callback: Optional[Callable[[int], None]] = None
    ) -> FlashResult:
        """烧录固件
        
        Args:
            firmware_data: 固件二进制数据
            target_config: 目标配置文件（如"stm32f1x.cfg"）
            verify: 是否验证
            reset: 烧录后是否复位
            progress_callback: 进度回调函数(percent)
            
        Returns:
            烧录结果
        """
        pass
    
    @abstractmethod
    async def reset_target(self) -> bool:
        """复位目标MCU
        
        Returns:
            是否成功
        """
        pass
    
    async def __aenter__(self):
        await self.connect()
        return self
    
    async def __aexit__(self, exc_type, exc_val, exc_tb):
        await self.disconnect()


class SerialClient(ABC):
    """串口客户端抽象基类
    
    支持本地串口和远程WebSocket串口的统一接口
    """
    
    def __init__(self):
        self._connected = False
    
    @abstractmethod
    async def connect(self, baudrate: int = 115200) -> bool:
        """连接串口"""
        pass
    
    @abstractmethod
    async def disconnect(self):
        """断开连接"""
        pass
    
    @abstractmethod
    async def write(self, data: bytes) -> int:
        """写入数据"""
        pass
    
    @abstractmethod
    async def read(self, size: int = 1, timeout: float = 1.0) -> bytes:
        """读取数据"""
        pass
    
    @abstractmethod
    async def read_until(self, terminator: bytes, timeout: float = 5.0) -> bytes:
        """读取直到终止符"""
        pass
    
    @abstractmethod
    async def set_baudrate(self, baudrate: int) -> bool:
        """设置波特率"""
        pass
    
    async def __aenter__(self):
        await self.connect()
        return self
    
    async def __aexit__(self, exc_type, exc_val, exc_tb):
        await self.disconnect()
