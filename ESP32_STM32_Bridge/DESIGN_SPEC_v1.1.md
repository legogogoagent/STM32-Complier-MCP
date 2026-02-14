# ESP32C3 Super Mini STM32 Bridge 技术规格书 v1.1

**版本**: v1.1 (最终确认版)  
**日期**: 2026-02-13  
**目标平台**: ESP32C3FN4 (ESP32C3 Super Mini)  
**开发框架**: ESP-IDF v5.1+  
**产品定位**: 局域网现场工具（多人共用，单会话独占，基础安全防护）

---

## 1. 需求概述

### 1.1 核心功能
- **无线烧录**: Agent 通过 WiFi (TCP) 控制 ESP32，对 STM32 进行 SWD 烧录
- **实时调试**: UART 串口透传 (WebSocket)，支持 Agent 实时收发数据
- **物理授权**: 必须通过物理按键触发授权窗口，防止远程误操作
- **独占会话**: 同一时刻仅允许一个 Agent 连接，其他人需等待或协商
- **大文件支持**: 流式烧录 (Stream Flash)，无固件大小上限
- **断线保护**: 烧录中断自动回滚，防止目标板“变砖”

### 1.2 约束条件
- **网络环境**: 局域网，无公网暴露，不强制 TLS
- **目标MCU**: 优先支持 STM32F1 系列 (Cortex-M3)，3.3V 系统
- **供电**: ESP32 通过 USB 供电，目标板由 ESP32 3.3V 供电 (可选)
- **安全等级**: 防止误操作 > 防止恶意攻击

---

## 2. 硬件架构

### 2.1 选型
- **主控**: ESP32-C3-FN4 (RISC-V, 4MB Flash, WiFi/BLE)
- **模块**: ESP32C3 Super Mini (超小体积)
- **接口**: SWD (烧录), UART (调试), GPIO (按键/LED)

### 2.2 引脚分配 (修正版)

| 排针 | GPIO | 功能 | 方向 | 说明 |
|------|------|------|------|------|
| **3V3** | - | 3.3V 输出 | Out | 给目标板供电 (100mA max) |
| **GND** | - | 地线 | - | 共地 |
| **D0** | GPIO2 | 系统状态 LED | Out | 绿色，表示运行/占用状态 |
| **D1** | GPIO3 | SWDIO | In/Out | SWD 数据线 |
| **D2** | GPIO4 | SWCLK | Out | SWD 时钟线 |
| **D3** | GPIO5 | NRST | Out | 目标板复位 |
| **D4** | GPIO6 | UART RX | In | 接收目标板 TX |
| **D5** | GPIO7 | UART TX | Out | 发送给目标板 RX |
| **D6** | GPIO10 | **授权按键** | In | **物理按键 (上拉)** |
| **D7** | GPIO20 | USB_CDC_TX | Out | 调试日志输出 |
| **D8** | GPIO21 | USB_CDC_RX | In | 调试指令输入 |
| **LED** | GPIO8 | WiFi 状态 LED | Out | 板载蓝色 LED |

**避坑指南**:
- ❌ **GPIO8/9**: Strapping Pins，避免外接干扰启动
- ❌ **GPIO18/19**: USB-JTAG 专用，不可用作 SWD
- ❌ **GPIO12-17**: 内部 Flash 专用

### 2.3 连接示意图

```
ESP32C3 Super Mini          STM32 Target
──────────────────          ────────────
GPIO3 (SWDIO)    <───────>  SWDIO (PA13)
GPIO4 (SWCLK)    ───────>  SWCLK (PA14)
GPIO5 (NRST)     ───────>  NRST
GPIO6 (RX)       <───────  USART1_TX (PA9)
GPIO7 (TX)       ───────>  USART1_RX (PA10)
GPIO10 (Button)  ──[KEY]── GND
3V3              ──[SW]──> VCC (3.3V)
GND              ────────  GND
```

---

## 3. 软件架构

### 3.1 核心模块图

```
┌─────────────────────────────────────────────────────────────┐
│                     Session Manager                          │
│              (状态机: DISARMED/ARMED/OWNED/BURNING)          │
├─────────────┬──────────────┬──────────────┬─────────────────┤
│  MCP Server │  Web Server  │ UART Bridge  │   SWD Driver    │
│  (TCP 4444) │  (HTTP 80)   │ (WS 8080)    │   (GPIO/RMT)    │
├─────────────┴──────────────┴──────────────┴─────────────────┤
│                     Hardware Abstraction                     │
│  [WiFi]  [mDNS]  [NVS]  [GPIO]  [RMT]  [UART]  [Security]   │
└─────────────────────────────────────────────────────────────┘
```

### 3.2 状态机 (Session Manager)

| 状态 | 说明 | 允许操作 | LED 指示 |
|------|------|----------|----------|
| **DISARMED** | 空闲，拒绝控制 | 查询 (status, version) | 灭 |
| **ARMED** | 授权窗口 (60s) | AUTH 认证 | 蓝灯快闪 |
| **OWNED** | 已被占用 | 所有操作 | 绿灯常亮 |
| **BURNING** | 烧录中 | 仅查询进度 | 蓝灯呼吸 |

**状态流转**:
1. 默认 **DISARMED**
2. 物理按键按下 → **ARMED** (开启 60s 倒计时)
3. Agent 发送 `AUTH` 成功 → **OWNED**
4. 收到 `FLASH_BEGIN` → **BURNING**
5. 烧录完成/失败 → **OWNED**
6. 收到 `DISCONNECT` 或超时 → **DISARMED**

---

## 4. 通讯协议 (MCP v2)

### 4.1 协议帧格式

采用二进制帧结构，确保数据完整性。

```c
// 帧头 (16字节)
struct mcp_header {
    uint32_t magic;      // 0x324D4350 ('MCP2')
    uint16_t version;    // 0x0002
    uint16_t type;       // 命令类型
    uint32_t seq;        // 序列号
    uint32_t length;     // Payload 长度
    uint32_t crc32;      // 帧校验 (Header+Payload)
};
```

**命令类型 (Type)**:
- `0x01` **AUTH**: 认证请求 (Response Challenge)
- `0x02` **DISCONNECT**: 释放会话
- `0x03` **STATUS**: 查询设备状态
- `0x04` **RESET**: SWD 复位目标板
- `0x10` **FLASH_BEGIN**: 开始烧录 (参数: 地址, 大小)
- `0x11` **FLASH_DATA**: 固件数据块 (流式)
- `0x12` **FLASH_END**: 结束烧录 (触发校验)
- `0x20` **UART_CONFIG**: 配置波特率
- `0x21` **UART_START**: 启动透传

### 4.2 认证流程 (Challenge-Response)

1. Agent 连接 TCP 4444
2. ESP32 发送 `AUTH_CHALLENGE` (含 16字节随机 nonce)
3. Agent 计算 `hash = HMAC-SHA256(secret, nonce)`
4. Agent 发送 `AUTH` (含 hash)
5. ESP32 验证通过 → 状态转为 **OWNED**

### 4.3 流式烧录流程

1. **FLASH_BEGIN**: 告知目标地址、总大小、页大小
2. **FLASH_DATA**: 分块发送 (建议 4KB/块)，ESP32 边收边烧
   - 收到数据 → 写入 STM32 Flash
   - 每写入一页 → 发送 `PROGRESS` 事件
3. **FLASH_END**: 发送校验请求
   - ESP32 读取 STM32 Flash 全文校验
   - 匹配 → 返回 `OK`
   - 不匹配 → 返回 `ERROR` 并**自动擦除**已烧录区域 (回滚)

---

## 5. 功能模块详解

### 5.1 SWD 驱动
- **模式**: GPIO Bit-bang (默认) 或 RMT 加速 (可选)
- **时序**: 自适应时钟 (初次连接 100kHz，成功后升至 1MHz)
- **初始化**: 使用标准的 `0xE79E` JTAG-to-SWD 切换序列
- **复位**: 支持 `Soft Reset` (SYSRESETREQ) 和 `Hard Reset` (NRST 引脚)

### 5.2 Flash 算法
- **自动识别**: 读取 `0x1FFFF7E0` 获取 Flash 容量，读取 `DBGMCU_IDCODE` 获取型号
- **页大小自适应**: 根据容量自动选择 1KB 或 2KB 页大小
- **F1 特性**: 强制 16-bit 半字写入，每次写入检查 `BSY` 位

### 5.3 UART 桥接
- **协议**: WebSocket (ws://ip:8080/uart)
- **数据流**: 双向透传，无缓冲延迟
- **控制**: 烧录期间 (BURNING 状态) 自动暂停 UART 转发，烧录完成后恢复

### 5.4 Landing Page (Web 界面)
- **地址**: http://esp32-bridge.local (mDNS) 或 IP
- **功能**:
  - **只读状态**: 显示设备状态、连接者 IP、目标板 IDCODE
  - **网络配置**: 修改 WiFi SSID/密码 (需物理按键授权)
  - **日志查看**: 实时显示系统日志
- **注意**: **不提供** Web 烧录或控制功能，强制使用 MCP

---

## 6. 安全与防护

### 6.1 物理层
- **按键授权**: 必须物理接触才能开启授权窗口
- **状态指示**: LED 明确指示当前是否被占用

### 6.2 网络层
- **mDNS**: 仅在局域网广播 `_stm32bridge._tcp`
- **TCP**: 仅开放 4444 (MCP) 和 8080 (WS)，80 (Web)
- **连接限制**: 限制最大 1 个 MCP 连接，拒绝多余连接

### 6.3 业务层
- **断线回滚**: 烧录中断自动擦除，防止残留损坏固件
- **电源保护**: (建议) 检测目标板电压，防止电源冲突

---

## 7. 开发计划 (Phase 1-6)

| 阶段 | 周期 | 任务 | 交付物 |
|------|------|------|--------|
| **Phase 1** | 3天 | 基础框架 | WiFi, mDNS, Session 状态机, 按键驱动 |
| **Phase 2** | 3天 | 协议栈 | MCP v2 二进制协议, TCP Server, Auth |
| **Phase 3** | 4天 | SWD/Flash | SWD 驱动, MCU 识别, F1 编程算法 |
| **Phase 4** | 2天 | 烧录逻辑 | 流式接收, 自动回滚, 校验机制 |
| **Phase 5** | 2天 | UART/Web | WebSocket 桥接, Landing Page |
| **Phase 6** | 2天 | 集成测试 | Python 客户端对接, 压力测试 |

---

## 8. 附录：Python 客户端用法示例

```python
# 1. 发现设备
devices = ESP32Bridge.discover()
target = devices[0]

# 2. 连接 (需先按物理按键)
client = ESP32Bridge(target.ip)
client.connect(secret="my_device_secret")

# 3. 烧录
client.flash("firmware.bin", verify=True)
# 输出:
# [INFO] Target: STM32F103CB (128KB)
# [INFO] Erasing...
# [INFO] Flashing 64KB... [=====>    ] 50%
# [INFO] Verify OK

# 4. 开启串口监视
client.start_monitor(baud=115200)
```
