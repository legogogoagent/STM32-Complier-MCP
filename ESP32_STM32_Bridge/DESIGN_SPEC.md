# ESP32C3 Super Mini STM32 Bridge 技术规格文档

**版本**: v1.0  
**日期**: 2026-02-13  
**目标**: ESP32C3FN4 (ESP32C3 Super Mini)  
**IDE**: ESP-IDF v5.1+  

---

## 1. 项目概述

### 1.1 项目目标
创建一个基于 ESP32C3 Super Mini 的 STM32 烧录调试桥接器，实现以下功能：
- 通过 WiFi TCP 接收 MCP 命令，控制 STM32 的 SWD 接口
- 实现 STM32F1 系列 Flash 编程算法
- 提供实时 UART 串口透传功能 (WebSocket)
- 提供 Web 配置界面 (Landing Page)
- 通过 mDNS 实现服务发现
- 多层安全机制防止未授权访问

### 1.2 硬件平台
- **主控**: ESP32C3 Super Mini (ESP32C3FN4)
- **目标**: STM32F1 系列 (3.3V)
- **连接**: SWD + UART

### 1.3 技术栈
- **SDK**: ESP-IDF v5.1+
- **语言**: C
- **网络**: WiFi STA/AP, mDNS, TCP Server, WebSocket, HTTP Server
- **存储**: NVS (Non-Volatile Storage)
- **安全**: SHA256, HTTP Basic Auth, Token Authentication

---

## 2. 硬件设计

### 2.1 引脚分配

ESP32C3 Super Mini 排针定义：

| 排针 | GPIO | 功能 | 方向 | 连接对象 | 备注 |
|------|------|------|------|----------|------|
| 3V3 | - | 供电 | - | - | 3.3V 输入 |
| GND | - | 地线 | - | STM32 GND | - |
| D0 | GPIO2 | 系统LED/状态 | 输出 | LED (可选) | Strapping Pin |
| D1 | GPIO3 | SWDIO | 双向 | STM32 SWDIO | - |
| D2 | GPIO4 | SWCLK | 输出 | STM32 SWCLK | - |
| D3 | GPIO5 | NRST | 输出 | STM32 NRST | 硬件复位 |
| D4 | GPIO6 | UART RX | 输入 | STM32 USART_TX | Debug 接收 |
| D5 | GPIO7 | UART TX | 输出 | STM32 USART_RX | Debug 发送 |
| D6 | GPIO10 | 运行LED | 输出 | LED (可选) | - |
| D7 | GPIO20 | USB_CDC_TX | 输出 | USB Serial | Debug 输出 |
| D8 | GPIO21 | USB_CDC_RX | 输入 | USB Serial | Debug 输入 |
| LED | GPIO8 | WiFi状态LED | 输出 | 板载LED | 蓝色 |

### 2.2 硬件连接图

```
ESP32C3 Super Mini          STM32 Target
──────────────────          ────────────
GPIO3 (SWDIO)    <───────>  SWDIO
GPIO4 (SWCLK)    ───────>  SWCLK  
GPIO5 (NRST)     ───────>  NRST
GPIO6 (UART RX)  <───────  USART_TX (PA9)
GPIO7 (UART TX)  ───────>  USART_RX (PA10)
GND              ────────  GND
3V3              ────────  3.3V (可选)
```

### 2.3 ESP32C3 限制说明

**不可用引脚**:
- GPIO12-17: 内部连接 Flash，不可用
- GPIO18-19: USB-JTAG 专用，不可用 (SWD 位操作)
- GPIO9: Strapping Pin (启动模式)，避免使用

**可用引脚**:
- GPIO0-7, GPIO10, GPIO20-21: 自由使用
- GPIO2: Strapping Pin，但可用作输出 (LED)

---

## 3. 软件架构

### 3.1 项目结构

```
esp32c3_stm32_bridge/
├── CMakeLists.txt              # 项目配置
├── sdkconfig.defaults          # 默认 sdkconfig
├── partitions.csv              # 分区表
│
├── main/
│   ├── CMakeLists.txt
│   ├── main.c                  # 入口 + 任务创建
│   ├── bridge_config.h         # 全局配置宏
│   │
│   ├── wifi_manager.c/h        # WiFi STA/AP + Captive Portal
│   ├── mdns_service.c/h        # mDNS 服务发现
│   ├── http_server.c/h         # HTTP Server + Landing Page
│   │
│   ├── mcp_server.c/h          # MCP TCP 服务器 (端口 4444)
│   ├── mcp_commands.c/h        # MCP 命令实现
│   │
│   ├── swd_driver.c/h          # SWD RMT 驱动
│   ├── flash_writer.c/h        # Flash 编程算法
│   │
│   ├── uart_bridge.c/h         # UART + WebSocket 桥接
│   │
│   ├── security.c/h            # 安全: Token/密码/认证
│   ├── config_storage.c/h      # NVS 配置读写
│   └── utils.c/h               # 工具函数
│
├── components/                 # 第三方组件
│   └── cJSON/                  # JSON 解析
│
└── html/                       # Landing Page (嵌入 SPIFFS)
    ├── index.html
    ├── style.css
    └── app.js
```

### 3.2 任务划分

| 任务名 | 优先级 | 堆栈 | 功能 |
|--------|--------|------|------|
| main | 5 | 4096 | 初始化，创建其他任务 |
| wifi_mgr | 3 | 4096 | WiFi 管理，AP/STA 切换 |
| mdns_svc | 2 | 2048 | mDNS 服务 |
| http_srv | 3 | 8192 | HTTP Server + WebSocket |
| mcp_srv | 4 | 8192 | MCP TCP 服务器 |
| swd_drv | 5 | 4096 | SWD 时序控制 (RMT) |
| uart_bridge | 3 | 4096 | UART 数据收发 + WebSocket |
| led_blink | 1 | 1024 | 状态 LED 闪烁 |

### 3.3 内存分配

```c
// 缓冲区大小定义 (bridge_config.h)
#define FIRMWARE_BUF_SIZE       (128 * 1024)    // 128KB 固件缓冲
#define UART_BUF_SIZE           1024            // UART 环形缓冲
#define MCP_RX_BUF_SIZE         256             // MCP 命令缓冲
#define MCP_TX_BUF_SIZE         4096            // MCP 响应缓冲
#define WS_BUF_SIZE             2048            // WebSocket 缓冲
```

---

## 4. 网络服务

### 4.1 WiFi 模式

**首次启动**:
- 模式: AP (Access Point)
- SSID: `ESP32-Bridge-Setup`
- 密码: `stm32bridge` (固定)
- IP: 192.168.4.1
- 功能: Captive Portal，强制配置 WiFi

**正常工作**:
- 模式: STA (Station)
- 连接用户配置的 WiFi
- 通过 DHCP 获取 IP
- 如果连接失败，回退到 AP 模式

### 4.2 mDNS 服务发现

**服务配置**:
```
主机名: esp32-bridge-<mac后4位>.local
        例如: esp32-bridge-a1b2.local

服务名: _stm32bridge._tcp
端口: 4444 (MCP)

TXT 记录:
- version=1.0.0
- chip=esp32c3
- mac=a1:b2:c3:d4:e5:f6
- uart_ws_port=8080
- secure=true/false
```

**ESP-IDF 实现**:
```c
// mdns_service.h
esp_err_t mdns_init_service(void);
esp_err_t mdns_update_status(bool connected, uint32_t ip);
```

**Python 发现示例**:
```python
from zeroconf import Zeroconf, ServiceBrowser

zeroconf = Zeroconf()
browser = ServiceBrowser(zeroconf, "_stm32bridge._tcp.local.", listener)
```

### 4.3 端口分配

| 端口 | 协议 | 用途 | 认证 |
|------|------|------|------|
| 80 | HTTP | Landing Page | Basic Auth |
| 4444 | TCP | MCP 命令 | Token |
| 8080 | WebSocket | UART 透传 | Token |

---

## 5. MCP 协议

### 5.1 连接流程

```
1. Agent 通过 mDNS 发现 ESP32 IP
2. TCP 连接到端口 4444
3. 发送: AUTH <token>  (必须在 5 秒内)
4. ESP32 验证 Token
5. 响应: OK: Authorized  或  ERROR: Invalid token
6. 进入命令模式
```

### 5.2 命令列表

#### 5.2.1 认证命令
```
AUTH <token>
  响应: OK: Authorized
        ERROR: Invalid token
        ERROR: Device locked (5 min)
```

#### 5.2.2 系统命令
```
version
  响应: OK: ESP32-STM32-Bridge v1.0.0

status
  响应: OK: {"wifi_connected":true,"ip":"192.168.1.105",
             "mcu_connected":true,"idcode":"0x2BA01477"}

reboot
  响应: OK: Rebooting...
```

#### 5.2.3 SWD 命令
```
reset
  功能: SWD 线复位，读取 IDCODE
  响应: OK: IDCODE=0x2BA01477
        ERROR: SWD not responding

idcode
  功能: 仅读取 IDCODE
  响应: OK: 0x2BA01477

halt
  功能: 停止 STM32 内核
  响应: OK: Core halted
        ERROR: Failed to halt
```

#### 5.2.4 Flash 命令
```
upload <size>
  功能: 准备接收固件数据
  参数: size - 固件大小 (字节)
  响应: OK: Ready for upload
        ERROR: Size too large (max 128KB)
  后续: Agent 发送原始二进制数据

flash_confirm <checksum>
  功能: 确认烧录 (二次确认)
  参数: checksum - SHA256 前 8 位
  响应: OK: Flash started
        ERROR: Checksum mismatch
        ERROR: No firmware loaded

flash
  功能: 执行烧录 (如果关闭二次确认)
  响应: INFO: Halting target...
        INFO: Erasing flash...
        INFO: Programming page 0/10...
        OK: Flash complete
        ERROR: Flash failed at page 5

erase
  功能: 擦除整个 Flash
  响应: INFO: Erasing...
        OK: Erase complete
```

#### 5.2.5 UART 桥接命令
```
uart_start <baudrate>
  功能: 启动 UART 桥接
  参数: baudrate - 波特率
  响应: OK: UART bridge started on ws://ip:8080
        ERROR: Invalid baudrate
  后续: Agent 连接 WebSocket 端口 8080

uart_stop
  功能: 停止 UART 桥接
  响应: OK: UART bridge stopped

uart_config baud=<baud> bits=<8|9> stop=<1|2> parity=<N|E|O>
  功能: 配置 UART 参数
  响应: OK: Configured
```

### 5.3 错误码

| 错误信息 | 说明 |
|----------|------|
| ERROR: Authentication required | 未发送 AUTH 命令 |
| ERROR: Invalid token | Token 验证失败 |
| ERROR: Device locked for N min | 认证失败次数过多 |
| ERROR: Command not found | 未知命令 |
| ERROR: SWD not responding | SWD 通讯失败 |
| ERROR: MCU not detected | 未检测到 STM32 |
| ERROR: Flash verify failed | 烧录校验失败 |

---

## 6. SWD 驱动

### 6.1 硬件接口

使用 ESP32 RMT (Remote Control) 模块实现精确时序：

```c
// swd_driver.h

#define SWD_RMT_CHANNEL     RMT_CHANNEL_0
#define SWD_RMT_CLK_DIV     8       // 80MHz/8 = 10MHz = 100ns/bit
#define SWD_GPIO_SWDIO      GPIO_NUM_3
#define SWD_GPIO_SWCLK      GPIO_NUM_4

typedef struct {
    gpio_num_t swdio_pin;
    gpio_num_t swclk_pin;
    rmt_channel_t rmt_channel;
} swd_handle_t;

// API
esp_err_t swd_init(swd_handle_t *handle);
esp_err_t swd_reset(swd_handle_t *handle);
esp_err_t swd_read_idcode(swd_handle_t *handle, uint32_t *idcode);
esp_err_t swd_read_reg(swd_handle_t *handle, uint8_t addr, uint32_t *data);
esp_err_t swd_write_reg(swd_handle_t *handle, uint8_t addr, uint32_t data);
```

### 6.2 SWD 协议基础

**线复位序列**:
```c
// 50+ 时钟周期，SWDIO 保持高
void swd_line_reset(swd_handle_t *handle) {
    // 发送 52 个时钟周期，SWDIO=1
    // 发送 JTAG-to-SWD 切换序列 0xE79C
    // 再发送 52 个时钟周期
    // 8 个空闲周期 (SWDIO=0)
}
```

**SWD 读写时序**:
```
请求阶段 (8 bits):
  Start(1) + APnDP(1) + RnW(1) + A[2:3](2) + Parity(1) + Stop(0) + Park(1)

ACK 阶段 (3 bits):
  OK=001, WAIT=010, FAULT=100

数据阶段 (33 bits):
  DATA[0:31] + Parity
```

### 6.3 寄存器地址

| 寄存器 | 地址 | 类型 | 说明 |
|--------|------|------|------|
| DP_IDCODE | 0x00 | 读 | 设备 ID |
| DP_ABORT | 0x00 | 写 | 清除错误 |
| DP_CTRL | 0x04 | 读/写 | 控制寄存器 |
| DP_RESEND | 0x08 | 读 | 重读数据 |
| DP_RDBUFF | 0x0C | 读 | 读缓冲 |
| AP_CSW | 0x00 | 读/写 | AP 控制状态 |
| AP_TAR | 0x04 | 读/写 | 传输地址 |
| AP_DRW | 0x0C | 读/写 | 数据读写 |

---

## 7. Flash 编程算法 (STM32F1)

### 7.1 STM32F1 Flash 特性

| 参数 | 值 |
|------|-----|
| 基地址 | 0x08000000 |
| 页大小 | 1KB (1024 bytes) |
| 写宽度 | 16-bit (半字) |
| Flash 寄存器 | 0x40022000 |
| 解锁密钥 1 | 0x45670123 |
| 解锁密钥 2 | 0xCDEF89AB |

**寄存器定义**:
```c
#define FLASH_ACR       0x40022000  // 访问控制
#define FLASH_KEYR      0x40022004  // 密钥
#define FLASH_OPTKEYR   0x40022008  // 选项字节密钥
#define FLASH_SR        0x4002200C  // 状态
#define FLASH_CR        0x40022010  // 控制
#define FLASH_AR        0x40022014  // 地址
#define FLASH_OBR       0x4002201C  // 选项字节
#define FLASH_WRPR      0x40022020  // 写保护

// 状态位
#define FLASH_SR_BSY        (1 << 0)
#define FLASH_SR_PGERR      (1 << 2)
#define FLASH_SR_WRPRTERR   (1 << 4)
#define FLASH_SR_EOP        (1 << 5)

// 控制位
#define FLASH_CR_PG         (1 << 0)   // 编程使能
#define FLASH_CR_PER        (1 << 1)   // 页擦除
#define FLASH_CR_MER        (1 << 2)   // 全擦除
#define FLASH_CR_STRT       (1 << 6)   // 开始
#define FLASH_CR_LOCK       (1 << 7)   // 锁定
```

### 7.2 API 接口

```c
// flash_writer.h

// 初始化 (检测 MCU 类型)
esp_err_t flash_init(swd_handle_t *swd, uint32_t *flash_size);

// 解锁 Flash
esp_err_t flash_unlock(swd_handle_t *swd);

// 锁定 Flash
esp_err_t flash_lock(swd_handle_t *swd);

// 擦除页
esp_err_t flash_erase_page(swd_handle_t *swd, uint32_t page_addr);

// 擦除全部
esp_err_t flash_erase_all(swd_handle_t *swd);

// 写半字 (16-bit)
esp_err_t flash_write_halfword(swd_handle_t *swd, uint32_t addr, uint16_t data);

// 写缓冲区 (自动分页)
esp_err_t flash_write_buffer(swd_handle_t *swd, uint32_t addr, 
                              uint8_t *data, uint32_t len);

// 校验
bool flash_verify(swd_handle_t *swd, uint32_t addr, uint8_t *data, uint32_t len);
```

### 7.3 编程流程

```
1. 解锁 Flash
   写 KEY1 到 FLASH_KEYR
   写 KEY2 到 FLASH_KEYR

2. 擦除页
   设置 FLASH_CR_PER
   写页地址到 FLASH_AR
   设置 FLASH_CR_STRT
   等待 BSY=0

3. 编程
   设置 FLASH_CR_PG
   写 16-bit 数据到目标地址
   等待 BSY=0
   清除 FLASH_CR_PG

4. 校验
   读回数据比较

5. 锁定
   设置 FLASH_CR_LOCK
```

---

## 8. UART 桥接

### 8.1 配置参数

支持波特率:
- 9600, 19200, 38400, 57600, 115200 (默认)
- 230400, 460800, 921600

数据格式:
- 数据位: 8 (默认), 9
- 停止位: 1 (默认), 2
- 校验: None (默认), Even, Odd

### 8.2 双通道设计

```
Channel 1: UART <-> WebSocket (实时透传)
Channel 2: UART <-> MCP 命令 (AT 指令模式)

默认: Channel 1 模式
切换: 通过 MCP 命令 uart_start/uart_stop
```

### 8.3 WebSocket 协议

**连接**:
```
ws://esp32-ip:8080/uart?token=<uart_token>
```

**数据帧**:
- Client -> Server: 原始串口发送数据
- Server -> Client: 原始串口接收数据

**控制帧** (JSON):
```json
{"type": "status", "connected": true, "baud": 115200}
{"type": "error", "message": "UART timeout"}
{"type": "stats", "tx_bytes": 1024, "rx_bytes": 2048}
```

---

## 9. 安全机制

### 9.1 威胁模型

| 威胁 | 风险 | 缓解措施 |
|------|------|----------|
| 未授权 Web 访问 | 高 | HTTP Basic Auth |
| 未授权 MCP 控制 | 高 | Token 认证 |
| 固件被篡改 | 高 | 烧录二次确认 + Checksum |
| Token 被盗 | 中 | Token 绑定 MAC + 密码 |
| 暴力破解 | 中 | 失败锁定机制 |
| WiFi 窃听 | 低 | WPA2/WPA3 |

### 9.2 认证体系

**三层认证**:

1. **Landing Page** (HTTP Basic Auth)
   ```
   用户名: admin
   密码: <用户设置>
   ```

2. **MCP 连接** (Token)
   ```
   Token = SHA256(MAC + admin_password + salt)
   Salt = "STM32Bridge2024"
   ```

3. **UART WebSocket** (独立 Token)
   ```
   可选独立密码
   或复用 MCP Token
   ```

### 9.3 Token 生成 (C 代码)

```c
// security.h
#include "mbedtls/sha256.h"

#define TOKEN_LENGTH 64
#define SALT "STM32Bridge2024"

// 生成 Token
void generate_token(const uint8_t *mac, const char *password, char *token);

// 验证 Token
bool verify_token(const uint8_t *mac, const char *password, const char *token);

// 生成随机 Token (用于 UART)
void generate_random_token(char *token, size_t len);
```

### 9.4 失败锁定

```c
// 配置
#define MAX_AUTH_FAIL 5
#define LOCK_DURATION_SEC 300  // 5分钟

// 状态
static int auth_fail_count = 0;
static time_t lock_until = 0;

// 检查是否锁定
bool is_locked(void) {
    return time(NULL) < lock_until;
}

// 记录失败
void record_auth_fail(void) {
    auth_fail_count++;
    if (auth_fail_count >= MAX_AUTH_FAIL) {
        lock_until = time(NULL) + LOCK_DURATION_SEC;
        auth_fail_count = 0;
    }
}
```

### 9.5 烧录保护

**二次确认机制**:
1. Agent 上传固件
2. ESP32 计算 SHA256 checksum
3. Agent 发送 `flash_confirm <checksum>`
4. ESP32 校验 checksum 匹配
5. 如果启用确认: 在 Landing Page 显示确认对话框
6. 用户点击"确认"后才执行烧录

**白名单** (可选):
```
配置允许访问的 IP 范围
例如: 192.168.1.0/24
```

---

## 10. Landing Page 功能

### 10.1 页面结构

```
/ (首页)
├── 设备信息
│   ├── 设备名、IP、MAC
│   ├── 运行时间
│   └── mDNS 服务信息
│
├── 串口配置
│   ├── 波特率选择
│   ├── 数据位/停止位/校验
│   ├── WebSocket 端口
│   └── 连接测试
│
├── MCP 配置
│   ├── TCP 端口
│   ├── Token 显示/重新生成
│   ├── IP 白名单
│   └── 烧录确认开关
│
├── 安全设置
│   ├── 修改管理员密码
│   ├── UART 独立密码
│   ├── 自动锁定设置
│   └── 访问日志
│
├── 调试工具
│   ├── 读取 IDCODE
│   ├── 复位 STM32
│   ├── 进入 Bootloader
│   └── 实时日志查看
│
└── 系统维护
    ├── 固件 OTA 更新
    ├── 导出配置
    ├── 恢复出厂
    └── 重启设备
```

### 10.2 实时日志

WebSocket 实时推送系统日志:
```json
{"time": "14:32:15", "level": "INFO", "msg": "MCP connected from 192.168.1.100"}
{"time": "14:32:18", "level": "INFO", "msg": "IDCODE read: 0x2BA01477"}
{"time": "14:35:42", "level": "WARN", "msg": "Auth failed from 192.168.1.50"}
```

### 10.3 烧录确认对话框

当收到烧录请求时弹出:
```
┌─────────────────────────────────────┐
│ ⚠️  固件烧录确认                      │
├─────────────────────────────────────┤
│ 来源: 192.168.1.100                 │
│ 固件大小: 24,576 bytes              │
│ Checksum: a1b2c3d4                  │
│ 目标: STM32F103 (ID: 0x2BA01477)    │
│                                     │
│ [查看固件信息] [确认烧录] [取消]     │
└─────────────────────────────────────┘
```

---

## 11. 配置存储 (NVS)

### 11.1 配置结构

```c
// config_storage.h

#define NVS_NAMESPACE "stm32bridge"
#define CONFIG_KEY "config_v1"
#define CONFIG_MAGIC 0x53544D32  // "STM2"

typedef struct {
    // 验证标记
    uint32_t magic;
    uint16_t version;
    
    // WiFi 配置
    char wifi_ssid[32];
    char wifi_pass[64];
    uint8_t wifi_static_ip;  // 0=DHCP, 1=Static
    uint32_t wifi_ip;
    uint32_t wifi_gateway;
    uint32_t wifi_netmask;
    
    // 安全设置
    char admin_pass_hash[64];     // SHA256
    uint8_t auth_enabled;
    uint8_t burn_confirm_required;
    uint8_t max_auth_fail;
    uint16_t lock_duration_sec;
    char allowed_ip_range[32];    // 例如 "192.168.1.0/24"
    
    // UART 设置
    uint32_t uart_baud;
    uint8_t uart_data_bits;
    uint8_t uart_stop_bits;
    char uart_parity;  // 'N', 'E', 'O'
    uint8_t uart_separate_auth;
    char uart_pass_hash[64];
    
    // MCP 设置
    uint16_t mcp_port;
    char mcp_token[65];
    
    // 系统
    uint32_t first_boot_time;
    uint32_t boot_count;
    
} bridge_config_t;
```

### 11.2 API

```c
esp_err_t config_init(void);
esp_err_t config_load(bridge_config_t *cfg);
esp_err_t config_save(bridge_config_t *cfg);
esp_err_t config_reset_to_default(void);
bool config_is_first_boot(void);
```

---

## 12. 开发计划

### 12.1 里程碑

| 阶段 | 工期 | 交付物 | 验证标准 |
|------|------|--------|----------|
| **Phase 1: 基础** | 3天 | WiFi + mDNS + HTTP | 可访问 Landing Page |
| **Phase 2: 安全** | 2天 | 认证 + NVS | Token 验证通过 |
| **Phase 3: MCP** | 3天 | TCP + SWD + Flash | 可读取 IDCODE |
| **Phase 4: UART** | 2天 | WebSocket 桥接 | 双向数据透传 |
| **Phase 5: 页面** | 2天 | 完整 Landing Page | 所有功能可用 |
| **Phase 6: 集成** | 3天 | 测试 + 文档 | 与 Python MCP 集成 |
| **总计** | **15天** | | |

### 12.2 详细任务

**Day 1-2: 项目搭建**
- [ ] ESP-IDF 项目创建
- [ ] 分区表配置 (增加 SPIFFS)
- [ ] CMakeLists.txt 配置
- [ ] 基础日志系统

**Day 3: WiFi Manager**
- [ ] AP 模式实现
- [ ] STA 模式实现
- [ ] Captive Portal DNS
- [ ] NVS 配置读写

**Day 4: mDNS + HTTP**
- [ ] mDNS 服务注册
- [ ] HTTP Server 框架
- [ ] 静态文件服务 (SPIFFS)

**Day 5: 安全框架**
- [ ] SHA256 实现
- [ ] Token 生成/验证
- [ ] HTTP Basic Auth
- [ ] 失败锁定机制

**Day 6-7: MCP Server**
- [ ] TCP Server socket
- [ ] 命令解析器
- [ ] AUTH 流程
- [ ] 命令分发

**Day 8: SWD Driver**
- [ ] RMT 配置
- [ ] GPIO 初始化
- [ ] SWD 基础协议
- [ ] IDCODE 读取

**Day 9-10: Flash Writer**
- [ ] F1 Flash 算法
- [ ] Buffer 管理
- [ ] 擦除/编程/校验
- [ ] 进度回调

**Day 11-12: UART Bridge**
- [ ] UART 驱动
- [ ] WebSocket Server
- [ ] 双向数据流
- [ ] 配置动态修改

**Day 13-14: Landing Page**
- [ ] HTML/CSS/JS
- [ ] API 接口
- [ ] 实时日志
- [ ] 烧录确认

**Day 15: 集成测试**
- [ ] Python MCP 客户端对接
- [ ] 硬件测试
- [ ] 文档整理

---

## 13. 测试用例

### 13.1 SWD 测试
```
测试: IDCODE 读取
输入: reset 命令
期望: OK: IDCODE=0x2BA01477

测试: 多次复位
输入: 连续 10 次 reset
期望: 每次返回正确的 IDCODE
```

### 13.2 Flash 测试
```
测试: 烧录 10KB 固件
输入: upload 10240, <数据>, flash_confirm <checksum>
期望: OK: Flash complete, 校验通过

测试: 大文件烧录
输入: upload 65536 (64KB)
期望: 分段上传成功，完整烧录
```

### 13.3 安全测试
```
测试: 错误 Token
输入: AUTH wrong_token (5次)
期望: 第5次后设备锁定 5 分钟

测试: 未授权访问
输入: 直接发送 reset (无 AUTH)
期望: ERROR: Authentication required
```

### 13.4 UART 测试
```
测试: 波特率 115200
输入: uart_start 115200, WebSocket 发送 "Hello"
期望: STM32 收到 "Hello", 回复通过 WS 传回

测试: 大数据量
输入: 连续发送 1MB 数据
期望: 无丢包，顺序正确
```

---

## 14. 依赖组件

### 14.1 ESP-IDF 组件
```
- esp_wifi
- esp_http_server
- esp_https_server (可选)
- mdns
- nvs_flash
- driver (RMT, UART, GPIO)
- esp_timer
- mbedtls (SHA256)
```

### 14.2 第三方组件
```
- cJSON: JSON 解析
- 可选: libwebsockets (如果用第三方 WS)
```

### 14.3 Python Agent 依赖
```python
zeroconf  # mDNS 发现
websocket-client  # UART 桥接
```

---

## 15. 参考文档

- [ESP32-C3 技术规格书](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf)
- [ESP-IDF 编程指南](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/)
- [STM32F1 参考手册 RM0008](https://www.st.com/resource/en/reference_manual/cd00171190-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf)
- [SWD 协议规范](https://developer.arm.com/documentation/ihi0031/c/)

---

## 16. 附录

### 16.1 现有 Python MCP 客户端接口

确保新 ESP32 固件与现有代码兼容：

```python
# esp32_bridge_client.py 接口保持不变

class ESP32BridgeClient:
    def __init__(self, host: str, port: int = 4444):
        self.host = host
        self.port = port
        self.token = None
    
    def connect(self, password: str = None):
        """建立连接并认证"""
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect((self.host, self.port))
        
        if password:
            token = self._generate_token(password)
            self._send(f"AUTH {token}")
            response = self._recv()
            if not response.startswith("OK"):
                raise AuthError(response)
    
    def reset(self) -> str:
        """SWD 复位，返回 IDCODE"""
        self._send("reset")
        return self._recv()
    
    def flash_firmware(self, data: bytes) -> bool:
        """烧录固件"""
        # 1. 上传
        self._send(f"upload {len(data)}")
        self.socket.sendall(data)
        
        # 2. 确认
        checksum = hashlib.sha256(data).hexdigest()[:8]
        self._send(f"flash_confirm {checksum}")
        
        # 3. 等待完成
        while True:
            line = self._recv()
            if "OK: Flash complete" in line:
                return True
            if "ERROR" in line:
                return False
    
    def start_uart_bridge(self, baudrate: int = 115200):
        """启动 UART 桥接，返回 WebSocket URL"""
        self._send(f"uart_start {baudrate}")
        response = self._recv()
        # 解析 ws://ip:8080/uart?token=xxx
        return websocket_url
```

### 16.2 快速启动检查清单

**硬件:**
- [ ] ESP32C3 Super Mini × 1
- [ ] STM32F103 开发板 × 1
- [ ] USB-C 线 × 1
- [ ] 杜邦线 (母对母) × 10

**软件:**
- [ ] ESP-IDF v5.1+ 安装完成
- [ ] Python 3.10+ 环境
- [ ] `pip install zeroconf websocket-client`

**烧录 ESP32:**
```bash
cd esp32c3_stm32_bridge
idf.py set-target esp32c3
idf.py menuconfig  # 配置 WiFi 默认参数 (可选)
idf.py build
idf.py flash
idf.py monitor
```

**测试:**
```python
# 发现设备
python -c "from esp32_bridge_client import discover; print(discover())"

# 连接并读取 IDCODE
python -c "
from esp32_bridge_client import ESP32BridgeClient
client = ESP32BridgeClient('esp32-bridge-a1b2.local')
client.connect(password='your_password')
print(client.reset())
"
```

---

**文档结束**
