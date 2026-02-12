"""本地OpenOCD烧录器实现 - 基于ST-Link本地连接"""

import asyncio
import subprocess
import tempfile
import os
import re
from pathlib import Path
from typing import Optional, Callable

from mcp_flash.base_flasher import (
    BaseFlasher, 
    FlashResult, 
    MCUTargetInfo, 
    FlasherType,
    SerialClient
)
from mcp_flash.mcu_database import get_mcu_info


class LocalOpenOCDFlasher(BaseFlasher):
    """本地OpenOCD烧录器
    
    通过本地安装的OpenOCD和ST-Link进行烧录。
    适用于开发电脑直接连接ST-Link的场景。
    """
    
    def __init__(
        self,
        interface: str = "stlink",
        transport: str = "hla_swd",
        openocd_path: str = "openocd"
    ):
        super().__init__(name="LocalOpenOCD")
        self.interface = interface
        self.transport = transport
        self.openocd_path = openocd_path
        self._default_target = "stm32f1x"
    
    @property
    def flasher_type(self) -> FlasherType:
        return FlasherType.LOCAL_OPENOCD
    
    async def connect(self) -> bool:
        """检查OpenOCD是否可用"""
        try:
            proc = await asyncio.create_subprocess_exec(
                self.openocd_path, "--version",
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE
            )
            stdout, stderr = await asyncio.wait_for(proc.communicate(), timeout=5.0)
            return proc.returncode == 0
        except Exception:
            return False
    
    async def disconnect(self):
        """本地OpenOCD无需断开"""
        pass
    
    async def is_available(self) -> bool:
        """检查ST-Link是否连接"""
        try:
            cmd = self._build_command("init", "exit")
            proc = await asyncio.create_subprocess_exec(
                *cmd,
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE
            )
            stdout, stderr = await asyncio.wait_for(proc.communicate(), timeout=10.0)
            output = (stdout.decode() + stderr.decode()).lower()
            
            # 检查是否检测到ST-Link
            return "stlink" in output and "error" not in output
        except Exception:
            return False
    
    async def detect_target(self) -> MCUTargetInfo:
        """检测目标MCU"""
        try:
            cmd = self._build_command("init", "exit")
            proc = await asyncio.create_subprocess_exec(
                *cmd,
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE
            )
            stdout, stderr = await asyncio.wait_for(proc.communicate(), timeout=10.0)
            output = stdout.decode() + stderr.decode()
            
            # 解析设备ID
            device_id = self._parse_device_id(output)
            
            if device_id:
                # 查询MCU数据库
                mcu_info = get_mcu_info(device_id)
                if mcu_info:
                    return MCUTargetInfo(
                        connected=True,
                        device_id=device_id,
                        name=mcu_info.name,
                        family=mcu_info.family.value,
                        target_config=mcu_info.target_config
                    )
                else:
                    return MCUTargetInfo(
                        connected=True,
                        device_id=device_id,
                        name="Unknown",
                        family="Unknown"
                    )
            
            # 检查是否连接但没有识别到ID
            if "stlink" in output.lower():
                return MCUTargetInfo(connected=True)
            
            return MCUTargetInfo(connected=False)
            
        except Exception as e:
            return MCUTargetInfo(connected=False)
    
    async def flash_firmware(
        self,
        firmware_data: bytes,
        target_config: str = "",
        verify: bool = True,
        reset: bool = True,
        progress_callback: Optional[Callable[[int], None]] = None
    ) -> FlashResult:
        """烧录固件"""
        
        # 1. 保存固件到临时文件
        suffix = ".hex" if firmware_data.startswith(b':') else ".bin"
        with tempfile.NamedTemporaryFile(suffix=suffix, delete=False) as f:
            f.write(firmware_data)
            temp_path = f.name
        
        try:
            # 2. 构建program命令
            program_cmd = f"program {temp_path}"
            if verify:
                program_cmd += " verify"
            if reset:
                program_cmd += " reset"
            program_cmd += " exit"
            
            # 3. 执行烧录
            target = target_config or self._default_target
            cmd = self._build_command(program_cmd)
            
            proc = await asyncio.create_subprocess_exec(
                *cmd,
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE
            )
            
            stdout, stderr = await asyncio.wait_for(
                proc.communicate(), 
                timeout=120.0
            )
            
            output = stdout.decode() + stderr.decode()
            
            # 4. 解析结果
            device_id = self._parse_device_id(output)
            success = proc.returncode == 0 and "verified" in output.lower()
            
            if progress_callback:
                progress_callback(100 if success else 0)
            
            return FlashResult(
                ok=success,
                device_id=device_id,
                message="Flash successful" if success else "Flash failed",
                stdout=stdout.decode(),
                stderr=stderr.decode(),
                flasher_type=self.flasher_type
            )
            
        except asyncio.TimeoutError:
            return FlashResult(
                ok=False,
                message="Flash timeout",
                error_code="TIMEOUT",
                flasher_type=self.flasher_type
            )
        except Exception as e:
            return FlashResult(
                ok=False,
                message=str(e),
                error_code="EXCEPTION",
                flasher_type=self.flasher_type
            )
        finally:
            # 清理临时文件
            try:
                os.unlink(temp_path)
            except:
                pass
    
    async def reset_target(self) -> bool:
        """复位目标"""
        try:
            cmd = self._build_command("init", "reset", "exit")
            proc = await asyncio.create_subprocess_exec(
                *cmd,
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE
            )
            await asyncio.wait_for(proc.communicate(), timeout=10.0)
            return proc.returncode == 0
        except Exception:
            return False
    
    def _build_command(self, *commands: str) -> list:
        """构建OpenOCD命令"""
        cmd = [
            self.openocd_path,
            "-f", f"interface/{self.interface}.cfg",
            "-f", f"target/{self._default_target}.cfg"
        ]
        
        for command in commands:
            cmd.extend(["-c", command])
        
        return cmd
    
    def _parse_device_id(self, output: str) -> str:
        """从输出中解析设备ID"""
        match = re.search(r"device id\s*=\s*(0x[0-9a-fA-F]+)", output)
        if match:
            return match.group(1)
        return ""


class LocalSerialClient(SerialClient):
    """本地串口客户端 - 简化版（Phase 1）
    
    通过本地串口设备文件（如/dev/ttyUSB0或COM1）与STM32通信。
    Phase 2将实现完整功能。
    """
    
    def __init__(self, port: str = "", auto_detect: bool = True):
        super().__init__()
        self.port = port
        self.auto_detect = auto_detect
        self._baudrate = 115200
        self._serial = None
        self._buffer = b""
    
    async def connect(self, baudrate: int = 115200) -> bool:
        """连接串口"""
        try:
            import serial
            
            if not self.port and self.auto_detect:
                self.port = self._detect_serial_port()
            
            if not self.port:
                return False
            
            self._baudrate = baudrate
            self._serial = serial.Serial(
                port=self.port,
                baudrate=baudrate,
                timeout=0.1
            )
            self._connected = True
            return True
            
        except Exception as e:
            print(f"串口连接失败: {e}")
            return False
    
    async def disconnect(self):
        """断开连接"""
        if self._serial:
            self._serial.close()
            self._serial = None
        self._connected = False
    
    async def write(self, data: bytes) -> int:
        """写入数据"""
        if not self._connected or not self._serial:
            return 0
        
        self._serial.write(data)
        return len(data)
    
    async def read(self, size: int = 1, timeout: float = 1.0) -> bytes:
        """读取数据"""
        if not self._connected or not self._serial:
            return b""
        
        # 同步读取转异步
        loop = asyncio.get_event_loop()
        return await loop.run_in_executor(
            None, 
            lambda: self._serial.read(size)
        )
    
    async def read_until(self, terminator: bytes, timeout: float = 5.0) -> bytes:
        """读取直到终止符"""
        buffer = b""
        start_time = asyncio.get_event_loop().time()
        
        while asyncio.get_event_loop().time() - start_time < timeout:
            if terminator in buffer:
                idx = buffer.index(terminator) + len(terminator)
                return buffer[:idx]
            
            chunk = await self.read(1, timeout=0.1)
            if chunk:
                buffer += chunk
            else:
                await asyncio.sleep(0.01)
        
        return buffer
    
    async def set_baudrate(self, baudrate: int) -> bool:
        """设置波特率"""
        if self._serial:
            self._serial.baudrate = baudrate
            self._baudrate = baudrate
            return True
        return False
    
    def _detect_serial_port(self) -> str:
        """自动检测STM32串口"""
        try:
            import serial.tools.list_ports
            
            stm32_patterns = [
                "0483:5740",
                "0483:3744",
                "0483:3748",
                "0483:374b",
            ]
            
            ports = serial.tools.list_ports.comports()
            
            for port in ports:
                vid_pid = f"{port.vid:04x}:{port.pid:04x}" if port.vid and port.pid else ""
                if vid_pid in stm32_patterns:
                    return port.device
                
                desc = port.description.lower()
                if any(x in desc for x in ["stlink", "stm32", "cdc"]):
                    return port.device
            
            if ports:
                return ports[0].device
        except Exception:
            pass
        
        return ""
