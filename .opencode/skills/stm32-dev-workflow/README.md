# STM32开发工作流 Skill

用于OpenCode Agent的STM32开发工作流Skill，封装了Build MCP和Flash MCP的完整调用。

## 功能

- ✅ **自动编译** - 调用Build MCP编译STM32项目
- ✅ **自动修复** - 编译错误时自动修复（最多3次重试）
- ✅ **自动烧录** - 调用Flash MCP烧录到MCU
- ✅ **MCU检测** - 自动检测连接的MCU类型
- ✅ **健康检查** - 检查烧录器可用性
- ✅ **智能决策** - 失败时自动重试或询问用户

## 安装

### 方式1: Agent自动安装（推荐）

在Agent对话中直接说：

```
User: 帮我安装https://github.com/legogogoagent/STM32-Complier-MCP的skill

Agent会自动：
1. 克隆GitHub仓库
2. 复制skill到当前项目的 .opencode/skills/
3. 复制MCP代码到项目
4. 配置 .opencode/mcp.json
5. 验证安装
```

其他可用指令：
- `"配置STM32开发环境"`
- `"安装stm32-dev-workflow"`
- `"从legogogoagent/STM32-Complier-MCP安装skill"`

详见 [SKILL_INSTALL.md](./SKILL_INSTALL.md) 和 [PROMPT_EXAMPLES.md](./PROMPT_EXAMPLES.md)

### 方式2: 手动安装

1. 克隆仓库：
```bash
git clone https://github.com/legogogoagent/STM32-Complier-MCP.git /tmp/stm32-mcp
```

2. 复制Skill到项目：
```bash
mkdir -p .opencode/skills
cp -r /tmp/stm32-mcp/.opencode/skills/stm32-dev-workflow .opencode/skills/
```

3. 复制MCP代码：
```bash
cp -r /tmp/stm32-mcp/mcp_build ./
cp -r /tmp/stm32-mcp/mcp_flash ./
cp /tmp/stm32-mcp/requirements.txt ./
```

4. 配置MCP（.opencode/mcp.json）：
```json
{
  "mcpServers": {
    "stm32-build": {
      "command": "python3",
      "args": ["-m", "mcp_build.stm32_build_server"],
      "cwd": "{PROJECT_ROOT}",
      "env": {
        "PYTHONPATH": "{PROJECT_ROOT}"
      }
    },
    "stm32-flash": {
      "command": "python3",
      "args": ["-m", "mcp_flash.stm32_flash_server_v2"],
      "cwd": "{PROJECT_ROOT}",
      "env": {
        "PYTHONPATH": "{PROJECT_ROOT}"
      }
    }
  }
}
```

5. 安装依赖：
```bash
pip install -r requirements.txt
```

## 使用

### 在Agent对话中使用

```
User: 编译这个STM32项目

Agent: 
  /使用 stm32-dev-workflow
  
  我来帮你编译项目：
  
  result = await self.mcp.stm32_build.build_firmware(
      workspace="./Test_Data/Elder_Lifter_STM32_V1.32/Elder_Lifter_STM32",
      clean=True
  )
  
  if result.ok:
      print("✅ 编译成功！")
      print(f"产物: {result.artifacts}")
  else:
      print(f"❌ 编译失败: {result.error}")
```

### 完整工作流示例

```python
# 修改代码并编译烧录

# 1. 修改代码
agent.modify_code("实现LED闪烁功能")

# 2. 编译
build = await self.mcp.stm32_build.build_firmware(
    workspace="./project",
    clean=True
)

if not build.ok:
    # 自动修复错误
    agent.fix_code(build.formatted_errors)
    # 重试编译...

# 3. 烧录
flash = await self.mcp.stm32_flash.flash_firmware(
    workspace="./project",
    auto_detect=True
)

if flash.ok:
    print(f"✅ 烧录成功: {flash.mcu_info.name}")
```

## 文件结构

```
stm32-dev-workflow/
├── SKILL.md                      # 完整文档
├── QUICK_REFERENCE.md            # 快速参考
├── scripts/
│   └── agent_example.py          # Agent使用示例
└── references/
    └── mcp-config.json           # MCP配置示例
```

## 文档

- **SKILL.md** - 完整的工作流文档，包含所有API参考
- **QUICK_REFERENCE.md** - 快速参考卡片
- **scripts/agent_example.py** - Agent使用示例代码
- **references/mcp-config.json** - MCP配置文件示例

## 前置条件

1. **硬件**
   - ST-Link调试器
   - STM32目标板

2. **软件**
   - OpenOCD 已安装
   - Python 3.8+
   - MCP Server代码（STM32_Complier_MCP项目）

3. **配置**
   - 正确配置 .opencode/mcp.json
   - 设置正确的 PYTHONPATH 和工作目录

## 限制

- **Phase 1**: 当前仅支持本地ST-Link
- **Phase 2**: ESP32远程烧录待实现
- 串口功能为简化版

## 版本历史

- v1.0.0 - Phase 1: 本地ST-Link支持
- v2.0.0 - Phase 2: ESP32远程烧录（计划）

## 更多信息

查看完整文档 [SKILL.md](SKILL.md)

## 许可证

MIT
