# STM32 MCP Build Server

> ⚠️ **架构升级通知**: 当前版本 (v1.0) 采用"复制代码到项目"的安装方式。
> v2.0 将重构为 **`uvx` + Docker** 标准架构，实现零安装、零污染。
> 详见 [ARCHITECTURE.md](ARCHITECTURE.md) 和 [Issue #1](https://github.com/legogogoagent/STM32-Complier-MCP/issues/1)。

AI自动编译修复系统 - 基于 MCP (Model Context Protocol) 的 STM32 开发平台

## 🎯 项目目标

构建 MCP Server，实现完整的嵌入式开发闭环：

```
Agent修改代码 → Build MCP编译 → 解析错误自动修复 → 编译成功 → Flash MCP烧录 → MCU运行
```

## 🏗️ 架构版本

### v1.0 (当前版本)

安装方式：复制MCP代码到用户项目 + `pip install`

```json
{
  "mcpServers": {
    "stm32-build": {
      "command": "python3",
      "args": ["-m", "mcp_build.stm32_build_server"]
    }
  }
}
```

**已知问题** (详见 [Issue #1](https://github.com/legogogoagent/STM32-Complier-MCP/issues/1)):
- 需要手动安装ARM工具链
- Makefile兼容性问题 (Windows路径、GCC版本)
- 安装步骤繁琐 (6步, >20分钟首次安装)
- 往用户项目复制代码 (非行业标准)

### v2.0 (开发中 → `feature/uvx-docker-refactor`)

安装方式：`uvx` + Docker，**零安装、零污染**

```json
{
  "mcpServers": {
    "stm32": {
      "command": "uvx",
      "args": ["stm32-mcp"]
    }
  }
}
```

**改进**:
- ✅ 不复制代码到用户项目
- ✅ 不污染用户Python环境
- ✅ 预构建Docker镜像 (`legogogoagent/stm32-toolchain:12.3`)
- ✅ Makefile自动修复 (容器内临时副本)
- ✅ 符合MCP行业标准 (Playwright MCP、SQLite MCP同款模式)

详见 [ARCHITECTURE.md](ARCHITECTURE.md)

---

## 🚀 快速开始 (v1.0)

> 以下为 v1.0 安装方式。v2.0 发布后将大幅简化。

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

## 📁 项目结构

```
STM32_Complier_MCP/
├── docker/                      # Docker编译环境
│   ├── Dockerfile              # arm-none-eabi-gcc工具链镜像
│   └── flash.Dockerfile        # 烧录工具镜像 (OpenOCD/ST-Link)
├── tools/                       # 编译工具脚本
│   ├── build.sh                # 容器内编译入口脚本
│   └── flash.sh                # 容器内烧录入口脚本
├── mcp_build/                   # Build MCP Server核心代码
│   ├── __init__.py
│   ├── stm32_build_server.py   # Build MCP主程序
│   └── gcc_parse.py            # GCC/LD错误解析器
├── mcp_flash/                   # Flash MCP Server核心代码
│   ├── __init__.py
│   └── stm32_flash_server.py   # Flash MCP主程序
├── ESP32_STM32_Bridge/          # ESP32远程烧录桥接器
│   ├── firmware/               # ESP32 Arduino固件
│   ├── scripts/                # Python客户端
│   └── tests/                  # Flash算法单元测试
├── Test_Data/                   # 测试工程
├── ARCHITECTURE.md             # 架构文档 (v1.0 → v2.0)
├── CHANGELOG.md                # 版本变更日志
└── requirements.txt            # Python依赖
```

## 🛡️ 核心原则

| 原则 | 说明 |
|------|------|
| **MCP只编译，不改代码** | 源码以只读方式挂载进Docker容器 (`:ro`) |
| **Agent只改代码，不直接编译** | 所有编译动作都通过MCP工具调用 |
| **结构化错误返回** | MCP解析GCC输出，返回`file/line/col/message`结构化数据 |
| **可重复构建环境** | 使用Docker容器保证编译环境一致性 |

## 📋 开发阶段

### v1.0 已完成

- [x] **Phase 0**: 项目初始化与仓库搭建
- [x] **Phase 1**: Docker编译环境 (Dockerfile + build.sh)
- [x] **Phase 2**: Build MCP Server (stm32_build_server.py + gcc_parse.py)
- [x] **Phase 3**: Flash MCP Server (stm32_flash_server.py)
- [x] **Phase 2.5**: ESP32远程烧录桥接器
  - [x] ESP32 Arduino固件 (SWD协议实现)
  - [x] STM32F1xx Flash编程算法
  - [x] STM32F4xx Flash编程算法
  - [x] 73个单元测试
- [x] **Skill**: OpenCode Agent Skill (stm32-dev-workflow)

### v2.0 开发中

- [ ] 重构为 PyPI 包 (`stm32-mcp`)
- [ ] `uvx stm32-mcp` 零安装启动
- [ ] 预构建 Docker 镜像 (`legogogoagent/stm32-toolchain:12.3`)
- [ ] 多架构支持 (x86_64 + ARM64)
- [ ] Makefile自动修复 (容器内)
- [ ] 增强错误提示 (头文件大小写检测)

## 🔧 技术栈

- **Language**: Python 3.10+
- **MCP Framework**: FastMCP (mcp[cli]>=1.26.0)
- **Container**: Docker + Ubuntu 24.04
- **Toolchain**: arm-none-eabi-gcc (GNU Arm Embedded Toolchain)
- **Build System**: GNU Make
- **Target**: STM32F1xx / F4xx 系列

## 📝 许可证

MIT License

## 🔗 参考文档

- [ARCHITECTURE.md](ARCHITECTURE.md) - 架构文档与重构计划
- [CHANGELOG.md](CHANGELOG.md) - 版本变更日志
- [MCP官方规范](https://modelcontextprotocol.io/)
- [MCP Python SDK](https://github.com/modelcontextprotocol/python-sdk)
- [Arm GNU Toolchain](https://developer.arm.com/Tools%20and%20Software/GNU%20Toolchain)
