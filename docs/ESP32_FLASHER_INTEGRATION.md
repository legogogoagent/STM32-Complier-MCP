# ESP32S3远程烧录服务器集成指南

本文档说明如何让现有MCP Build/Flash Server兼容ESP32S3无线烧录服务器。

## 集成架构

```
┌───────────────────────────────────────────────────────────────────────┐
│                           Agent (PC/笔记本)                            │
│  ┌─────────────────────────────────────────────────────────────────┐  │
│  │  STM32DevelopmentFlow Skill                                     │  │
│  │  ├─ 修改代码                                                     │  │
│  │  ├─ Build MCP (本地Docker编译)                                  │  │
│  │  ├─ Flash Router (自动选择本地/远程)                            │  │
│  │  │   ├─ 本地ST-Link不可用 → 远程ESP32                          │  │
│  │  │   └─ 本地可用 → 本地烧录                                    │  │
│  │  ├─ Serial Router (自动选择本地/远程)                           │  │
│  │  └─ 交互式调试                                                 │  │
│  └─────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────┬─────────────────────────────────────┘
                                  │
              ┌───────────────────┼───────────────────┐
              │                   │                   │
        ┌─────▼─────┐      ┌──────▼──────┐      ┌────▼─────┐
        │  本地模式  │      │  局域网模式  │      │ 远程模式 │
        ├───────────┤      ├─────────────┤      ├──────────┤
        │ ST-Link   │      │ ESP32S3     │      │ ESP32S3  │
        │ (USB直连) │      │ (mDNS发现)  │      │ (固定IP) │
        └───────────┘      └─────────────┘      └──────────┘
                                  │
                          ┌───────┴───────┐
                          │               │
                    ┌─────▼─────┐   ┌─────▼─────┐
                    │  SWD烧录  │   │ 串口桥接  │
                    └─────┬─────┘   └─────┬─────┘
                          │               │
                    ┌─────▼───────────────▼─────┐
                    │      STM32 Target MCU     │
                    └───────────────────────────┘
```

## 协议兼容性

### 通信协议对比

| 功能 | 本地OpenOCD | ESP32远程 | 协议兼容性 |
|------|-------------|-----------|------------|
| 烧录 | OpenOCD CLI | WebSocket | ✓ 封装兼容 |
| 检测 | OpenOCD init | HTTP GET | ✓ 封装兼容 |
| 串口 | 本地设备文件 | WebSocket | ✓ 封装兼容 |
| 进度 | 文本输出 | JSON消息 | ✓ 封装兼容 |

### 统一的Python客户端接口

```python
# mcp_flash/remote_flasher_client.py

from abc import ABC, abstractmethod
from typing import Optional, BinaryIO
import asyncio

class BaseFlasher(ABC):
    """烧录器基类，本地和远程都实现此接口"""
    
    @abstractmethod
    async def connect(self) -> bool:
        """连接烧录器"""
        pass
    
    @abstractmethod
    async def detect_target(self) -> dict:
        """检测目标MCU"""
        pass
    
    @abstractmethod
    async def flash_firmware(self, 
                            firmware_data: bytes,
                            progress_callback=None) -> dict:
        """烧录固件"""
        pass
    
    @abstractmethod
    async def reset_target(self) -> bool:
        """复位目标"""
        pass
    
    @abstractmethod
    async def disconnect(self):
        """断开连接"""
        pass


class LocalOpenOCDFlasher(BaseFlasher):
    """本地OpenOCD烧录器"""
    
    def __init__(self, interface="stlink", target="stm32f1x"):
        self.interface = interface
        self.target = target
        
    async def connect(self) -> bool:
        # 检查本地ST-Link是否可用
        result = await self._run_openocd("-c", "init", "-c", "exit")
        return result.returncode == 0
    
    async def detect_target(self) -> dict:
        result = await self._run_openocd(
            "-f", f"interface/{self.interface}.cfg",
            "-f", f"target/{self.target}.cfg",
            "-c", "init",
            "-c", "exit"
        )
        # 解析输出获取device_id
        return self._parse_detection_output(result)
    
    async def flash_firmware(self, 
                            firmware_data: bytes,
                            progress_callback=None) -> dict:
        # 保存临时文件
        temp_file = "/tmp/firmware.hex"
        with open(temp_file, "wb") as f:
            f.write(firmware_data)
        
        # 执行烧录
        result = await self._run_openocd(
            "-f", f"interface/{self.interface}.cfg",
            "-f", f"target/{self.target}.cfg",
            "-c", f"program {temp_file} verify reset exit"
        )
        
        return {
            "ok": result.returncode == 0,
            "stdout": result.stdout,
            "stderr": result.stderr
        }
    
    async def _run_openocd(self, *args) -> subprocess.CompletedProcess:
        cmd = ["openocd"] + list(args)
        return await asyncio.create_subprocess_exec(
            *cmd,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE
        )


class ESP32RemoteFlasher(BaseFlasher):
    """ESP32远程烧录器客户端"""
    
    def __init__(self, host: str, port: int = 80):
        self.host = host
        self.port = port
        self.base_url = f"http://{host}:{port}"
        self.ws_url = f"ws://{host}:{port}"
        self.session = None
        
    async def connect(self) -> bool:
        """测试HTTP连接"""
        import aiohttp
        try:
            async with aiohttp.ClientSession() as session:
                async with session.get(
                    f"{self.base_url}/api/status",
                    timeout=aiohttp.ClientTimeout(total=5)
                ) as response:
                    return response.status == 200
        except Exception as e:
            print(f"连接ESP32失败: {e}")
            return False
    
    async def detect_target(self) -> dict:
        """获取目标MCU信息"""
        import aiohttp
        async with aiohttp.ClientSession() as session:
            async with session.get(
                f"{self.base_url}/api/status"
            ) as response:
                data = await response.json()
                return {
                    "connected": data["target"]["connected"],
                    "device_id": data["target"].get("device_id"),
                    "name": data["target"].get("name"),
                    "voltage": data["target"].get("voltage")
                }
    
    async def flash_firmware(self,
                            firmware_data: bytes,
                            progress_callback=None) -> dict:
        """通过WebSocket烧录固件"""
        import websockets
        import json
        
        ws_uri = f"{self.ws_url}/ws/flash"
        
        try:
            async with websockets.connect(ws_uri) as websocket:
                # 1. 发送开始烧录命令
                await websocket.send(json.dumps({
                    "cmd": "flash_start",
                    "file_size": len(firmware_data),
                    "verify": True
                }))
                
                # 2. 等待准备就绪
                response = await websocket.recv()
                status = json.loads(response)
                
                if status["status"] != "ready":
                    return {"ok": False, "error": status.get("message", "准备失败")}
                
                # 3. 分块发送固件数据
                chunk_size = status.get("max_chunk_size", 1024)
                total_chunks = (len(firmware_data) + chunk_size - 1) // chunk_size
                
                for i in range(total_chunks):
                    start = i * chunk_size
                    end = min(start + chunk_size, len(firmware_data))
                    chunk = firmware_data[start:end]
                    
                    # 发送二进制数据
                    await websocket.send(chunk)
                    
                    # 等待确认
                    response = await websocket.recv()
                    progress = json.loads(response)
                    
                    if progress_callback:
                        progress_callback(progress.get("progress", 0))
                    
                    if progress["status"] == "error":
                        return {"ok": False, "error": progress.get("message")}
                
                # 4. 等待烧录完成
                response = await websocket.recv()
                result = json.loads(response)
                
                return {
                    "ok": result["status"] == "complete",
                    "device_id": result.get("device_id"),
                    "duration_ms": result.get("duration_ms"),
                    "message": result.get("message")
                }
                
        except Exception as e:
            return {"ok": False, "error": str(e)}
    
    async def reset_target(self) -> bool:
        """通过HTTP API复位目标"""
        import aiohttp
        async with aiohttp.ClientSession() as session:
            async with session.post(
                f"{self.base_url}/api/target/power",
                json={"action": "reset"}
            ) as response:
                data = await response.json()
                return data.get("success", False)


class SerialBridgeClient:
    """远程串口客户端"""
    
    def __init__(self, host: str, port: int = 80):
        self.host = host
        self.port = port
        self.ws_url = f"ws://{host}:{port}"
        self.websocket = None
        self.receive_task = None
        self.message_queue = asyncio.Queue()
        
    async def connect(self, baudrate: int = 115200):
        """连接远程串口"""
        import websockets
        
        ws_uri = f"{self.ws_url}/ws/serial?baudrate={baudrate}"
        self.websocket = await websockets.connect(ws_uri)
        
        # 启动接收任务
        self.receive_task = asyncio.create_task(self._receive_loop())
        
    async def _receive_loop(self):
        """后台接收串口数据"""
        try:
            while self.websocket:
                data = await self.websocket.recv()
                if isinstance(data, bytes):
                    await self.message_queue.put(data)
                else:
                    # 控制消息
                    pass
        except websockets.exceptions.ConnectionClosed:
            pass
    
    async def write(self, data: bytes):
        """发送数据到串口"""
        if self.websocket:
            await self.websocket.send(data)
    
    async def read(self, size: int = 1, timeout: float = 1.0) -> bytes:
        """读取串口数据"""
        try:
            data = await asyncio.wait_for(
                self.message_queue.get(),
                timeout=timeout
            )
            return data[:size]
        except asyncio.TimeoutError:
            return b""
    
    async def read_until(self, terminator: bytes, timeout: float = 5.0) -> bytes:
        """读取直到终止符"""
        buffer = b""
        start_time = asyncio.get_event_loop().time()
        
        while True:
            if terminator in buffer:
                idx = buffer.index(terminator) + len(terminator)
                return buffer[:idx]
            
            if asyncio.get_event_loop().time() - start_time > timeout:
                return buffer
            
            try:
                data = await asyncio.wait_for(
                    self.message_queue.get(),
                    timeout=0.1
                )
                buffer += data
            except asyncio.TimeoutError:
                continue
    
    async def close(self):
        """关闭连接"""
        if self.receive_task:
            self.receive_task.cancel()
        if self.websocket:
            await self.websocket.close()
```

## MCP Server集成

### 更新Flash MCP Server

```python
# mcp_flash/stm32_flash_server.py

# 新增导入
from mcp_flash.remote_flasher_client import (
    LocalOpenOCDFlasher, 
    ESP32RemoteFlasher,
    SerialBridgeClient
)
from mcp_flash.mdns_discovery import MDNSDiscovery

# 全局配置
ESP32_DISCOVERY_TIMEOUT = 5  # mDNS发现超时

def _get_flasher(prefer_remote: bool = False) -> BaseFlasher:
    """获取合适的烧录器（本地或远程）"""
    
    if not prefer_remote:
        # 先尝试本地
        local = LocalOpenOCDFlasher()
        if asyncio.run(local.connect()):
            return local
    
    # 尝试发现远程ESP32
    discovery = MDNSDiscovery()
    servers = discovery.find_servers(timeout=ESP32_DISCOVERY_TIMEOUT)
    
    if servers:
        # 选择第一个可用的
        for server in servers:
            remote = ESP32RemoteFlasher(server["host"], server["port"])
            if asyncio.run(remote.connect()):
                return remote
    
    # 回退到本地（即使之前失败）
    if prefer_remote:
        local = LocalOpenOCDFlasher()
        if asyncio.run(local.connect()):
            return local
    
    raise NoFlasherAvailable("未找到可用的烧录器（本地ST-Link或远程ESP32）")


@mcp.tool()
def flash_firmware_with_fallback(
    workspace: str,
    hex_file: str = "",
    prefer_remote: bool = False,
    auto_detect: bool = True,
    target_family: str = "",
    timeout_sec: int = 120,
) -> Dict[str, Any]:
    """烧录固件（自动选择本地或远程烧录器）
    
    Args:
        workspace: 工程根目录
        hex_file: hex文件路径
        prefer_remote: 优先使用远程烧录器（ESP32）
        auto_detect: 自动检测MCU类型
        target_family: 手动指定MCU系列
        timeout_sec: 超时秒数
        
    Returns:
        烧录结果，包含使用的烧录器信息
    """
    start_time = datetime.now()
    
    try:
        # 1. 获取hex文件路径
        hex_path = _resolve_hex_path(workspace, hex_file)
        
        # 2. 选择烧录器
        flasher = _get_flasher(prefer_remote)
        flasher_type = "remote_esp32" if isinstance(flasher, ESP32RemoteFlasher) else "local_openocd"
        
        # 3. 读取固件数据
        with open(hex_path, "rb") as f:
            firmware_data = f.read()
        
        # 4. 检测MCU（如果启用）
        mcu_info = None
        if auto_detect:
            detection = asyncio.run(flasher.detect_target())
            if detection["connected"]:
                mcu_info = detection
        
        # 5. 执行烧录
        def progress_callback(percent):
            print(f"烧录进度: {percent}%")
        
        result = asyncio.run(flasher.flash_firmware(
            firmware_data,
            progress_callback=progress_callback
        ))
        
        # 6. 构建返回结果
        duration = (datetime.now() - start_time).total_seconds()
        
        return {
            "ok": result["ok"],
            "flasher_type": flasher_type,
            "flasher_host": getattr(flasher, "host", "localhost"),
            "hex_file": str(hex_path),
            "mcu_info": mcu_info,
            "device_id": result.get("device_id"),
            "duration_sec": duration,
            "stdout": result.get("stdout", ""),
            "stderr": result.get("stderr", ""),
            "message": result.get("message", "")
        }
        
    except Exception as e:
        return {
            "ok": False,
            "error": str(e),
            "error_type": type(e).__name__
        }


@mcp.tool()
def discover_remote_flashers(timeout_sec: int = 5) -> Dict[str, Any]:
    """发现局域网内的ESP32远程烧录器
    
    Args:
        timeout_sec: 发现超时秒数
        
    Returns:
        发现的设备列表
    """
    discovery = MDNSDiscovery()
    servers = discovery.find_servers(timeout=timeout_sec)
    
    # 测试每个设备的连接性
    available_servers = []
    for server in servers:
        flasher = ESP32RemoteFlasher(server["host"], server["port"])
        if asyncio.run(flasher.connect()):
            # 获取详细信息
            try:
                status = asyncio.run(flasher.detect_target())
                server["target_connected"] = status["connected"]
                server["target_name"] = status.get("name")
                server["available"] = True
            except:
                server["available"] = True
                server["target_connected"] = False
            available_servers.append(server)
    
    return {
        "ok": True,
        "total_found": len(servers),
        "available": len(available_servers),
        "servers": available_servers
    }


@mcp.tool()
def interactive_serial_session(
    host: str = "",
    baudrate: int = 115200,
    command: str = "",
    expect_response: bool = True,
    timeout_sec: float = 5.0
) -> Dict[str, Any]:
    """交互式串口会话（支持本地和远程）
    
    Args:
        host: ESP32服务器地址（空字符串使用本地串口）
        baudrate: 波特率
        command: 要发送的命令
        expect_response: 是否等待响应
        timeout_sec: 等待超时
        
    Returns:
        串口通信结果
    """
    try:
        if host:
            # 使用远程串口
            serial = SerialBridgeClient(host)
            asyncio.run(serial.connect(baudrate))
        else:
            # 使用本地串口
            serial = LocalSerialClient()
            serial.connect(baudrate)
        
        # 发送命令
        asyncio.run(serial.write(command.encode() + b'\n'))
        
        if expect_response:
            # 读取响应
            response = asyncio.run(serial.read_until(b'\n', timeout_sec))
            return {
                "ok": True,
                "sent": command,
                "received": response.decode('utf-8', errors='ignore'),
                "bytes_received": len(response)
            }
        else:
            return {
                "ok": True,
                "sent": command,
                "message": "命令已发送，未等待响应"
            }
            
    except Exception as e:
        return {
            "ok": False,
            "error": str(e)
        }
```

## mDNS服务发现

```python
# mcp_flash/mdns_discovery.py

import socket
import zeroconf
from typing import List, Dict

class MDNSDiscovery:
    """mDNS服务发现，自动查找局域网内的ESP32烧录器"""
    
    SERVICE_TYPE = "_http._tcp.local."
    SERVICE_NAME_PREFIX = "stm32-flasher"
    
    def __init__(self):
        self.zc = zeroconf.Zeroconf()
        
    def find_servers(self, timeout: float = 5.0) -> List[Dict]:
        """发现可用的ESP32烧录服务器
        
        Args:
            timeout: 发现超时秒数
            
        Returns:
            服务器信息列表
        """
        from zeroconf import ServiceBrowser, ServiceListener
        
        servers = []
        
        class MyListener(ServiceListener):
            def add_service(self, zc, type_, name):
                if name.startswith(self.SERVICE_NAME_PREFIX):
                    info = zc.get_service_info(type_, name)
                    if info:
                        servers.append({
                            "name": name,
                            "host": socket.inet_ntoa(info.addresses[0]),
                            "port": info.port,
                            "properties": {
                                k.decode(): v.decode() 
                                for k, v in info.properties.items()
                            }
                        })
            
            def remove_service(self, zc, type_, name):
                pass
            
            def update_service(self, zc, type_, name):
                pass
        
        listener = MyListener()
        browser = ServiceBrowser(self.zc, self.SERVICE_TYPE, listener)
        
        import time
        time.sleep(timeout)
        
        browser.cancel()
        
        return servers
    
    def close(self):
        self.zc.close()
```

## 使用示例

### 1. 发现远程烧录器

```python
# Agent发现局域网内的ESP32设备
result = await mcp_flash.discover_remote_flashers(timeout_sec=5)

print(f"发现 {result['total_found']} 个设备，{result['available']} 个可用")
for server in result['servers']:
    print(f"  - {server['name']}: {server['host']}:{server['port']}")
    print(f"    MCU: {server.get('target_name', '未连接')}")
```

### 2. 自动选择烧录

```python
# Agent自动选择最佳烧录方式
result = await mcp_flash.flash_firmware_with_fallback(
    workspace="/path/to/project",
    prefer_remote=False,  # 优先本地，本地不可用时使用远程
    auto_detect=True
)

if result['ok']:
    print(f"✓ 烧录成功")
    print(f"  使用烧录器: {result['flasher_type']}")
    print(f"  MCU: {result['mcu_info']['name']}")
else:
    print(f"✗ 烧录失败: {result['error']}")
```

### 3. 串口交互调试

```python
# 与远程STM32进行串口交互
result = await mcp_flash.interactive_serial_session(
    host="192.168.1.100",  # ESP32地址
    baudrate=115200,
    command="status",
    expect_response=True,
    timeout_sec=2.0
)

if result['ok']:
    print(f"发送: {result['sent']}")
    print(f"接收: {result['received']}")
```

## 配置更新

### 新增配置文件项

```yaml
# ~/.stm32-mcp/config.yaml

flash:
  # 自动发现配置
  discovery:
    enabled: true
    timeout_sec: 5
    prefer_local: true  # 本地优先还是远程优先
    
  # 本地配置
  local:
    enabled: true
    interface: "stlink"
    timeout: 120
    
  # 远程ESP32配置
  remote:
    enabled: true
    # 可选：固定配置的ESP32列表
    servers:
      - name: "lab-desk"
        host: "192.168.1.100"
        port: 80
        token: "optional-auth-token"
      - name: "home-workbench"
        host: "192.168.1.101"
        port: 80
    # 是否启用自动发现
    auto_discovery: true

serial:
  # 串口配置
  local:
    enabled: true
    default_baudrate: 115200
    
  remote:
    enabled: true
    # 串口服务发现
    auto_discovery: true
```

## 部署检查清单

### MCP项目更新

- [ ] 添加 `remote_flasher_client.py`
- [ ] 添加 `mdns_discovery.py`
- [ ] 更新 `stm32_flash_server.py` 新增工具
- [ ] 更新配置文件结构
- [ ] 添加 `websockets` 和 `zeroconf` 依赖
- [ ] 更新文档

### 测试场景

- [ ] 本地ST-Link单独工作
- [ ] ESP32单独工作
- [ ] 自动切换（拔掉本地ST-Link，自动使用远程）
- [ ] 串口本地/远程切换
- [ ] mDNS自动发现
- [ ] 固定IP配置
- [ ] 网络断开重连

## 版本规划

### MCP v0.7.0 - 远程烧录支持
- ESP32远程烧录客户端
- mDNS自动发现
- 智能路由选择

### MCP v0.8.0 - 远程串口支持
- WebSocket串口桥接
- 交互式串口会话
- 串口日志记录

### MCP v0.9.0 - 高级功能
- 多设备管理
- 烧录队列
- 批量操作

---

**创建日期**: 2026-02-12  
**兼容硬件**: ESP32S3-STM32-Flasher v1.0+  
**MCP版本要求**: v0.6.0+
