# STM32开发工作流 - 快速参考

## 快速开始

```python
# 1. 加载Skill（在Agent对话中）
/使用 stm32-dev-workflow

# 2. 编译项目
result = await self.mcp.stm32_build.build_firmware(
    workspace="/path/to/project"
)

# 3. 烧录固件
result = await self.mcp.stm32_flash.flash_firmware(
    workspace="/path/to/project",
    auto_detect=True
)
```

## 完整工作流示例

```python
# 修改代码 → 编译 → 烧录

# Step 1: 修改代码（Agent执行）
agent.modify_code("实现LED闪烁功能")

# Step 2: 编译
build = await self.mcp.stm32_build.build_firmware(
    workspace="./project",
    clean=True
)

if not build.ok:
    # 自动修复错误
    agent.fix_code(build.formatted_errors)
    # 重新编译...

# Step 3: 烧录
flash = await self.mcp.stm32_flash.flash_firmware(
    workspace="./project",
    auto_detect=True
)

if flash.ok:
    print(f"✅ 成功烧录到 {flash.mcu_info.name}")
```

## 常用API

### Build MCP

```python
# 编译
build = await mcp.stm32_build.build_firmware(
    workspace="/path/to/project",    # 必需
    project_subdir="",                # Makefile子目录
    clean=True,                       # make clean
    jobs=4,                          # 并行任务
    timeout_sec=600                  # 超时
)

# 检查错误
if build.ok:
    print("编译成功")
else:
    print(f"错误数: {len(build.errors)}")
    for error in build.errors:
        print(f"  {error.file}:{error.line}: {error.message}")
```

### Flash MCP

```python
# 健康检查
health = await mcp.stm32_flash.health_check()
print(f"本地ST-Link: {'可用' if health.local_available else '不可用'}")

# 检测MCU
detection = await mcp.stm32_flash.detect_mcu()
if detection.detected:
    print(f"检测到: {detection.name}")

# 烧录
flash = await mcp.stm32_flash.flash_firmware(
    workspace="/path/to/project",
    auto_detect=True,                 # 自动检测MCU
    verify=True,                      # 验证
    reset=True                        # 复位
)

# 列出烧录器
flashers = await mcp.stm32_flash.list_flashers()
for f in flashers.flashers:
    print(f"{f.name}: {'可用' if f.available else '不可用'}")
```

## 错误处理

```python
# 编译失败处理
if not build.ok:
    if build.errors:
        # 有结构化错误，可以自动修复
        agent.fix_code(build.formatted_errors)
    else:
        # 系统错误，询问用户
        agent.ask_user("编译失败，请检查配置")

# 烧录失败处理
if not flash.ok:
    error_code = flash.get("error_code", "")
    if "NO_FLASHER_AVAILABLE" in error_code:
        print("❌ 未找到烧录器，请检查ST-Link连接")
    elif "TARGET_NOT_FOUND" in error_code:
        print("❌ 未检测到MCU，请检查硬件连接")
```

## 返回值结构

### Build结果
```python
{
    "ok": True/False,
    "exit_code": 0,
    "workspace": "/path/to/project",
    "outdir": "/path/to/project/out/artifacts",
    "artifacts": ["firmware.hex", "firmware.bin", "firmware.elf"],
    "errors": [
        {
            "type": "compiler",
            "file": "main.c",
            "line": 42,
            "col": 9,
            "message": "undefined identifier"
        }
    ],
    "error_summary": {"total": 5, "errors": 3, "warnings": 2},
    "log_tail": "...",
    "duration_sec": 45.2
}
```

### Flash结果
```python
{
    "ok": True/False,
    "flasher_type": "local_openocd",
    "flasher_name": "LocalOpenOCD",
    "device_id": "0x20036410",
    "mcu_info": {
        "name": "STM32F103C8",
        "family": "STM32F1",
        "flash_kb": 64,
        "ram_kb": 20,
        "core": "Cortex-M3"
    },
    "duration_sec": 5.2,
    "message": "Flash successful"
}
```

## 配置

### MCP配置 (.opencode/mcp.json)

```json
{
  "mcpServers": {
    "stm32-build": {
      "command": "python",
      "args": ["-m", "mcp_build.stm32_build_server"],
      "cwd": "/path/to/STM32_Complier_MCP"
    },
    "stm32-flash": {
      "command": "python",
      "args": ["-m", "mcp_flash.stm32_flash_server_v2"],
      "cwd": "/path/to/STM32_Complier_MCP"
    }
  }
}
```

## 限制

- **当前仅支持本地ST-Link**（Phase 1）
- ESP32远程烧录在Phase 2实现
- 串口功能为简化版

## 更多信息

查看完整文档：
- SKILL.md - 完整的工作流文档
- scripts/agent_example.py - Agent使用示例
- references/mcp-config.json - MCP配置示例
