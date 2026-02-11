# STM32 MCP Build Server

AI自动编译修复系统 - 基于 MCP (Model Context Protocol) 的 STM32 固件编译服务器

## 🎯 项目目标

构建一个 **MCP Build Server**，实现以下闭环：

```
Agent 修改本地代码
    ↓
调用 MCP build_firmware
    ↓
MCP 启动 Docker 编译
    ↓
MCP 返回结构化报错
    ↓
Agent 根据报错修复代码
    ↓
再次调用 MCP 编译
    ↓
直到成功（或达到迭代上限）
```

## 🏗️ 系统架构

```
┌─────────────────────────────────────────────────────────┐
│ 本地工作区 (Workspace)                                   │
│                                                         │
│ ├── Core/Src/      (用户代码，可修改)                    │
│ ├── Core/Inc/      (用户头文件，可修改)                  │
│ ├── Drivers/       (HAL库，禁止修改)                     │
│ ├── Makefile       (CubeMX生成)                         │
│ └── tools/build.sh (容器内编译入口脚本)                  │
└───────────────────────────┬─────────────────────────────┘
                            │ 只读挂载 (-v workspace:/src:ro)
                            ▼
┌─────────────────────────────────────────────────────────┐
│ MCP Build Server                                         │
│                                                         │
│ stm32_build_server.py (FastMCP)                         │
│ ├── build_firmware()  ← Agent调用的MCP工具              │
│ └── gcc_parse.py      ← GCC/LD报错解析器                │
└───────────────────────────┬─────────────────────────────┘
                            │ docker run --network=none
                            ▼
┌─────────────────────────────────────────────────────────┐
│ Docker 编译容器                                          │
│                                                         │
│ stm32-toolchain:latest                                  │
│ ├── arm-none-eabi-gcc (Arm GNU Toolchain)               │
│ ├── make                                                │
│ └── /src(只读) → 拷贝到 /work → 编译 → 输出到 /out       │
└─────────────────────────────────────────────────────────┘
```

## 📁 项目结构

```
STM32_Complier_MCP/
├── docker/                 # Docker编译环境
│   └── Dockerfile         # arm-none-eabi-gcc工具链镜像
├── tools/                 # 编译工具脚本
│   └── build.sh          # 容器内编译入口脚本
├── mcp_build/            # MCP Server核心代码
│   ├── __init__.py       # 包初始化
│   ├── stm32_build_server.py  # MCP Server主程序
│   └── gcc_parse.py      # GCC/LD错误解析器
├── Requirement/          # 需求文档
├── Test_Data/            # 测试工程
│   └── Elder_Lifter_STM32_V1.32/
├── docs/                 # 项目文档
├── scripts/              # 辅助脚本
├── tests/                # 单元测试
├── AGENTS.md            # Agent规范
├── CHANGELOG.md         # 版本变更日志
└── README.md            # 本文件
```

## 🚀 快速开始

### 1. 构建 Docker 镜像

```bash
docker build -f docker/Dockerfile -t stm32-toolchain:latest .
```

### 2. 验证镜像

```bash
docker run --rm stm32-toolchain:latest arm-none-eabi-gcc --version
docker run --rm stm32-toolchain:latest make --version
```

### 3. 安装依赖

```bash
pip install -r requirements.txt
```

### 4. 启动 MCP Server

```bash
# STDIO 模式
python -m mcp_build.stm32_build_server

# 或使用 MCP Inspector 调试
uv run mcp dev mcp_build/stm32_build_server.py
```

### 5. Agent 调用示例

```python
from agents import Agent
from agents.mcp import MCPServerStdio

async def main():
    async with MCPServerStdio(
        command="python",
        args=["-m", "mcp_build.stm32_build_server"],
        cwd="/path/to/your-project",
    ) as mcp_server:
        agent = Agent(
            name="STM32 Build Agent",
            instructions="...",
            mcp_servers=[mcp_server],
        )
        # ... agent 循环修复逻辑
```

## 🛡️ 核心原则

| 原则 | 说明 |
|------|------|
| **MCP只编译，不改代码** | 源码以只读方式挂载进Docker容器 (`:ro`) |
| **Agent只改代码，不直接编译** | 所有编译动作都通过MCP工具调用 |
| **结构化错误返回** | MCP解析GCC输出，返回`file/line/col/message`结构化数据 |
| **可重复构建环境** | 使用Docker容器保证编译环境一致性 |

## 📋 开发阶段

- [x] **Phase 0**: 项目初始化与仓库搭建
- [ ] **Phase 1**: Docker编译环境
  - [ ] Dockerfile (arm-none-eabi-gcc)
  - [ ] build.sh (容器内编译脚本)
  - [ ] 测试工程Makefile
- [ ] **Phase 2**: MCP Server核心
  - [ ] stm32_build_server.py
  - [ ] build_firmware工具
  - [ ] 安全校验和超时控制
- [ ] **Phase 3**: 错误解析器
  - [ ] gcc_parse.py
  - [ ] GCC/LD错误解析
  - [ ] 完整闭环测试

## 🔧 技术栈

- **Language**: Python 3.10+
- **MCP Framework**: FastMCP (mcp[cli]>=1.26.0)
- **Container**: Docker + Ubuntu 24.04
- **Toolchain**: arm-none-eabi-gcc (GNU Arm Embedded Toolchain)
- **Build System**: GNU Make
- **Target**: STM32F1xx 系列 (Cortex-M3)

## 📝 许可证

MIT License

## 🔗 参考文档

- [MCP官方规范](https://modelcontextprotocol.io/)
- [MCP Python SDK](https://github.com/modelcontextprotocol/python-sdk)
- [Arm GNU Toolchain](https://developer.arm.com/Tools%20and%20Software/GNU%20Toolchain)
- [STM32CubeMX用户手册](https://www.st.com/resource/en/user_manual/um1718-stm32cubemx-for-stm32-configuration-and-initialization-code-generation-stmicroelectronics.pdf)
