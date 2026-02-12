# 🎯 OpenCode Skill 使用指南

## 已创建的Skill

我已经为你创建了 **stm32-dev-workflow** Skill，让OpenCode Agent可以方便地调用Build MCP和Flash MCP。

## 📁 Skill位置

```
.opencode/skills/stm32-dev-workflow/
├── SKILL.md                      # 完整的Skill文档（必需）
├── README.md                     # Skill说明
├── QUICK_REFERENCE.md            # 快速参考
├── scripts/
│   └── agent_example.py          # 使用示例
└── references/
    └── mcp-config.json           # MCP配置示例
```

## 🚀 快速开始

### Step 1: 复制Skill到项目

```bash
# 在你的STM32项目目录下
mkdir -p .opencode/skills
cp -r /path/to/STM32_Complier_MCP/.opencode/skills/stm32-dev-workflow .opencode/skills/
```

### Step 2: 配置MCP

创建 `.opencode/mcp.json`：

```json
{
  "mcpServers": {
    "stm32-build": {
      "command": "python",
      "args": ["-m", "mcp_build.stm32_build_server"],
      "cwd": "/home/abc/projects/STM32_Complier_MCP",
      "env": {
        "PYTHONPATH": "/home/abc/projects/STM32_Complier_MCP"
      }
    },
    "stm32-flash": {
      "command": "python",
      "args": ["-m", "mcp_flash.stm32_flash_server_v2"],
      "cwd": "/home/abc/projects/STM32_Complier_MCP",
      "env": {
        "PYTHONPATH": "/home/abc/projects/STM32_Complier_MCP"
      }
    }
  }
}
```

### Step 3: Agent使用

在Agent对话中：

```
User: 编译这个STM32项目

Agent会自动加载Skill并执行：

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
    print(f"❌ 编译失败，发现 {len(result.errors)} 个错误")
```

## 💡 使用示例

### 示例1: 简单编译

```
User: 编译项目

Agent:
  result = await self.mcp.stm32_build.build_firmware(
      workspace="./project"
  )
```

### 示例2: 修改并编译

```
User: 把LED闪烁间隔改为500ms，然后编译

Agent:
  # 1. 修改代码
  agent.edit_file("main.c", "HAL_Delay(1000)", "HAL_Delay(500)")
  
  # 2. 编译
  result = await self.mcp.stm32_build.build_firmware(
      workspace="./project"
  )
  
  # 3. 检查结果
  if result.ok:
      print("✅ 编译成功")
```

### 示例3: 完整闭环

```
User: 实现串口回显功能并测试

Agent:
  # 1. 编写代码
  agent.write_file("uart.c", uart_code)
  
  # 2. 编译
  build = await self.mcp.stm32_build.build_firmware(workspace="./project")
  
  if not build.ok:
      agent.fix_code(build.formatted_errors)
      build = await self.mcp.stm32_build.build_firmware(workspace="./project")
  
  # 3. 烧录
  flash = await self.mcp.stm32_flash.flash_firmware(
      workspace="./project",
      auto_detect=True
  )
  
  # 4. 验证
  if flash.ok:
      print(f"✅ 烧录到 {flash.mcu_info.name}")
```

## 📚 Skill文档

### SKILL.md 内容

包含以下部分：

1. **概述** - Skill功能和前置条件
2. **核心工作流** - modify_and_build、full_development_loop
3. **MCP工具调用参考** - 完整的API文档
4. **智能决策逻辑** - 错误处理策略
5. **使用示例** - 3个完整示例
6. **错误码参考** - 常见错误和处理方式

### 关键API

#### Build MCP

```python
# 编译
result = await mcp.stm32_build.build_firmware(
    workspace="/path/to/project",
    clean=True,
    jobs=4,
    timeout_sec=600
)

# 返回
{
    "ok": True/False,
    "artifacts": ["firmware.hex", ...],
    "errors": [...],
    "duration_sec": 45.2
}
```

#### Flash MCP

```python
# 健康检查
health = await mcp.stm32_flash.health_check()

# 检测MCU
detection = await mcp.stm32_flash.detect_mcu()

# 烧录
result = await mcp.stm32_flash.flash_firmware(
    workspace="/path/to/project",
    auto_detect=True,
    verify=True
)

# 返回
{
    "ok": True,
    "flasher_type": "local_openocd",
    "mcu_info": {
        "name": "STM32F103C8",
        "family": "STM32F1"
    }
}
```

## ✅ 测试Skill

### 测试编译

```bash
# 在Agent对话中
测试编译项目 ./Test_Data/Elder_Lifter_STM32_V1.32/Elder_Lifter_STM32

Agent会：
1. 加载 stm32-dev-workflow Skill
2. 调用 build_firmware
3. 显示编译结果
```

### 测试烧录

```bash
# 确保ST-Link已连接
测试烧录

Agent会：
1. 检查ST-Link可用性
2. 检测MCU类型
3. 烧录固件
4. 显示MCU信息
```

## 🔧 故障排查

### Skill未加载

检查：
1. Skill文件在 `.opencode/skills/stm32-dev-workflow/SKILL.md`
2. 文件权限正确
3. SKILL.md 格式正确（YAML frontmatter）

### MCP连接失败

检查：
1. `.opencode/mcp.json` 配置正确
2. `cwd` 指向正确的项目路径
3. `PYTHONPATH` 包含项目根目录
4. Python环境已激活

### 编译失败

常见原因：
1. Makefile不存在
2. 依赖缺失
3. 代码错误

查看错误：
```python
result = await mcp.stm32_build.build_firmware(workspace="...")
print(result.errors)  # 结构化错误
print(result.log_tail)  # 日志尾部
```

### 烧录失败

常见原因：
1. ST-Link未连接
2. MCU未连接
3. 权限问题

检查：
```python
health = await mcp.stm32_flash.health_check()
print(health.recommendation)
```

## 📊 功能对比

| 功能 | 旧版 (v0.6) | 新版 (v0.7) | Skill封装 |
|------|-------------|-------------|-----------|
| 编译 | ✅ | ✅ | ✅ 自动重试 |
| 错误解析 | ✅ | ✅ | ✅ 自动修复 |
| 烧录 | ✅ | ✅ | ✅ 自动检测MCU |
| 多目标 | ✅ | ✅ | ✅ 智能选择 |
| 串口 | ⚠️ | ⚠️ 简化 | ⚠️ Phase 2 |
| 远程烧录 | ❌ | ⏳ 预留 | ⏳ Phase 2 |

## 🎓 最佳实践

### 1. 先检查健康状态

```python
health = await mcp.stm32_flash.health_check()
if not health.local_available:
    print("⚠️ ST-Link未连接，请检查硬件")
    return
```

### 2. 使用自动检测

```python
# 自动检测MCU类型
result = await mcp.stm32_flash.flash_firmware(
    workspace="./project",
    auto_detect=True  # 推荐
)
```

### 3. 处理错误

```python
# 编译失败时自动重试
for attempt in range(3):
    result = await mcp.stm32_build.build_firmware(...)
    if result.ok:
        break
    
    if result.errors:
        agent.fix_code(result.formatted_errors)
```

### 4. 显示进度

```python
print(f"🔨 编译中...")
result = await mcp.stm32_build.build_firmware(...)
print(f"✅ 完成！耗时: {result.duration_sec:.1f}秒")
```

## 📝 下一步

1. **测试Skill**
   - 复制Skill到项目
   - 配置MCP
   - 运行测试

2. **自定义工作流**
   - 根据需求修改工作流
   - 添加特定的错误处理
   - 集成到Agent中

3. **Phase 2准备**
   - 等待ESP32固件完成
   - 更新Skill支持远程烧录
   - 测试无线烧录功能

## 📞 支持

如有问题，查看：
- `SKILL.md` - 完整文档
- `QUICK_REFERENCE.md` - 快速参考
- `scripts/agent_example.py` - 示例代码

---

**版本**: v1.0.0 (Phase 1)  
**状态**: ✅ 已就绪，可以测试  
**更新**: 2026-02-12
