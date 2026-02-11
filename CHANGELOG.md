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
- [ ] Phase 1: Docker编译环境搭建
  - [ ] 创建docker/Dockerfile
  - [ ] 创建tools/build.sh
  - [ ] 生成测试工程Makefile
  
- [ ] Phase 2: MCP Server核心
  - [ ] 创建mcp_build/stm32_build_server.py
  - [ ] 实现build_firmware工具
  - [ ] 安全校验和超时控制
  
- [ ] Phase 3: 错误解析器
  - [ ] 创建mcp_build/gcc_parse.py
  - [ ] 解析GCC/LD错误
  - [ ] 完整闭环测试

---

## Release Schedule

| Version | Target Date | Milestone |
|---------|-------------|-----------|
| 0.1.0 | 2026-02-11 | ✅ 项目初始化完成 |
| 0.2.0 | TBD | Docker编译环境 |
| 0.3.0 | TBD | MCP Server核心 |
| 0.4.0 | TBD | GCC错误解析器 |
| 1.0.0 | TBD | 完整闭环 + 验收通过 |

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
