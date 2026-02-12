"""
ESP32 STM32 Bridge - Python Client Library
用于与ESP32 SWD Bridge通信，实现远程STM32烧录
"""

import socket
import time
import struct
from typing import Optional, Tuple
from dataclasses import dataclass
from enum import Enum

class BridgeError(Exception):
    """ESP32 Bridge通信错误"""
    pass

class FlashError(Exception):
    """烧录错误"""
    pass

@dataclass
class MCUInfo:
    """MCU信息"""
    idcode: int
    name: str
    flash_size: int

class ESP32BridgeClient:
    """
    ESP32 STM32 Bridge客户端
    
    使用示例：
        client = ESP32BridgeClient("192.168.4.1", 4444)
        client.connect()
        
        # 读取IDCODE
        idcode = client.read_idcode()
        print(f"MCU IDCODE: 0x{idcode:08X}")
        
        # 烧录固件
        with open("firmware.bin", "rb") as f:
            firmware = f.read()
        client.flash_firmware(firmware)
        
        client.close()
    """
    
    def __init__(self, host: str, port: int = 4444, timeout: float = 30.0):
        self.host = host
        self.port = port
        self.timeout = timeout
        self.socket: Optional[socket.socket] = None
        self._buffer = b""
    
    def connect(self) -> bool:
        """连接到ESP32 Bridge"""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.settimeout(self.timeout)
            self.socket.connect((self.host, self.port))
            
            # 等待欢迎消息
            welcome = self._read_line()
            if "ESP32-STM32-Bridge" not in welcome:
                raise BridgeError(f"Unexpected welcome message: {welcome}")
            
            return True
        except socket.error as e:
            raise BridgeError(f"Failed to connect: {e}")
    
    def close(self):
        """关闭连接"""
        if self.socket:
            self.socket.close()
            self.socket = None
    
    def _send_command(self, command: str) -> str:
        """发送命令并读取响应"""
        if not self.socket:
            raise BridgeError("Not connected")
        
        self.socket.send((command + "\n").encode())
        return self._read_line()
    
    def _read_line(self) -> str:
        """读取一行响应"""
        while b"\n" not in self._buffer:
            data = self.socket.recv(1024)
            if not data:
                raise BridgeError("Connection closed")
            self._buffer += data
        
        line, self._buffer = self._buffer.split(b"\n", 1)
        return line.decode().strip()
    
    def reset(self) -> int:
        """
        复位SWD并读取IDCODE
        
        Returns:
            MCU IDCODE
        """
        response = self._send_command("reset")
        if response.startswith("OK:"):
            # 解析 IDCODE=0xXXXXXXXX
            parts = response.split("=")
            if len(parts) == 2:
                return int(parts[1], 16)
        raise BridgeError(f"Reset failed: {response}")
    
    def read_idcode(self) -> int:
        """读取IDCODE"""
        response = self._send_command("idcode")
        if response.startswith("OK:"):
            return int(response.split(":")[1].strip(), 16)
        raise BridgeError(f"Failed to read IDCODE: {response}")
    
    def upload_firmware(self, firmware: bytes) -> bool:
        """
        上传固件到ESP32
        
        Args:
            firmware: 固件二进制数据
            
        Returns:
            上传成功返回True
        """
        response = self._send_command(f"upload {len(firmware)}")
        if not response.startswith("OK:"):
            raise BridgeError(f"Upload rejected: {response}")
        
        # 发送固件数据
        self.socket.sendall(firmware)
        
        # 等待确认
        response = self._read_line()
        if response.startswith("OK:"):
            return True
        raise BridgeError(f"Upload failed: {response}")
    
    def flash(self) -> bool:
        """
        将已上传的固件烧录到STM32
        
        Returns:
            烧录成功返回True
        """
        self.socket.settimeout(300.0)  # 烧录可能需要较长时间
        try:
            response = self._send_command("flash")
            
            # 读取所有响应直到完成
            while True:
                if response.startswith("OK:"):
                    return True
                elif response.startswith("ERROR:"):
                    raise FlashError(response)
                elif response.startswith("INFO:"):
                    print(f"  {response}")  # 打印进度信息
                
                response = self._read_line()
                
        finally:
            self.socket.settimeout(self.timeout)
    
    def flash_firmware(self, firmware: bytes, show_progress: bool = True) -> bool:
        """
        完整烧录流程：上传+烧录
        
        Args:
            firmware: 固件二进制数据
            show_progress: 是否显示进度
            
        Returns:
            烧录成功返回True
        """
        if show_progress:
            print(f"上传固件 ({len(firmware)} bytes)...")
        self.upload_firmware(firmware)
        
        if show_progress:
            print("烧录到STM32...")
        return self.flash()
    
    def get_version(self) -> str:
        """获取Bridge版本"""
        response = self._send_command("version")
        if response.startswith("OK:"):
            return response.split(":")[1].strip()
        raise BridgeError(f"Failed to get version: {response}")
    
    def enable_serial_bridge(self, baudrate: int = 115200):
        """
        启用串口透传模式
        在此模式下，所有TCP数据将直接转发到STM32 UART
        """
        # 此功能需要协议扩展，当前版本暂不支持
        pass
    
    def read_serial(self, timeout: float = 1.0) -> bytes:
        """读取STM32串口输出（如果启用了串口桥）"""
        if not self.socket:
            return b""
        
        self.socket.settimeout(timeout)
        try:
            return self.socket.recv(4096)
        except socket.timeout:
            return b""
        finally:
            self.socket.settimeout(self.timeout)


class ESP32BridgeDiscovery:
    """ESP32 Bridge自动发现"""
    
    @staticmethod
    def discover(timeout: float = 5.0) -> list:
        """
        在本地网络发现ESP32 Bridge设备
        
        Returns:
            发现的设备列表 [(ip, port, version), ...]
        """
        # 简单的扫描实现
        import socket
        
        devices = []
        base_ip = "192.168.4"  # AP模式默认网段
        
        # 扫描.2到.254
        for i in range(2, 255):
            ip = f"{base_ip}.{i}"
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(0.5)
                sock.connect((ip, 4444))
                
                # 读取欢迎消息
                sock.settimeout(2.0)
                welcome = sock.recv(256).decode()
                if "ESP32-STM32-Bridge" in welcome:
                    version = welcome.split("\n")[0].strip()
                    devices.append((ip, 4444, version))
                
                sock.close()
            except:
                pass
        
        return devices


# ============== 与Flash MCP集成 ==============

class ESP32RemoteFlasher:
    """
    符合Flasher接口的ESP32远程烧录器
    用于集成到Flash MCP
    """
    
    def __init__(self, host: str, port: int = 4444):
        self.host = host
        self.port = port
        self.client: Optional[ESP32BridgeClient] = None
    
    def connect(self) -> bool:
        """连接到远程设备"""
        self.client = ESP32BridgeClient(self.host, self.port)
        return self.client.connect()
    
    def disconnect(self):
        """断开连接"""
        if self.client:
            self.client.close()
            self.client = None
    
    def get_target_info(self) -> dict:
        """获取目标信息"""
        if not self.client:
            raise BridgeError("Not connected")
        
        idcode = self.client.read_idcode()
        version = self.client.get_version()
        
        return {
            "idcode": f"0x{idcode:08X}",
            "bridge_version": version,
            "host": self.host,
            "port": self.port
        }
    
    def flash_binary(self, binary_data: bytes, address: int = 0x08000000, 
                     verify: bool = True) -> bool:
        """
        烧录二进制数据
        
        Args:
            binary_data: 固件数据
            address: 起始地址 (目前ESP32只支持0x08000000)
            verify: 是否验证 (ESP32自动验证)
            
        Returns:
            烧录成功返回True
        """
        if not self.client:
            raise BridgeError("Not connected")
        
        return self.client.flash_firmware(binary_data)
    
    def read_idcode(self) -> int:
        """读取目标IDCODE"""
        if not self.client:
            raise BridgeError("Not connected")
        return self.client.read_idcode()
    
    def reset_target(self):
        """复位目标"""
        if self.client:
            self.client.reset()


if __name__ == "__main__":
    # 简单测试
    print("ESP32 Bridge Client Test")
    print("=" * 40)
    
    # 发现设备
    print("\n发现设备...")
    devices = ESP32BridgeDiscovery.discover()
    if devices:
        print(f"发现 {len(devices)} 个设备:")
        for ip, port, version in devices:
            print(f"  - {ip}:{port} ({version})")
    else:
        print("未发现设备")
    
    # 连接到AP模式默认地址
    host = "192.168.4.1"
    print(f"\n连接到 {host}:4444...")
    
    try:
        client = ESP32BridgeClient(host, 4444)
        client.connect()
        print("连接成功!")
        
        version = client.get_version()
        print(f"Bridge版本: {version}")
        
        idcode = client.read_idcode()
        print(f"MCU IDCODE: 0x{idcode:08X}")
        
        client.close()
        print("断开连接")
        
    except BridgeError as e:
        print(f"错误: {e}")
