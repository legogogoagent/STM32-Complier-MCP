# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Planned
- Docker编译环境搭建
- MCP Build Server核心实现
- GCC/LD错误解析器
- 完整闭环测试

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
