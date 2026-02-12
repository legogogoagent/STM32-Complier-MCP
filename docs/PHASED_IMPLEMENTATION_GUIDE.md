# MCP项目渐进式实现指南

## 概述

本项目采用**渐进式实现策略**，分阶段支持本地ST-Link烧录和远程ESP32烧录。

## 架构图

```
当前状态 (Phase 1)
==================
┌──────────────────────────────────────────────┐
│              Agent (PC/笔记本)                │
│  ┌────────────────────────────────────────┐  │
│  │  Flash MCP Server (v0.7.0)             │  │
│  │  ├─ FlasherRouter                      │  │
│  │  │   └─ LocalOpenOCDFlasher  ◄─────────┼──┤  ✓ 已实现
│  │  └─ ESP32RemoteFlasher (占位符)         │  │  ⏳ Phase 2
│  └────────────────────────────────────────┘  │
└──────────────────────┬───────────────────────┘
                       │ USB
                       ▼
              ┌──────────────────┐
              │   ST-Link/V2     │
              │   (本地连接)      │
              └────────┬─────────┘
                       │ SWD
                       ▼
              ┌──────────────────┐
              │   STM32 MCU      │
              └──────────────────┘

未来 (Phase 2)
==============
┌──────────────────────────────────────────────┐
│              Agent (PC/笔记本)                │
│  ┌────────────────────────────────────────┐  │
│  │  Flash MCP Server (v0.8.0+)            │  │
│  │  ├─ FlasherRouter                      │  │
│  │  │   ├─ LocalOpenOCDFlasher            │  │
│  │  │   └─ ESP32RemoteFlasher  ◄──────────┼──┤  ⏳ 待实现
│  └────────────────────────────────────────┘  │
└──────────────────────┬───────────────────────┘
                       │ WiFi
                       ▼
              ┌──────────────────┐
              │   ESP32S3        │
              │   (无线烧录服务器) │
              └────────┬─────────┘
                       │ SWD (GPIO)
                       ▼
              ┌──────────────────┐
              │   STM32 MCU      │
              └──────────────────┘
```

## Phase 1: 本地ST-Link（当前 ✅）

### 已实现功能

1. **统一烧录器接口** (`base_flasher.py`)
   - `BaseFlasher` 抽象基类
   - `FlashResult` 结果数据类
   - `MCUTargetInfo` 目标信息类
   - `SerialClient` 串口客户端接口

2. **本地OpenOCD烧录器** (`local_flasher.py`)
   - `LocalOpenOCDFlasher` 实现
   - ST-Link检测和连接
   - MCU自动检测
   - Flash烧录和验证
   - 本地串口客户端（简化版）

3. **烧录器路由器** (`flasher_router.py`)
   - `FlasherRouter` 智能路由
   - 自动选择最佳烧录器
   - 健康检查和状态报告

4. **新版MCP Server** (`stm32_flash_server_v2.py`)
   - 统一接口的 `flash_firmware()`
   - `detect_mcu()` 检测工具
   - `list_flashers()` 烧录器列表
   - `health_check()` 健康检查

### 使用示例

```python
# 获取Flash服务器信息
info = await mcp_flash.get_flash_info()
print(f"版本: {info['version']}")
# 输出: 版本: 0.7.0
#       phases.local_stlink: ✅ 已完成
#       phases.remote_esp32: ⏳ Phase 2

# 健康检查
health = await mcp_flash.health_check()
if health['local_available']:
    print("✓ 本地ST-Link可用")
else:
    print("✗ 请检查ST-Link连接")

# 烧录固件（自动选择烧录器）
result = await mcp_flash.flash_firmware(
    workspace="/path/to/project",
    auto_detect=True  # 自动检测MCU
)

if result['ok']:
    print(f"✓ 烧录成功")
    print(f"  使用烧录器: {result['flasher_name']}")
    print(f"  MCU: {result['mcu_info']['name']}")
    print(f"  耗时: {result['duration_sec']:.2f}秒")
```

### 文件结构

```
mcp_flash/
├── __init__.py
├── stm32_flash_server.py      # 旧版（保持兼容）
├── stm32_flash_server_v2.py   # 新版（Phase 1）✨
├── base_flasher.py            # 统一接口 ✅
├── local_flasher.py           # 本地烧录器 ✅
├── esp32_remote_flasher.py    # 远程烧录器（占位符）⏳
├── flasher_router.py          # 智能路由 ✅
├── mcu_database.py            # MCU数据库 ✅
└── ...
```

## Phase 2: ESP32远程烧录（未来）

### 待实现功能

1. **ESP32固件** (独立项目)
   - Web服务器（Landing Page配置）
   - CMSIS-DAP协议（SWD over GPIO）
   - WebSocket烧录接口
   - 串口桥接（WebSocket转UART）
   - mDNS服务发现

2. **MCP客户端扩展**
   - `ESP32RemoteFlasher` 完整实现
   - WebSocket通信
   - 远程串口客户端
   - mDNS自动发现

3. **增强路由器**
   - 自动发现远程烧录器
   - 负载均衡（多远程烧录器）
   - 故障转移

### 使用示例（Phase 2）

```python
# 自动发现远程烧录器
flashers = await mcp_flash.list_flashers()
for flasher in flashers['flashers']:
    print(f"- {flasher['name']}: {'可用' if flasher['available'] else '不可用'}")
    if flasher['target_connected']:
        print(f"  MCU: {flasher['target_info']['name']}")

# 输出:
# - Local ST-Link: 可用
#   MCU: STM32F103C8
# - ESP32 Remote (192.168.1.100): 可用
#   MCU: STM32F407VG
# - ESP32 Remote (192.168.1.101): 不可用

# 选择远程烧录器
result = await mcp_flash.flash_firmware(
    workspace="/path/to/project",
    prefer_local=False  # 优先使用远程
)

# 远程串口交互
serial = await mcp_flash.open_serial(
    host="192.168.1.100",
    baudrate=115200
)
await serial.write(b"status\n")
response = await serial.read_until(b"\n")
```

## 渐进式迁移指南

### 从旧版迁移到新版

#### 旧版代码
```python
# stm32_flash_server.py (旧版)
result = await mcp_flash.flash_firmware(
    workspace="/path/to/project",
    programmer="stlink",  # 指定烧录器
    interface="swd",
    timeout_sec=120
)
```

#### 新版代码（Phase 1）
```python
# stm32_flash_server_v2.py (新版)
result = await mcp_flash.flash_firmware(
    workspace="/path/to/project",
    auto_detect=True,        # 自动检测MCU
    prefer_local=True,       # 优先本地（默认）
    timeout_sec=120
)
# 返回更丰富的信息
print(result['flasher_type'])  # "local_openocd"
print(result['flasher_name'])  # "LocalOpenOCD"
print(result['mcu_info'])      # {name, family, device_id}
```

### 兼容性说明

- **Phase 1**: 新旧版本可以共存
- **Phase 2**: 旧版接口将标记为deprecated
- **Phase 3**: 移除旧版接口（未来计划）

## 开发计划

### Phase 1 (当前) ✅
- [x] 统一烧录器接口设计
- [x] 本地OpenOCD烧录器实现
- [x] 烧录器路由器
- [x] 新版MCP Server
- [x] 健康检查工具
- [ ] 完整测试覆盖
- [ ] 文档完善

### Phase 2 (未来) ⏳
- [ ] ESP32S3固件开发
- [ ] ESP32远程烧录器实现
- [ ] mDNS服务发现
- [ ] WebSocket串口
- [ ] 配置界面
- [ ] 集成测试

### Phase 3 (未来) 📅
- [ ] 多烧录器管理
- [ ] 负载均衡
- [ ] 远程OTA更新
- [ ] 高级调试功能

## 测试检查清单

### Phase 1 测试

#### 本地ST-Link测试
- [ ] ST-Link检测
- [ ] MCU自动检测
- [ ] Flash烧录
- [ ] 验证功能
- [ ] 复位功能
- [ ] 错误处理

#### 路由器测试
- [ ] 获取最佳烧录器
- [ ] 列出所有烧录器
- [ ] 健康检查
- [ ] 本地优先策略

#### MCP工具测试
- [ ] flash_firmware
- [ ] detect_mcu
- [ ] list_flashers
- [ ] health_check
- [ ] get_flash_info

### Phase 2 测试（未来）

#### ESP32固件测试
- [ ] Web服务器
- [ ] WiFi配置
- [ ] SWD通信
- [ ] Flash烧录
- [ ] 串口桥接

#### 远程通信测试
- [ ] WebSocket连接
- [ ] 固件传输
- [ ] 进度反馈
- [ ] 串口交互

#### 集成测试
- [ ] 自动发现
- [ ] 故障转移
- [ ] 多烧录器切换

## 配置示例

### Phase 1 配置

```yaml
# ~/.stm32-mcp/config.yaml (Phase 1)
flash:
  prefer_local: true
  local:
    interface: "stlink"
    timeout: 120
  remote:
    enabled: false  # Phase 2启用
```

### Phase 2 配置

```yaml
# ~/.stm32-mcp/config.yaml (Phase 2)
flash:
  prefer_local: true
  local:
    interface: "stlink"
    timeout: 120
  remote:
    enabled: true
    auto_discovery: true
    servers:
      - host: "192.168.1.100"
        name: "desk-esp32"
      - host: "192.168.1.101"
        name: "lab-esp32"
    mdns:
      enabled: true
      service: "_stm32-flash._tcp"
```

## 升级路径

### 从Phase 1升级到Phase 2

1. **部署ESP32固件**
   ```bash
   # 烧录ESP32固件
   esptool.py write_flash 0x0 esp32_firmware.bin
   ```

2. **配置WiFi**
   - 连接ESP32的AP
   - 访问 http://192.168.4.1
   - 配置WiFi SSID/密码

3. **更新MCP配置**
   ```yaml
   # 启用远程烧录
   remote:
     enabled: true
     auto_discovery: true
   ```

4. **验证连接**
   ```bash
   # 健康检查
   python -c "
   from mcp_flash.flasher_router import FlasherRouter
   import asyncio
   
   router = FlasherRouter()
   report = asyncio.run(router.health_check())
   print(report)
   "
   ```

## 故障排查

### Phase 1常见问题

#### 本地ST-Link不可用
```bash
# 检查OpenOCD
openocd --version

# 检查ST-Link
lsusb | grep ST-Link

# 手动检测
openocd -f interface/stlink.cfg -c "init" -c "exit"
```

#### 烧录失败
```python
# 查看详细错误
result = await mcp_flash.flash_firmware(...)
if not result['ok']:
    print(result['stdout'])
    print(result['stderr'])
```

### Phase 2常见问题（未来）

#### ESP32无法发现
```bash
# 检查mDNS
avahi-browse -r _http._tcp

# 直接访问IP
curl http://192.168.1.100/api/status
```

#### WebSocket连接失败
```python
# 测试连接
import websockets
async with websockets.connect("ws://192.168.1.100/ws/flash") as ws:
    await ws.send('{"cmd": "ping"}')
    response = await ws.recv()
    print(response)
```

## 参考文档

- [ESP32S3烧录服务器项目](../ESP32S3_FLASHER_PROJECT.md)
- [MCP与ESP32集成指南](../ESP32_FLASHER_INTEGRATION.md)
- [多目标支持指南](../MULTI_TARGET_GUIDE.md)

---

**当前版本**: Phase 1 (v0.7.0)  
**状态**: ✅ 本地ST-Link功能已完成  
**下一步**: Phase 2 ESP32远程烧录（待开发）
