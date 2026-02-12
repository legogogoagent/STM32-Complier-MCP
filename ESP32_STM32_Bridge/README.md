# ESP32 STM32 Bridge 项目

ESP32作为远程SWD桥接器，实现STM32的无线烧录和调试。

## 项目结构

```
ESP32_STM32_Bridge/
├── firmware/                   # ESP32固件
│   └── esp32_stm32_bridge.ino # Arduino主程序
├── scripts/                    # Python工具脚本
│   └── esp32_bridge_client.py # Python客户端库
├── docs/                       # 文档
│   ├── HARDWARE.md            # 硬件连接指南
│   ├── PROTOCOL.md            # 通信协议文档
│   └── API.md                 # API参考
└── examples/                   # 使用示例
    ├── basic_flash.py         # 基础烧录示例
    └── mcp_integration.py     # MCP集成示例
```

## 功能特性

- ✅ **SWD桥接** - ESP32通过GPIO模拟SWD协议
- ✅ **WiFi连接** - 支持AP模式和STA模式
- ✅ **固件上传** - 通过TCP socket上传固件
- ✅ **远程烧录** - 将固件烧录到STM32
- ✅ **IDCODE读取** - 自动检测连接的MCU
- ✅ **串口透传** - STM32串口日志通过WiFi转发
- ⚠️ **Flash编程** - 当前为简化实现，需根据具体MCU完善

## 硬件连接

```
ESP32          STM32
-----          -----
GPIO18  ---->  SWDIO
GPIO19  ---->  SWCLK
GPIO21  ---->  NRST (可选)
GND     ---->  GND
3.3V    ---->  3.3V (可选供电)

可选串口连接:
GPIO16 (RX) <--  STM32 UART TX
GPIO17 (TX) -->  STM32 UART RX
```

## 快速开始

### 1. 硬件准备

- ESP32开发板 (ESP32-WROOM-32或类似)
- STM32目标板 (F1/F4/F7/H7系列)
- 连接线 (杜邦线)

### 2. 烧录ESP32固件

1. 安装Arduino IDE并添加ESP32支持
2. 打开 `firmware/esp32_stm32_bridge.ino`
3. 选择正确的板子和端口
4. 编译并上传

### 3. 连接WiFi

**AP模式** (默认):
- ESP32创建热点: `ESP32-STM32-Bridge`
- 密码: `stm32flash`
- 连接后访问: `192.168.4.1:4444`

**STA模式**:
- 修改代码中的WiFi凭据
- ESP32连接到现有WiFi网络

### 4. Python客户端测试

```bash
cd scripts
python esp32_bridge_client.py
```

## 通信协议

### TCP命令格式

所有命令以换行符 `\n` 结尾，响应格式为 `STATUS: message`。

| 命令 | 描述 | 响应 |
|------|------|------|
| `reset` | SWD复位并读取IDCODE | `OK: IDCODE=0xXXXXXXXX` |
| `idcode` | 读取MCU IDCODE | `OK: 0xXXXXXXXX` |
| `upload <size>` | 准备接收固件 | `OK: Ready for upload` |
| `flash` | 烧录已上传的固件 | `OK: Flash complete` |
| `version` | 获取版本信息 | `OK: ESP32-STM32-Bridge v1.0.0` |
| `help` | 显示帮助 | 命令列表 |

### 固件上传流程

```
Client                          ESP32
  |                               |
  |---- upload <size> --------->|
  |<--- OK: Ready for upload ----|
  |                               |
  |---- [binary data...] ------->|
  |                               |
  |<--- OK: Received N bytes ----|
  |                               |
  |---- flash ------------------>|
  |<--- INFO: Halting target ----|
  |<--- INFO: Erasing flash -----|
  |<--- INFO: Programming ------|
  |<--- OK: Complete -----------|
```

## 与STM32 MCP集成

此项目设计为STM32 MCP的远程烧录后端。

### MCP配置示例

```python
# 在FlasherRouter中添加ESP32支持
from esp32_bridge_client import ESP32RemoteFlasher

class ESP32RemoteFlasher(FlasherBase):
    def __init__(self, host: str, port: int = 4444):
        self.flasher = ESP32RemoteFlasher(host, port)
    
    def flash(self, binary_data: bytes) -> bool:
        self.flasher.connect()
        result = self.flasher.flash_binary(binary_data)
        self.flasher.disconnect()
        return result
```

## 当前限制

1. **Flash算法**: 当前为简化实现，需要根据具体STM32型号实现完整的Flash编程算法
2. **速度**: SWD位操作通过软件实现，速度较慢 (~10-50KB/s)
3. **调试功能**: 不支持断点、单步等高级调试功能
4. **多目标**: 目前只支持单个STM32目标

## 未来增强

- [ ] 完整的Flash编程算法 (F1/F4/F7/H7)
- [ ] DMA加速SWD传输
- [ ] 支持JTAG协议
- [ ] 支持CMSIS-DAP协议
- [ ] 多目标支持 (通过多路复用器)
- [ ] TLS/SSL加密通信

## 相关项目

- [STM32 MCP Build Server](https://github.com/legogogoagent/STM32-Complier-MCP) - 编译和烧录MCP Server

## 许可证

MIT License
