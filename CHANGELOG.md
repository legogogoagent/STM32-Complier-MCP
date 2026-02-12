# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Planned
- Phase 3 - Advanced debug features
- Phase 4 - CI/CD integration and web interface
- ESP32 hardware testing and validation

## [0.8.0] - 2026-02-12

### ESP32 Flash MCP Integration 🔌

#### Added
- **Full ESP32RemoteFlasher Implementation** (`mcp_flash/esp32_remote_flasher.py`)
  - Complete async interface for WiFi-based STM32 flashing
  - TCP socket communication with ESP32 Bridge
  - IDCODE-based MCU auto-detection (F1/F4/F7/H7/L0/L1/L4/WB/WL series)
  - Flash programming with progress callbacks
  - Error handling (BridgeError, FlashError)
  
- **New MCP Tools for ESP32** (`mcp_flash/stm32_flash_server.py`)
  - `flash_firmware_esp32()` - Remote flash via ESP32 Bridge
  - `discover_esp32_devices()` - Auto-discovery on local network
  - `check_esp32_bridge()` - Connection and MCU detection check
  - HEX to BIN conversion for Intel HEX format support
  - Automatic firmware format detection
  
- **Enhanced Flash Info** 
  - Added ESP32 to supported programmers list
  - Added WiFi to supported interfaces
  - Remote support configuration in server info
  
#### Technical Details
- **Communication Protocol**: TCP socket on port 4444
- **SWD Interface**: GPIO18 (SWDIO) / GPIO19 (SWCLK)
- **Default Network**: 192.168.4.1 (AP mode)
- **Supported Formats**: .bin (native), .hex (converted)
- **Timeout**: Configurable (default 300s for large firmware)

#### Usage Example
```python
# Discover ESP32 devices
result = await flash_server.discover_esp32_devices()
# Returns: [{"ip": "192.168.4.1", "port": 4444, "version": "v1.0.0"}]

# Check ESP32 connection
check = await flash_server.check_esp32_bridge("192.168.4.1")
# Returns: {connected: true, mcu_connected: true, mcu_idcode: "0x2BA01477"}

# Flash firmware remotely
result = await flash_server.flash_firmware_esp32(
    workspace="/path/to/project",
    esp32_host="192.168.4.1",
    esp32_port=4444
)
```

#### Architecture
```
Flash MCP Server
      │
      ├── LocalOpenOCDFlasher (ST-Link/J-Link)
      │
      └── ESP32RemoteFlasher (NEW)
            ├── ESP32BridgeClient (TCP socket)
            │       └── Connect to 192.168.4.1:4444
            └── SWD Commands
                    ├── reset → MCU IDCODE
                    ├── upload → Firmware binary
                    └── flash → Program STM32
```

#### Files Modified
- `mcp_flash/esp32_remote_flasher.py` - Full implementation (290 lines)
- `mcp_flash/stm32_flash_server.py` - Added 3 new tools (+200 lines)
- `mcp_flash/flasher_router.py` - ESP32 route support

#### Testing
- Python import validation ✅
- MCP tool registration ✅
- Protocol integration ✅
- Hardware testing: Pending (requires physical ESP32 + STM32)

## [0.7.0] - 2026-02-12

### ESP32 Remote Flasher Project 🆕

#### Added
- **ESP32 Firmware** (`ESP32_STM32_Bridge/firmware/esp32_stm32_bridge.ino`)
  - SWD bit-banging implementation over GPIO
  - WiFi AP/STA mode support
  - TCP command protocol (reset, idcode, upload, flash)
  - Serial bridge for STM32 UART forwarding
  - Firmware buffer up to 256KB
  
- **Python Client Library** (`ESP32_STM32_Bridge/scripts/esp32_bridge_client.py`)
  - `ESP32BridgeClient` - Main client class
  - `ESP32BridgeDiscovery` - Auto-discovery on local network
  - `ESP32RemoteFlasher` - MCP-compatible flasher interface
  - Full documentation and examples
  
- **Hardware Documentation** (`ESP32_STM32_Bridge/docs/HARDWARE.md`)
  - Pin connection diagrams
  - Power supply options
  - Troubleshooting guide
  - Multi-target extension plans
  
- **Project README** (`ESP32_STM32_Bridge/README.md`)
  - Project overview and features
  - Quick start guide
  - Protocol documentation
  - MCP integration examples

#### Architecture
```
PC/MCP Server ──WiFi──▶ ESP32 ──SWD──▶ STM32
                          │
                          └──UART──▶ Serial Bridge
```

### OpenCode Skill v1.0 - Auto-Installation 🎯

#### Added
- **Auto-Detection System** (`SKILL.md` frontmatter)
  - Pattern matching for GitHub URL requests
  - Trigger phrases: "安装skill", "配置STM32环境"
  - Automatic skill loading without manual installation
  
- **Installation Script** (`scripts/install-from-github.sh`)
  - Clone from GitHub
  - Copy skill to project `.opencode/skills/`
  - Copy MCP code to project root
  - Auto-generate `.opencode/mcp.json`
  - Python import verification
  
- **Installation Guide** (`SKILL_INSTALL.md`)
  - Agent auto-install instructions
  - Manual installation fallback
  - Troubleshooting section
  
- **Prompt Examples** (`PROMPT_EXAMPLES.md`)
  - User prompt patterns
  - Agent response examples
  - Best practices

#### Files Created
- `ESP32_STM32_Bridge/firmware/esp32_stm32_bridge.ino` (189 lines)
- `ESP32_STM32_Bridge/scripts/esp32_bridge_client.py` (340 lines)
- `ESP32_STM32_Bridge/docs/HARDWARE.md` (215 lines)
- `ESP32_STM32_Bridge/README.md` (156 lines)

#### Installation Test ✅
- Tested auto-installation in isolated environment
- All components installed correctly
- MCP configuration auto-generated
- Python imports verified

## [0.6.0] - 2026-02-11

### OpenCode Skill Creation 🎨

#### Added
- **Complete Skill Structure** (`.opencode/skills/stm32-dev-workflow/`)
  - `SKILL.md` - Full workflow documentation
  - `QUICK_REFERENCE.md` - API quick reference
  - `scripts/agent_example.py` - Agent usage examples
  - `references/mcp-config.json` - Configuration template

#### Integration
- MCP server configuration templates
- Agent workflow examples
- Error handling patterns

## [0.5.0] - 2026-02-11

### Multi-Target MCU Support 🔧

#### Added
- **MCU Database** (`mcp_flash/mcu_database.py`)
  - 40+ MCU definitions (F1/F4/F7/H7 series)
  - IDCODE to MCU mapping
  - Flash algorithm metadata
  - Memory layout specifications
  
- **Auto-Detection** (`mcp_flash/stm32_flash_server_v2.py`)
  - OpenOCD IDCODE reading
  - Automatic MCU matching
  - Flash algorithm selection
  
- **Unified Architecture**
  - `BaseFlasher` abstract interface
  - `LocalOpenOCDFlasher` implementation
  - `FlasherRouter` for smart routing

#### Supported MCUs
- **F1 Series**: F103, F105, F107 (Cortex-M3)
- **F4 Series**: F401, F407, F411, F429, F446 (Cortex-M4)
- **F7 Series**: F722, F767 (Cortex-M7)
- **H7 Series**: H743, H747 (Cortex-M7, dual-core)

## [0.4.0] - 2026-02-10

### Flash MCP v2 & Docker Support 🐳

#### Added
- **Flash MCP Server v2** (`mcp_flash/stm32_flash_server_v2.py`)
  - Multi-target support architecture
  - Improved error handling
  - Health check endpoint
  
- **Docker Flash Environment** (`docker/flash.Dockerfile`)
  - OpenOCD installation
  - USB device support
  - Multi-platform compatibility
  
- **Version Switching** (`mcp_flash/version_switch.py`)
  - Runtime version selection
  - Migration utilities

## [0.3.0] - 2026-02-09

### Dual-MCP Architecture 🔌

#### Added
- **Flash MCP Server** (`mcp_flash/stm32_flash_server.py`)
  - `flash_firmware()` tool
  - ST-Link/OpenOCD/J-Link support
  - Flash verification
  - Reset control
  
- **Flash Scripts** (`tools/flash.sh`)
  - Containerized flashing
  - Device permission handling
  
- **Shared Output Directory** (`out/`)
  - Build MCP writes artifacts
  - Flash MCP reads for programming

#### Architecture
```
┌─────────────────────────────────────┐
│           Agent (AI Assistant)      │
└──────────┬─────────────────┬────────┘
           │                 │
    ┌──────▼──────┐   ┌─────▼────────┐
    │ Build MCP    │   │ Flash MCP    │
    │ (编译)       │   │ (烧录)       │
    └──────┬──────┘   └──────┬───────┘
           │                 │
           └────────┬────────┘
                    ▼
              ┌──────────┐
              │ STM32 MCU│
              └──────────┘
```

## [0.2.0] - 2026-02-11

### Phase 1: Docker编译环境 🐳

#### Added
- **Docker镜像定义** (`docker/Dockerfile`)
  - 基础镜像: Ubuntu 24.04 LTS
  - 工具链: gcc-arm-none-eabi (GNU Arm Embedded Toolchain)
  - 辅助工具: make, python3, bash, coreutils, findutils, sed, grep, gawk, file
  - 工作目录: /src (只读), /work (可写), /out (输出)
  - 镜像大小目标: < 1GB

- **容器内编译脚本** (`tools/build.sh`)
  - 从只读 /src 拷贝源码到可写 /work/project
  - 支持 make clean (通过CLEAN环境变量控制)
  - 支持并行编译 (通过JOBS环境变量控制)
  - 完整日志输出到 /out/build.log
  - 自动收集编译产物 (.elf, .hex, .bin, .map, .lst) 到 /out/artifacts/
  - 返回正确的make退出码

- **测试工程Makefile** (`Test_Data/Elder_Lifter_STM32_V1.32/Elder_Lifter_STM32/Makefile`)
  - 目标: STM32F103CBTx (Cortex-M3)
  - 内存: 128KB Flash, 20KB RAM
  - 包含23个Core源文件 (用户代码 + HAL MSP)
  - 包含16个HAL库源文件
  - 预处理器定义: USE_HAL_DRIVER, STM32F103xB
  - 优化等级: -O2
  - 链接脚本: STM32F103CBTX_FLASH.ld
  - 生成产物: .elf, .hex, .bin, .map

#### Docker Build Instructions
```bash
# 构建镜像
docker build -f docker/Dockerfile -t stm32-toolchain:latest .

# 验证镜像
docker run --rm stm32-toolchain:latest arm-none-eabi-gcc --version
docker run --rm stm32-toolchain:latest make --version

# 测试编译 (在工程目录)
docker run --rm --network=none \
  -v $(pwd)/Test_Data/Elder_Lifter_STM32_V1.32/Elder_Lifter_STM32:/src:ro \
  -v /tmp/build_out:/out:rw \
  -e CLEAN=1 \
  -e JOBS=8 \
  stm32-toolchain:latest bash /src/tools/build.sh
```

#### Files Created
- `docker/Dockerfile` (39 lines)
- `tools/build.sh` (147 lines, executable)
- `Test_Data/Elder_Lifter_STM32_V1.32/Elder_Lifter_STM32/Makefile` (179 lines)

---

## [0.1.0] - 2026-02-11

### Project Initialization 🚀

#### Added
- **项目初始化**
  - 创建项目目录结构
  - 初始化AGENTS.md - Agent角色和规范定义
  - 初始化CHANGELOG.md - 版本变更日志
  - 创建README.md框架
  - 初始化Git仓库

- **需求文档整理**
  - Requirement/stm32_mcp_2in1.txt - 完整需求与技术规格
  - Requirement/stm32_mcp_gpt.txt - 本地Agent自动修复方案
  - Requirement/stm32_mcp_opus.txt - 项目需求与技术方案

- **测试工程**
  - Test_Data/Elder_Lifter_STM32_V1.32/ - STM32F103CBTx工程
    - MCU: STM32F103CBTx (Cortex-M3, 128KB Flash, 20KB RAM)
    - HAL库: STM32F1xx_HAL_Driver
    - USB Device功能
    - 自定义模块: Lifter_Task, Motor, Modbus等

#### Directory Structure
```
STM32_Complier_MCP/
├── docker/                 # Docker编译环境
├── tools/                 # 编译工具脚本
├── mcp_build/            # MCP Server核心代码
├── Requirement/          # 需求文档
├── Test_Data/            # 测试工程
├── docs/                 # 项目文档
├── scripts/              # 辅助脚本
├── tests/                # 单元测试
├── .opencode/            # 会话记忆
├── AGENTS.md            # Agent规范
├── CHANGELOG.md         # 版本变更日志
└── README.md            # 项目说明
```

#### Technical Stack
- **Language**: Python 3.10+
- **MCP Framework**: FastMCP (mcp[cli]>=1.26.0)
- **Container**: Docker + Ubuntu 24.04
- **Toolchain**: arm-none-eabi-gcc
- **Build System**: Make

#### Next Steps

##### Phase 1-3: Build MCP (编译)
- [x] Phase 1: Docker编译环境搭建 ✅
  - [x] 创建docker/Dockerfile
  - [x] 创建tools/build.sh
  - [x] 生成测试工程Makefile
  
- [ ] Phase 2: MCP Server核心
  - [ ] 创建mcp_build/stm32_build_server.py
  - [ ] 创建mcp_build/__init__.py
  - [ ] 创建requirements.txt
  - [ ] 实现build_firmware工具
  - [ ] 安全校验和超时控制
  
- [ ] Phase 3: 错误解析器
  - [ ] 创建mcp_build/gcc_parse.py
  - [ ] 解析GCC/LD错误
  - [ ] 完整闭环测试

##### Phase 4-6: Flash MCP (烧录) 🆕
- [ ] Phase 4: Flash MCP Server
  - [ ] 创建mcp_flash/stm32_flash_server.py
  - [ ] 实现flash_firmware工具
  - [ ] 支持ST-Link/OpenOCD/J-Link
  
- [ ] Phase 5: 烧录环境搭建
  - [ ] 创建docker/flash.Dockerfile
  - [ ] USB设备权限配置
  - [ ] 烧录脚本工具
  
- [ ] Phase 6: 双MCP集成
  - [ ] Build → Flash 产物传递机制
  - [ ] Agent协调两个MCP
  - [ ] 完整闭环测试（修改→编译→烧录→运行）

---

## Release Schedule

| Version | Target Date | Milestone | Scope |
|---------|-------------|-----------|-------|
| 0.1.0 | 2026-02-11 | ✅ 项目初始化完成 | 基础架构 |
| 0.2.0 | 2026-02-11 | ✅ Docker编译环境 | Build MCP |
| 0.3.0 | TBD | MCP Server核心 | Build MCP |
| 0.4.0 | TBD | GCC错误解析器 | Build MCP |
| **0.5.0** | **TBD** | **🆕 Flash MCP Server** | **Flash MCP** |
| **0.6.0** | **TBD** | **🆕 烧录环境搭建** | **Flash MCP** |
| **0.7.0** | **TBD** | **🆕 双MCP集成** | **集成测试** |
| 1.0.0 | TBD | 完整闭环 + 验收通过 | 生产就绪 |

---

## Contributing

### Commit Message Format
```
<type>(<scope>): <subject>

<body>

<footer>
```

Types: `feat`, `fix`, `docs`, `style`, `refactor`, `test`, `chore`

### Version Bump Rules
- **MAJOR**: 不兼容的API更改
- **MINOR**: 向后兼容的功能添加
- **PATCH**: 向后兼容的问题修复
