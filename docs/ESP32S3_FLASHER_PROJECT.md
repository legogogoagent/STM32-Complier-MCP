# ESP32S3-STM32-Flasher

## 项目概述

一个基于ESP32S3的无线STM32烧录和串口调试服务器。通过WiFi网络提供远程SWD烧录和串口交互功能，替代传统的有线ST-Link调试器。

### 核心特性

- 🔥 **无线烧录**：通过WiFi对STM32进行SWD烧录，无需物理连接
- 🔄 **串口桥接**：WebSocket转串口，支持交互式调试
- 🌐 **双模网络**：同时支持STA模式（连接WiFi）和AP模式（自建热点）
- 🔍 **自动发现**：mDNS服务发现，即插即用
- ⚙️ **Web配置**：内置配置页面，无需刷机即可修改参数
- 💰 **低成本**：单芯片方案，成本低于¥20

### 硬件架构

```
┌─────────────────────────────────────────────────────────────┐
│                    ESP32S3 (USB OTG + WiFi)                  │
│                                                              │
│  ┌───────────────────────────────────────────────────────┐  │
│  │          Web Server (HTTP + WebSocket)                 │  │
│  │  ├─ Landing Page (WiFi配置、状态监控)                   │  │
│  │  ├─ /api/config (REST API)                            │  │
│  │  ├─ /ws/flash (烧录WebSocket)                         │  │
│  │  └─ /ws/serial (串口WebSocket)                        │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                              │
│  ┌─────────────────────┐    ┌────────────────────────────┐  │
│  │   CMSIS-DAP         │    │   UART Bridge              │  │
│  │   (SWD Master)      │    │   (ESP32 ↔ STM32)          │  │
│  └──────────┬──────────┘    └────────────┬───────────────┘  │
│             │ GPIO                        │ GPIO (UART)      │
│             │                             │                  │
│  ┌──────────▼──────────┐    ┌─────────────▼────────────┐   │
│  │  SWDIO ──[Buffer]───┼────┼────► STM32.SWDIO (PA13) │   │
│  │  SWCLK ──[Buffer]───┼────┼────► STM32.SWCLK (PA14) │   │
│  │  NRST  ──[Driver]───┼────┼────► STM32.NRST         │   │
│  │  PWR_EN ─[Switch]───┼────┼────► STM32.VCC (3.3V)   │   │
│  │                     │    │                          │   │
│  │  UART1_TX ──────────┼────┼────► STM32.RX (PA10)    │   │
│  │  UART1_RX ◄─────────┼────┼───── STM32.TX (PA9)     │   │
│  │  GND    ────────────┼────┼───── GND                │   │
│  └─────────────────────┘    └──────────────────────────┘   │
│                                                              │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  Network Stack                                        │  │
│  │  ├─ WiFi Manager (STA + AP Dual Mode)                 │  │
│  │  ├─ mDNS Responder (stm32-flasher.local)              │  │
│  │  └─ TCP/IP Stack                                      │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

## 硬件设计

### 电路原理图

```
                    ESP32S3-DevKitC
                   ┌─────────────────┐
                   │                 │
        3.3V ◄─────┤ 3.3V       GND  ├─────► GND
                   │                 │
   [EN] Reset ◄────┤ EN         GPIO0├───► Boot Mode
                   │                 │
                   │ GPIO12 (SWDIO)  ├───┬───[Buffer]──► STM32.SWDIO (PA13)
                   │ GPIO13 (SWCLK)  ├───┼───[Buffer]──► STM32.SWCLK (PA14)
                   │ GPIO14 (NRST)   ├───┼───[Driver]──► STM32.NRST
                   │ GPIO15 (PWR_EN) ├───┼───[Switch]──► STM32.VCC
                   │                 │   │
                   │ GPIO17 (UART1_TX)├───┴─────────────► STM32.RX (PA10)
                   │ GPIO18 (UART1_RX)◄─────────────────── STM32.TX (PA9)
                   │                 │
                   │ GPIO38 (LED)    ├───[330Ω]──► LED ──► GND
                   │                 │
                   │ GPIO39 (BTN)    ├───[10K]───► 3.3V
                   │                 │       └───► GND
                   └─────────────────┘
```

### 关键元件清单

| 元件 | 型号 | 数量 | 说明 |
|------|------|------|------|
| 主控 | ESP32-S3-WROOM-1 | 1 | WiFi+BLE, USB OTG |
| 电平缓冲 | 74LVC245 | 1 | SWD信号缓冲 |
| 复位驱动 | 2N7002 | 1 | 开漏NMOS驱动NRST |
| 电源开关 | SI2301 | 1 | PMOS控制STM32电源 |
| LED | 3mm 红色 | 1 | 状态指示 |
| 按键 | 轻触开关 | 1 | 配置/复位按钮 |
| 电阻 | 10KΩ | 3 | 上拉电阻 |
| 电阻 | 330Ω | 1 | LED限流 |
| 电阻 | 1KΩ | 2 | 栅极电阻 |
| 电容 | 100nF | 3 | 去耦电容 |
| 排针 | 2.54mm | 1组 | 调试接口 |

### PCB设计要点

1. **SWD信号完整性**
   - SWCLK和SWDIO走线尽量短（<10cm）
   - 串联22Ω电阻抑制振铃
   - 避免高速信号线平行

2. **电源设计**
   - ESP32S3和STM32独立供电，通过MOSFET控制
   - 添加TVS二极管保护USB端口
   - 3.3V LDO给ESP32供电，STM32通过PMOS供电

3. **布局建议**
   - ESP32天线区域净空，远离金属
   - SWD接口靠近板边，方便连接
   - LED和按键放置在易见易操作位置

## 固件架构

### 目录结构

```
ESP32S3-STM32-Flasher/
├── src/
│   ├── main.cpp              # 程序入口
│   ├── config/
│   │   ├── config_manager.h  # 配置管理
│   │   └── wifi_config.h     # WiFi配置存储
│   ├── network/
│   │   ├── wifi_manager.h    # WiFi STA/AP管理
│   │   ├── mdns_server.h     # mDNS服务发现
│   │   └── web_server.h      # HTTP/WebSocket服务器
│   ├── swd/
│   │   ├── swd_host.h        # SWD主机底层
│   │   ├── cmsis_dap.h       # CMSIS-DAP协议
│   │   └── target_flash.h    # Flash烧录算法
│   ├── serial/
│   │   └── uart_bridge.h     # 串口桥接
│   └── utils/
│       ├── hex_parser.h      # Intel HEX解析
│       └── crc32.h           # CRC校验
├── data/
│   └── www/                  # Web前端文件
│       ├── index.html
│       ├── app.js
│       └── style.css
├── platformio.ini            # PlatformIO配置
└── README.md
```

### 核心模块设计

#### 1. WiFi管理器 (WiFiManager)

```cpp
class WiFiManager {
public:
    enum Mode { STA, AP, BOTH };
    
    void begin(Mode mode = BOTH);
    bool connectSTA(const char* ssid, const char* password);
    void startAP(const char* ssid = "STM32-Flasher");
    bool isConnected();
    String getIP();
    
    // Captive Portal检测
    bool isCaptivePortalRequested();
    void handleCaptivePortal();
};
```

#### 2. Web服务器 (WebServer)

```cpp
class WebServer {
public:
    void begin(uint16_t port = 80);
    
    // HTTP端点
    void setupRoutes();
    
    // WebSocket处理
    void onFlashWebSocket(AsyncWebSocketClient* client, 
                          AwsEventType type, 
                          uint8_t* data, 
                          size_t len);
    void onSerialWebSocket(AsyncWebSocketClient* client,
                           AwsEventType type,
                           uint8_t* data,
                           size_t len);
    
    // 静态文件服务
    void serveStaticFiles();
};
```

#### 3. CMSIS-DAP实现 (CMSISDAP)

```cpp
class CMSISDAP {
public:
    // 初始化
    bool init(uint8_t swdio_pin, uint8_t swclk_pin, uint8_t nrst_pin);
    
    // SWD基础操作
    bool connect();
    bool disconnect();
    bool resetTarget();
    
    // DAP命令
    uint32_t readIDCode();
    bool readDP(uint8_t addr, uint32_t& data);
    bool writeDP(uint8_t addr, uint32_t data);
    bool readAP(uint8_t addr, uint32_t& data);
    bool writeAP(uint8_t addr, uint32_t data);
    
    // 内存访问
    bool readMemory(uint32_t addr, uint8_t* buffer, size_t len);
    bool writeMemory(uint32_t addr, const uint8_t* buffer, size_t len);
    
    // Flash操作
    bool eraseChip();
    bool programFlash(uint32_t addr, const uint8_t* data, size_t len);
    bool verifyFlash(uint32_t addr, const uint8_t* data, size_t len);
    
private:
    // SWD位操作
    void swdWriteBit(uint8_t bit);
    uint8_t swdReadBit();
    void swdWriteByte(uint8_t byte);
    uint8_t swdReadByte();
    
    // 协议操作
    bool swdRequest(uint8_t request, uint32_t* data);
    uint8_t calculateParity(uint32_t data);
};
```

#### 4. 烧录协议 (FlashProtocol)

```cpp
class FlashProtocol {
public:
    // WebSocket消息处理
    void handleMessage(AsyncWebSocketClient* client, 
                       const String& message);
    
    // 烧录流程
    void startFlash(AsyncWebSocketClient* client, 
                    const String& target_id,
                    size_t file_size);
    void receiveChunk(AsyncWebSocketClient* client,
                      uint8_t* data,
                      size_t len);
    void verifyFlash(AsyncWebSocketClient* client);
    void completeFlash(AsyncWebSocketClient* client);
    
    // 状态回调
    void sendProgress(AsyncWebSocketClient* client, 
                      uint8_t percent);
    void sendError(AsyncWebSocketClient* client,
                   const String& error);
    void sendSuccess(AsyncWebSocketClient* client);
};
```

### WebSocket协议定义

#### 烧录协议

**连接建立**
```
Client ──WebSocket──► Server: ws://<ip>/ws/flash
```

**消息格式** (JSON)

1. 开始烧录请求
```json
// Client → Server
{
  "cmd": "flash_start",
  "target": "stm32f103c8",
  "file_size": 524288,
  "verify": true
}

// Server → Client
{
  "status": "ready",
  "target_config": "stm32f1x.cfg",
  "max_chunk_size": 1024
}
```

2. 发送固件数据
```json
// Client → Server (Binary Frame)
[0x00, 0x01, 0x02, ...]  // HEX文件数据

// Server → Client
{
  "status": "chunk_received",
  "bytes_received": 1024,
  "progress": 25
}
```

3. 烧录完成
```json
// Server → Client
{
  "status": "complete",
  "device_id": "0x20036410",
  "duration_ms": 4520,
  "bytes_written": 524288
}
```

4. 错误处理
```json
// Server → Client
{
  "status": "error",
  "code": "TARGET_NOT_FOUND",
  "message": "未检测到STM32目标设备"
}
```

#### 串口协议

**连接建立**
```
Client ──WebSocket──► Server: ws://<ip>/ws/serial?baudrate=115200
```

**数据流**
```
// 双向二进制透传
Client ◄══════════════► Server ◄══════════════► STM32.UART
```

**控制命令** (Text Frame)
```json
// Client → Server
{
  "cmd": "set_baudrate",
  "value": 921600
}

{
  "cmd": "set_config",
  "data_bits": 8,
  "parity": "none",
  "stop_bits": 1
}

{
  "cmd": "break",
  "duration_ms": 100
}
```

## API接口文档

### HTTP REST API

#### 1. 获取设备状态
```http
GET /api/status

Response:
{
  "device": {
    "name": "STM32-Flasher-A1B2",
    "version": "1.0.0",
    "uptime": 3600
  },
  "network": {
    "mode": "both",
    "sta_connected": true,
    "sta_ip": "192.168.1.100",
    "ap_active": true,
    "ap_ip": "192.168.4.1"
  },
  "target": {
    "connected": true,
    "device_id": "0x20036410",
    "name": "STM32F103C8",
    "voltage": 3.3
  }
}
```

#### 2. 获取WiFi配置
```http
GET /api/wifi/config

Response:
{
  "mode": "both",
  "sta": {
    "ssid": "Office-WiFi",
    "connected": true,
    "ip": "192.168.1.100",
    "rssi": -45
  },
  "ap": {
    "ssid": "STM32-Flasher-A1B2",
    "ip": "192.168.4.1",
    "clients": 0
  }
}
```

#### 3. 设置WiFi配置
```http
POST /api/wifi/config
Content-Type: application/json

{
  "mode": "sta",
  "sta": {
    "ssid": "New-WiFi",
    "password": "password123"
  }
}

Response:
{
  "success": true,
  "message": "配置已保存，设备将重启"
}
```

#### 4. 扫描WiFi网络
```http
GET /api/wifi/scan

Response:
{
  "networks": [
    {"ssid": "Office-WiFi", "rssi": -45, "channel": 6, "secure": true},
    {"ssid": "Guest-WiFi", "rssi": -62, "channel": 11, "secure": true}
  ]
}
```

#### 5. 控制目标电源
```http
POST /api/target/power
Content-Type: application/json

{
  "action": "reset"  // "on", "off", "reset"
}

Response:
{
  "success": true,
  "target_power": true
}
```

### mDNS服务发现

**服务类型**: `_http._tcp`

**服务名称**: `stm32-flasher-<chip_id>.local`

**TXT记录**:
```
version=1.0.0
target_connected=true
device_id=0x20036410
serial_available=true
```

## Web配置界面

### 功能模块

1. **状态仪表板**
   - 连接状态指示（WiFi、目标MCU）
   - 网络信息显示（IP地址、信号强度）
   - 目标MCU信息（型号、Flash大小）

2. **WiFi配置**
   - 网络扫描和选择
   - SSID/密码输入
   - 工作模式切换（STA/AP/Both）
   - 连接测试

3. **烧录控制**
   - 固件文件选择（HEX/BIN）
   - 进度条显示
   - 日志输出窗口
   - 一键烧录按钮

4. **串口终端**
   - 波特率设置
   - 数据收发区域
   - 历史记录
   - 快捷命令按钮

5. **系统设置**
   - 设备名称修改
   - 固件OTA更新
   - 恢复出厂设置
   - 重启设备

## 开发计划

### Phase 1: 基础框架 (Week 1)
- [ ] 项目初始化和PlatformIO配置
- [ ] WiFi管理器（STA/AP双模式）
- [ ] Web服务器基础框架
- [ ] 配置存储（Preferences库）

### Phase 2: SWD核心 (Week 2)
- [ ] SWD主机驱动（GPIO位操作）
- [ ] CMSIS-DAP基础命令
- [ ] 目标检测和ID读取
- [ ] 内存读写测试

### Phase 3: Flash烧录 (Week 3)
- [ ] Flash算法框架
- [ ] STM32F1系列支持
- [ ] HEX文件解析器
- [ ] WebSocket烧录协议
- [ ] 进度反馈机制

### Phase 4: 串口功能 (Week 4)
- [ ] UART初始化
- [ ] WebSocket串口桥接
- [ ] 波特率自适应
- [ ] 流控制支持

### Phase 5: Web界面 (Week 5)
- [ ] 前端框架搭建
- [ ] 状态仪表板
- [ ] WiFi配置页面
- [ ] 烧录控制界面
- [ ] 串口终端

### Phase 6: 测试优化 (Week 6)
- [ ] 多系列MCU测试
- [ ] 稳定性测试
- [ ] 性能优化
- [ ] 文档完善

## 兼容性说明

### 支持的STM32系列

| 系列 | OpenOCD配置 | 测试状态 |
|------|-------------|----------|
| STM32F0 | stm32f0x.cfg | 计划中 |
| STM32F1 | stm32f1x.cfg | ✓ 优先支持 |
| STM32F4 | stm32f4x.cfg | 计划中 |
| STM32F7 | stm32f7x.cfg | 计划中 |
| STM32H7 | stm32h7x.cfg | 计划中 |
| STM32L4 | stm32l4x.cfg | 计划中 |

### 与现有MCP项目的关系

本项目是独立硬件设备，但设计上与现有MCP Build Server兼容：

- **协议兼容**：使用相同的WebSocket协议，MCP可直接连接
- **自动发现**：mDNS服务发现，MCP自动识别网络中的烧录器
- **功能互补**：MCP提供编译，本设备提供无线烧录

详见：[MCP项目兼容性文档](../STM32_Complier_MCP/docs/ESP32_FLASHER_INTEGRATION.md)

## 参考资源

- [ESP32-S3技术规格书](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)
- [CMSIS-DAP协议规范](https://arm-software.github.io/CMSIS_5/DAP/html/index.html)
- [SWD协议详解](https://developer.arm.com/documentation/ddi0314/h/Debug-Access-Port/Serial-Wire-Debug)
- [OpenOCD STM32 Flash算法](https://github.com/openocd-org/openocd/tree/master/contrib/loaders/flash/stm32)

## 开源协议

MIT License

---

**作者**: [你的名字]  
**创建日期**: 2026-02-12  
**版本**: v1.0.0-draft
