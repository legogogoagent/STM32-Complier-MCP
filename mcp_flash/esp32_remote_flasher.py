"""ESP32远程烧录器实现 - 完整版

基于ESP32 STM32 Bridge项目的Python客户端实现
支持WiFi远程烧录STM32 MCU
"""

import asyncio
from typing import Optional, Callable
import sys
import os

# 添加ESP32 Bridge客户端到路径
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'ESP32_STM32_Bridge', 'scripts'))

try:
    from esp32_bridge_client import (
        ESP32BridgeClient, 
        ESP32BridgeDiscovery,
        BridgeError,
        FlashError
    )
    ESP32_CLIENT_AVAILABLE = True
except ImportError:
    ESP32_CLIENT_AVAILABLE = False

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
    使用ESP32BridgeClient进行底层通信。
    
    Attributes:
        host: ESP32 IP地址 (默认 192.168.4.1 AP模式)
        port: TCP端口 (默认 4444)
    """
    
    def __init__(self, host: str = "192.168.4.1", port: int = 4444):
        super().__init__(name=f"ESP32Remote-{host}")
        self.host = host
        self.port = port
        self._client: Optional[ESP32BridgeClient] = None
        self._available = False
        self._version = ""
    
    @property
    def flasher_type(self) -> FlasherType:
        return FlasherType.REMOTE_ESP32
    
    async def connect(self) -> bool:
        """连接到ESP32 Bridge
        
        Returns:
            是否连接成功
        """
        if not ESP32_CLIENT_AVAILABLE:
            return False
        
        try:
            self._client = ESP32BridgeClient(self.host, self.port)
            self._client.connect()
            self._version = self._client.get_version()
            self._available = True
            return True
        except Exception as e:
            self._available = False
            return False
    
    async def disconnect(self):
        """断开与ESP32的连接"""
        if self._client:
            try:
                self._client.close()
            except:
                pass
            self._client = None
            self._available = False
    
    async def is_available(self) -> bool:
        """检查ESP32是否可用
        
        Returns:
            ESP32是否在线且可连接
        """
        if not ESP32_CLIENT_AVAILABLE:
            return False
        
        # 如果已连接，直接返回
        if self._available and self._client:
            return True
        
        # 尝试连接
        try:
            return await self.connect()
        except:
            return False
    
    async def detect_target(self) -> MCUTargetInfo:
        """检测连接的MCU目标
        
        Returns:
            MCU目标信息
        """
        if not self._client:
            return MCUTargetInfo(connected=False)
        
        try:
            idcode = self._client.read_idcode()
            
            # 根据IDCODE识别MCU系列
            family = self._detect_family_from_idcode(idcode)
            
            return MCUTargetInfo(
                connected=True,
                device_id=f"0x{idcode:08X}",
                name=f"STM32 {family}",
                family=family,
                voltage=3.3,  # ESP32使用3.3V逻辑
                target_config=f"stm32{family.lower()}x.cfg"
            )
        except Exception as e:
            return MCUTargetInfo(
                connected=False,
                device_id="",
                name="",
                family="",
                voltage=0.0,
                target_config=""
            )
    
    def _detect_family_from_idcode(self, idcode: int) -> str:
        """根据IDCODE识别MCU系列
        
        Args:
            idcode: 设备IDCODE
            
        Returns:
            MCU系列名称 (F1, F4, F7, H7等)
        """
        # IDCODE格式: 0x0BBXXXXX (ST产品代码)
        # 提取系列ID (高12位)
        family_id = (idcode >> 16) & 0xFFF
        
        # ST MCU系列映射
        family_map = {
            0x410: "F1",  # STM32F10xxx (Medium-density)
            0x411: "F2",  # STM32F2xxxx
            0x413: "F4",  # STM32F40xxx/41xxx
            0x419: "F4",  # STM32F42xxx/43xxx
            0x421: "F4",  # STM32F446xx
            0x434: "F4",  # STM32F469xx/479xx
            0x451: "F7",  # STM32F7xxxx
            0x452: "F7",  # STM32F72xxx/73xxx
            0x449: "F7",  # STM32F745xx/746xx
            0x450: "H7",  # STM32H7xxxx
            0x480: "H7",  # STM32H7A3xx/B3xx
            0x440: "F0",  # STM32F05xxx
            0x444: "F0",  # STM32F03xxx
            0x445: "F0",  # STM32F04xxx
            0x448: "F0",  # STM32F07xxx
            0x442: "F0",  # STM32F09xxx
            0x412: "L1",  # STM32L1xxxx
            0x416: "L1",  # STM32L1xxx Cat.2
            0x429: "L1",  # STM32L1xxx Cat.3/4/5/6
            0x447: "L0",  # STM32L0xxxx
            0x457: "L0",  # STM32L01xxx/02xxx
            0x425: "L0",  # STM32L04xxx/05xxx
            0x470: "L4",  # STM32L4xxxx
            0x461: "L4",  # STM32L4+xxxx
            0x435: "WB",  # STM32WBxxx
            0x495: "WB",  # STM32WB5xxx
            0x466: "WL",  # STM32WLxxx
            0x497: "G0",  # STM32G0B0xx/C0xx
            0x460: "G0",  # STM32G07xxx/08xxx
            0x468: "G4",  # STM32G4xxxx
            0x469: "G4",  # STM32G4xxxx Cat.2
            0x479: "L5",  # STM32L5xxxx
            0x472: "U5",  # STM32U5xxxx
            0x482: "C0",  # STM32C0xxxx
        }
        
        return family_map.get(family_id, "Unknown")
    
    async def flash_firmware(
        self,
        firmware_data: bytes,
        target_config: str = "",
        verify: bool = True,
        reset: bool = True,
        progress_callback: Optional[Callable[[int], None]] = None
    ) -> FlashResult:
        """烧录固件到STM32
        
        Args:
            firmware_data: 固件二进制数据 (bin格式)
            target_config: 目标配置 (ESP32忽略此参数)
            verify: 是否验证 (ESP32自动验证)
            reset: 烧录后是否复位 (ESP32自动处理)
            progress_callback: 进度回调函数(percent)
            
        Returns:
            烧录结果
        """
        import time
        start_time = time.time()
        
        if not self._client:
            return FlashResult(
                ok=False,
                message="未连接到ESP32 Bridge",
                error_code="NOT_CONNECTED",
                flasher_type=self.flasher_type
            )
        
        if not ESP32_CLIENT_AVAILABLE:
            return FlashResult(
                ok=False,
                message="ESP32 Bridge客户端不可用",
                error_code="CLIENT_NOT_AVAILABLE",
                flasher_type=self.flasher_type
            )
        
        try:
            # 上报上传进度 0-50%
            if progress_callback:
                progress_callback(10)
            
            # 上传固件
            self._client.upload_firmware(firmware_data)
            
            if progress_callback:
                progress_callback(50)
            
            # 上报烧录进度 50-100%
            if progress_callback:
                progress_callback(60)
            
            # 执行烧录
            self._client.flash()
            
            if progress_callback:
                progress_callback(100)
            
            duration_ms = int((time.time() - start_time) * 1000)
            
            # 读取IDCODE作为设备标识
            device_id = ""
            try:
                idcode = self._client.read_idcode()
                device_id = f"0x{idcode:08X}"
            except:
                pass
            
            return FlashResult(
                ok=True,
                device_id=device_id,
                message=f"烧录成功 ({len(firmware_data)} bytes)",
                stdout=f"ESP32 Bridge: {self._version}",
                stderr="",
                duration_ms=duration_ms,
                error_code="",
                flasher_type=self.flasher_type
            )
            
        except FlashError as e:
            duration_ms = int((time.time() - start_time) * 1000)
            return FlashResult(
                ok=False,
                device_id="",
                message=f"烧录失败: {str(e)}",
                stdout="",
                stderr=str(e),
                duration_ms=duration_ms,
                error_code="FLASH_ERROR",
                flasher_type=self.flasher_type
            )
        except BridgeError as e:
            duration_ms = int((time.time() - start_time) * 1000)
            return FlashResult(
                ok=False,
                device_id="",
                message=f"通信错误: {str(e)}",
                stdout="",
                stderr=str(e),
                duration_ms=duration_ms,
                error_code="BRIDGE_ERROR",
                flasher_type=self.flasher_type
            )
        except Exception as e:
            duration_ms = int((time.time() - start_time) * 1000)
            return FlashResult(
                ok=False,
                device_id="",
                message=f"未知错误: {str(e)}",
                stdout="",
                stderr=str(e),
                duration_ms=duration_ms,
                error_code="UNKNOWN_ERROR",
                flasher_type=self.flasher_type
            )
    
    async def reset_target(self) -> bool:
        """复位目标MCU
        
        Returns:
            是否成功
        """
        if not self._client:
            return False
        
        try:
            self._client.reset()
            return True
        except:
            return False


class ESP32SerialClient(SerialClient):
    """ESP32远程串口客户端
    
    通过ESP32的串口桥功能访问STM32的UART输出
    """
    
    def __init__(self, host: str = "192.168.4.1", port: int = 4444):
        super().__init__()
        self.host = host
        self.port = port
        self._client: Optional[ESP32BridgeClient] = None
    
    async def connect(self, baudrate: int = 115200) -> bool:
        """连接到ESP32串口桥"""
        if not ESP32_CLIENT_AVAILABLE:
            return False
        
        try:
            self._client = ESP32BridgeClient(self.host, self.port)
            self._client.connect()
            # TODO: 发送串口桥启用命令
            self._connected = True
            return True
        except:
            return False
    
    async def disconnect(self):
        """断开连接"""
        if self._client:
            self._client.close()
            self._client = None
        self._connected = False
    
    async def write(self, data: bytes) -> int:
        """写入数据到串口"""
        # TODO: 实现串口桥写入
        return 0
    
    async def read(self, size: int = 1, timeout: float = 1.0) -> bytes:
        """从串口读取数据"""
        if not self._client:
            return b""
        return self._client.read_serial(timeout)
    
    async def read_until(self, terminator: bytes, timeout: float = 5.0) -> bytes:
        """读取直到终止符"""
        # TODO: 实现缓冲区读取
        return b""
    
    async def set_baudrate(self, baudrate: int) -> bool:
        """设置波特率"""
        # TODO: 发送波特率设置命令
        return False


# 延迟导入避免循环依赖
from mcp_flash.local_flasher import LocalOpenOCDFlasher
