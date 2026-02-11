# AGENTS.md - STM32 MCP Build Server

**Project**: STM32 MCP Build Server - AI自动编译修复系统
**Purpose**: 构建MCP (Model Context Protocol) Build Server，实现"修改→编译→解析错误→自动修复"闭环
**Tech Stack**: Python + FastMCP + Docker + arm-none-eabi-gcc

---

## 项目结构

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
│   ├── stm32_mcp_2in1.txt
│   ├── stm32_mcp_gpt.txt
│   └── stm32_mcp_opus.txt
├── Test_Data/            # 测试工程
│   └── Elder_Lifter_STM32_V1.32/
├── docs/                 # 项目文档
├── scripts/              # 辅助脚本
├── tests/                # 单元测试
├── .opencode/            # 会话记忆
├── AGENTS.md            # 本文件 - Agent规范
├── CHANGELOG.md         # 版本变更日志
├── README.md            # 项目说明
└── requirements.txt     # Python依赖
```

---

## AGENT ROLES

### @git-manager (Git 协调员)

**职责**: 管理项目版本控制，确保代码安全

#### 自动触发词（听到这些立即执行提交流程）
- "写完了"、"完成了"、"ok了"、"搞定了" → 标准提交
- "保存一下"、"提交吧"、"commit" → 标准提交  
- "强制提交"、"保存检查点"、"commit now" → 立即提交（不问确认）

#### 定时提醒
- 会话开始30分钟后，如果检测到有未提交的改动：
  "⏰ 已工作30分钟，期间改动了：[文件列表]。是否提交当前进度？"

#### 记忆维护
维护 `.opencode/session.json`：
```json
{
  "session_id": "uuid",
  "start_time": "2026-02-11T10:00:00Z",
  "files_modified": [
    {
      "path": "mcp_build/stm32_build_server.py",
      "edit_count": 3,
      "last_modified": "2026-02-11T10:30:00Z",
      "context": "实现build_firmware工具"
    }
  ],
  "commits": [
    {
      "hash": "abc123",
      "message": "feat: 实现Docker编译环境",
      "time": "2026-02-11T10:15:00Z"
    }
  ]
}
```

---

### @build-master (构建专家)

**职责**: 负责Docker编译环境和Makefile生成

#### 核心任务
1. 创建 `docker/Dockerfile` - Ubuntu 24.04 + arm-none-eabi-gcc
2. 创建 `tools/build.sh` - 容器内编译脚本
3. 为测试工程生成 `Makefile`
4. 确保编译环境可重复

#### 验收标准
- Docker镜像构建成功 (< 1GB)
- arm-none-eabi-gcc --version 输出正确
- build.sh 能在只读 /src 下正常工作

---

### @mcp-developer (MCP开发者)

**职责**: 实现MCP Build Server核心功能

#### 核心任务
1. 创建 `mcp_build/stm32_build_server.py` - FastMCP Server
2. 实现 `build_firmware` 工具
3. 安全校验和超时控制
4. Docker集成和错误处理

#### API规范
```python
@tool
def build_firmware(
    workspace: str,           # 工程根目录绝对路径
    project_subdir: str = "",  # Makefile子目录
    clean: bool = True,       # 是否先make clean
    jobs: int = 8,           # 并行任务数
    make_target: str = "all", # make目标
    timeout_sec: int = 600,   # 超时秒数
    max_log_tail_kb: int = 96, # 日志尾部大小
    image: str = ""          # 覆盖默认镜像
) -> dict:
    """
    返回结构:
    {
        "ok": bool,
        "exit_code": int,
        "image": str,
        "outdir": str,
        "artifacts": list[str],
        "errors": list[dict],
        "log_tail": str,
        "docker_tail": str
    }
    """
```

---

### @parser-engineer (解析工程师)

**职责**: 实现GCC/LD错误解析器

#### 核心任务
1. 创建 `mcp_build/gcc_parse.py`
2. 解析编译期错误：`file:line:col: error: message`
3. 解析链接期错误：`undefined reference to`
4. 路径归一化和排序

#### 错误结构
```python
{
    "type": "compiler" | "linker" | "toolchain" | "system",
    "severity": "error" | "warning" | "note",
    "file": "Core/Src/main.c",  # 相对路径
    "line": 123,
    "col": 9,
    "message": "unknown type name 'Foo_t'",
    "raw": "原始行文本"
}
```

---

## 开发流程

### 阶段1: Docker编译环境
1. 创建 `docker/Dockerfile`
2. 创建 `tools/build.sh`
3. 生成测试工程 `Makefile`
4. **验收**: docker build成功，能编译测试工程

### 阶段2: MCP Server核心
1. 创建 `mcp_build/stm32_build_server.py`
2. 创建 `mcp_build/__init__.py`
3. 创建 `requirements.txt`
4. **验收**: MCP Inspector能调用，返回编译结果

### 阶段3: 错误解析器
1. 创建 `mcp_build/gcc_parse.py`
2. 集成到MCP Server
3. 完整闭环测试
4. **验收**: 正确解析GCC/LD错误

---

## 提交信息格式

```
<type>(<scope>): <subject>

<body>

<footer>
```

### Type
- `feat`: 新功能
- `fix`: 修复bug
- `docs`: 文档更新
- `style`: 代码格式（不影响功能）
- `refactor`: 重构
- `test`: 测试相关
- `chore`: 构建/工具

### Scope
- `docker`: Docker相关
- `mcp`: MCP Server相关
- `parser`: 错误解析器
- `tools`: 工具脚本
- `test`: 测试相关
- `docs`: 文档

### 示例
```
feat(docker): 添加arm-none-eabi-gcc编译环境

- 基于Ubuntu 24.04
- 安装gcc-arm-none-eabi工具链
- 创建/work和/out目录

Closes #1
```

---

## 安全规范

### 必须遵守
1. **MCP只编译，不改代码** - workspace以只读(:ro)挂载
2. **Agent只改代码，不直接编译** - 所有编译通过MCP
3. **容器禁网** - 使用 `--network=none`
4. **路径白名单** - 校验 workspace 在 `STM32_ALLOWED_ROOT` 下
5. **超时控制** - 防止编译卡死

### 禁止操作
- ❌ 在MCP中修改workspace任何文件
- ❌ 接受用户自定义shell命令
- ❌ 容器访问网络
- ❌ 不做路径校验直接挂载

---

## 参考文档

- [MCP官方规范](https://modelcontextprotocol.io/)
- [MCP Python SDK](https://github.com/modelcontextprotocol/python-sdk)
- [Arm GNU Toolchain](https://developer.arm.com/Tools%20and%20Software/GNU%20Toolchain)
- [STM32CubeMX用户手册](https://www.st.com/resource/en/user_manual/um1718-stm32cubemx-for-stm32-configuration-and-initialization-code-generation-stmicroelectronics.pdf)

