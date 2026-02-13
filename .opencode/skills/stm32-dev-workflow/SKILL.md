---
name: stm32-dev-workflow
description: |-
  STM32开发工作流Skill（v2.0）- 通过uvx自动安装，支持编译、烧录、调试闭环。
  
  **v2.0重大更新**: 现在使用 `uvx stm32-mcp` 自动安装，无需手动复制文件！
  
  Use proactively when:
  - 用户要求"编译STM32项目"
  - 需要烧录固件到STM32 MCU
  - 用户要求"配置STM32开发环境"、"安装STM32工具"
  
  Examples:
  - user: "编译这个STM32项目" → 检查mcp.json → 创建配置 → 调用build_firmware
  - user: "烧录固件到STM32" → 调用flash_firmware → 自动检测MCU → 烧录
  - user: "配置STM32开发环境" → 创建.opencode/mcp.json → 完成
---

> 🎉 **v2.0已发布**: 现在通过 `uvx stm32-mcp` 自动安装，零配置，零污染！
> 
> - PyPI: https://pypi.org/project/stm32-mcp/
> - Docker: https://hub.docker.com/r/legogogoagent/stm32-toolchain

# STM32开发工作流 Skill (v2.0)

## ⚠️ 重要: OpenCode 配置路径

**正确的配置文件路径是 `.opencode/opencode.json`，不是 `.opencode/mcp.json`！**

```json
// ✅ 正确: .opencode/opencode.json
{
  "$schema": "https://opencode.ai/config.json",
  "mcp": {
    "stm32": {
      "type": "local",
      "command": ["uvx", "stm32-mcp"],
      "enabled": true
    }
  }
}
```

```json
// ❌ 错误: .opencode/mcp.json (OpenCode 不会读取此文件)
{
  "mcpServers": { ... }
}
```

配置完成后，**需要重启 Agent** 才能加载新的 MCP 服务器。

## 快速开始（3秒钟）

在任何STM32项目中：

```bash
# 1. 创建配置目录和文件
mkdir -p .opencode
cat > .opencode/opencode.json << 'EOF'
{
  "$schema": "https://opencode.ai/config.json",
  "mcp": {
    "stm32": {
      "type": "local",
      "command": ["uvx", "stm32-mcp"],
      "enabled": true
    }
  }
}
EOF

# 2. 重启 OpenCode Agent
# 3. 开始使用！
```

✅ 完成！Agent现在可以自动编译和烧录STM32项目了。

## 安装流程（Agent自动执行）

### 方式1: 自动检测并配置

```
User: 配置STM32开发环境

Agent:
  正在配置STM32开发环境...
  
  1. 检查 .opencode/opencode.json 是否存在
  2. 如果不存在，创建正确格式的配置
  3. 提醒用户：配置完成后需要重启 Agent
  4. uvx 自动下载并启动 stm32-mcp
  5. 验证Docker环境
  
  ✅ 配置完成！请重启 Agent 后使用。
```

### 方式2: 手动配置

```bash
# 创建项目级配置（推荐）
mkdir -p .opencode
cat > .opencode/opencode.json << 'EOF'
{
  "$schema": "https://opencode.ai/config.json",
  "mcp": {
    "stm32": {
      "type": "local",
      "command": ["uvx", "stm32-mcp"],
      "enabled": true
    }
  }
}
EOF

# 或者创建用户全局配置（所有项目可用）
mkdir -p ~/.config/opencode
cat > ~/.config/opencode/opencode.json << 'EOF'
{
  "$schema": "https://opencode.ai/config.json",
  "mcp": {
    "stm32": {
      "type": "local",
      "command": ["uvx", "stm32-mcp"],
      "enabled": true
    }
  }
}
EOF
```

**重要**: 修改配置后必须**重启 Agent**！

## 前置条件

### 必需
- **Docker** - 用于编译环境（自动下载镜像）
- **uv** - Python包管理器（通常已安装）

### 可选（烧录需要）
- **ST-Link** 调试器
- **OpenOCD** - 烧录工具

### 安装检查命令

```bash
# 检查Docker
docker --version

# 检查uv
uv --version
```

## MCP工具调用（v2.0统一接口）

```python
# 新的统一接口（v2.0）
result = await mcp.stm32.build_firmware(
    workspace="/path/to/project",
    clean=True,
    jobs=4
)

result = await mcp.stm32.flash_firmware(
    workspace="/path/to/project",
    hex_file="",
    verify=True
)

result = await mcp.stm32.detect_mcu()

result = await mcp.stm32.check_environment()

result = await mcp.stm32.get_server_info()
```

## 核心工作流

### 工作流1: 修改-编译-修复循环

```python
# 当用户要求修改代码并编译时
async def modify_and_build(agent, workspace, task_description):
    """
    1. 根据task_description修改代码
    2. 调用Build MCP编译
    3. 如果有错误，自动修复
    4. 循环直到编译成功或达到最大重试次数
    """
    max_retries = 3
    
    for attempt in range(max_retries):
        # Step 1: 修改代码（Agent执行）
        agent.modify_code(task_description)
        
        # Step 2: 编译
        build_result = await agent.mcp.stm32.build_firmware(
            workspace=workspace,
            clean=True
        )
        
        # Step 3: 检查结果
        if build_result.ok:
            print("✅ 编译成功")
            return build_result
        
        # Step 4: 解析错误并修复
        if build_result.errors:
            print(f"⚠️  编译失败（尝试 {attempt+1}/{max_retries}）")
            print(f"错误: {build_result.error_summary}")
            
            # 自动修复
            fix_prompt = f"""
            编译错误如下：
            {build_result.formatted_errors}
            
            请修复这些错误。只修改必要的代码，保持原有逻辑。
            """
            agent.fix_code(fix_prompt)
        else:
            print(f"❌ 编译失败: {build_result.log_tail}")
            break
    
    return build_result
```

### 工作流2: 完整开发闭环

```python
# 修改-编译-烧录-验证
async def full_development_loop(agent, workspace, task):
    """
    完整工作流：
    1. 修改代码
    2. 编译
    3. 烧录
    4. 串口验证
    5. 根据验证结果决定下一步
    """
    
    # Step 1-2: 修改并编译
    build = await modify_and_build(agent, workspace, task)
    if not build.ok:
        return {"ok": False, "stage": "build", "error": build.error}
    
    # Step 3: 烧录
    flash = await agent.mcp.stm32.flash_firmware(
        workspace=workspace,
        auto_detect=True,
        prefer_local=True
    )
    
    if not flash.ok:
        return {"ok": False, "stage": "flash", "error": flash.error}
    
    print(f"✅ 烧录成功: {flash.mcu_info.name}")
    
    # Step 4: 串口验证（可选）
    if task.requires_verification:
        serial = await agent.mcp.stm32.open_serial(
            baudrate=115200
        )
        await serial.write(b"test\n")
        response = await serial.read_until(b"OK", timeout=5.0)
        
        # 根据响应判断是否需要调整
        if b"ERROR" in response:
            print("⚠️  运行时错误，需要修复")
            # 触发新一轮修改
            return await full_development_loop(agent, workspace, task)
    
    return {"ok": True, "message": "任务完成"}
```

## MCP工具调用参考

### Build MCP工具

```python
# 编译固件
build_result = await mcp.stm32.build_firmware(
    workspace="/path/to/project",     # 工程根目录（必需）
    project_subdir="",                # Makefile子目录
    clean=True,                       # 是否先make clean
    jobs=4,                          # 并行编译任务数
    make_target="all",               # make目标
    timeout_sec=600,                 # 超时秒数
    max_log_tail_kb=96               # 日志尾部大小限制
)

# 返回结构
{
    "ok": True/False,
    "exit_code": 0,
    "workspace": "/path/to/project",
    "outdir": "/path/to/project/out/artifacts",
    "artifacts": ["firmware.hex", "firmware.bin"],
    "errors": [...],                  # 结构化错误列表
    "error_summary": {...},           # 错误统计
    "log_tail": "...",                # 日志尾部
    "duration_sec": 45.2
}
```

### Flash MCP工具

```python
# 烧录固件
flash_result = await mcp.stm32.flash_firmware(
    workspace="/path/to/project",     # 工程根目录（必需）
    hex_file="",                      # hex文件路径（可选，自动查找）
    auto_detect=True,                 # 自动检测MCU
    target_family="",                 # 手动指定系列（如"F4"）
    verify=True,                      # 验证烧录
    reset=True,                       # 烧录后复位
    timeout_sec=120,                  # 超时
    prefer_local=True                 # 优先本地ST-Link
)

# 返回结构
{
    "ok": True/False,
    "flasher_type": "local_openocd",
    "flasher_name": "LocalOpenOCD",
    "device_id": "0x20036410",
    "mcu_info": {
        "name": "STM32F103C8",
        "family": "STM32F1",
        "flash_kb": 64,
        "ram_kb": 20
    },
    "duration_sec": 5.2,
    "message": "Flash successful"
}

# 健康检查
health = await mcp.stm32.health_check()
# 返回: {ok, status, local_available, remote_available, targets_detected, recommendation}

# 检测MCU
detection = await mcp.stm32.detect_mcu()
# 返回: {ok, detected, device_id, name, family, mcu_info, flasher_type}

# 列出烧录器
flashers = await mcp.stm32.list_flashers()
# 返回: {ok, total, available, flashers: [...]}
```

## 智能决策逻辑

### 编译失败处理

```python
def handle_build_error(build_result, attempt, max_retries):
    """智能处理编译错误"""
    
    # 错误分类
    if build_result.exit_code == 2:
        # Makefile错误
        return "ask_user", "Makefile配置错误，需要手动检查"
    
    if not build_result.errors:
        # 没有结构化错误，可能是系统问题
        return "retry", "系统错误，尝试重试"
    
    # 分析错误类型
    error_types = set(e["type"] for e in build_result.errors)
    
    if "compiler" in error_types:
        # 编译错误可以自动修复
        if attempt < max_retries:
            return "auto_fix", "自动修复编译错误"
        else:
            return "ask_user", f"编译错误，已重试{max_retries}次未解决"
    
    if "linker" in error_types:
        # 链接错误通常需要手动处理
        return "ask_user", "链接错误，可能需要检查库或内存配置"
    
    return "auto_fix", "尝试自动修复"
```

### 烧录失败处理

```python
def handle_flash_error(flash_result):
    """智能处理烧录错误"""
    
    if "NO_FLASHER_AVAILABLE" in flash_result.get("error_code", ""):
        return "ask_user", "未找到可用烧录器。请检查：\n1. ST-Link是否连接\n2. USB权限是否正确\n3. 或者配置远程ESP32烧录器"
    
    if "TIMEOUT" in flash_result.get("error_code", ""):
        return "retry", "烧录超时，尝试重试"
    
    if "TARGET_NOT_FOUND" in flash_result.get("error", ""):
        return "ask_user", "未检测到MCU。请检查：\n1. STM32是否正确连接\n2. 电源是否正常\n3. SWD接线是否正确"
    
    return "ask_user", f"烧录失败: {flash_result.get('error', '未知错误')}"
```

## 使用示例

### 示例1: 简单的编译任务

```
User: 编译这个STM32项目

Agent:
  1. 确定workspace路径
  2. 调用 mcp.stm32.build_firmware
  3. 检查结果
  4. 如果成功，显示编译产物信息
  5. 如果失败，解析错误并询问用户是否自动修复
```

### 示例2: 修改后编译烧录

```
User: 把LED闪烁间隔改为500ms，然后编译烧录

Agent:
  1. 找到LED相关代码（通常在main.c或led.c）
  2. 修改delay时间：HAL_Delay(1000) → HAL_Delay(500)
  3. 调用build_firmware编译
  4. 如果有编译错误，自动修复（最多3次）
  5. 编译成功后调用flash_firmware烧录
  6. 显示烧录结果和MCU信息
```

### 示例3: 完整的调试工作流

```
User: 实现串口回显功能并测试

Agent:
  1. 编写串口初始化代码
  2. 编写中断接收和发送代码
  3. 编译（自动修复可能的错误）
  4. 烧录到MCU
  5. 打开串口监视器
  6. 发送测试数据："Hello"
  7. 检查是否收到回显："Hello"
  8. 如果正常，任务完成
  9. 如果不正常，分析原因并修复
```

## 错误码参考

### Build MCP错误码

| 错误码 | 含义 | 建议处理 |
|--------|------|----------|
| COMPILATION_ERROR | 编译错误 | 自动修复 |
| LINKER_ERROR | 链接错误 | 询问用户 |
| MAKEFILE_ERROR | Makefile错误 | 询问用户 |
| TIMEOUT | 编译超时 | 重试或询问 |

### Flash MCP错误码

| 错误码 | 含义 | 建议处理 |
|--------|------|----------|
| NO_FLASHER_AVAILABLE | 无可用烧录器 | 询问用户检查连接 |
| TARGET_NOT_FOUND | 未找到MCU | 询问用户检查硬件 |
| TIMEOUT | 烧录超时 | 重试 |
| VERIFY_FAILED | 验证失败 | 询问用户 |

## 最佳实践

1. **始终先检查健康状态**
   ```python
   health = await mcp.stm32.health_check()
   if not health["local_available"]:
       print("⚠️  ST-Link未连接，请检查硬件")
   ```

2. **保存编译产物**
   ```python
   if build_result.ok:
       print(f"✅ 编译产物: {build_result.outdir}")
       print(f"   - {build_result.artifacts}")
   ```

3. **显示MCU信息**
   ```python
   if flash_result.ok and flash_result.mcu_info:
       mcu = flash_result.mcu_info
       print(f"✅ 目标MCU: {mcu['name']}")
       print(f"   Flash: {mcu['flash_kb']}KB")
       print(f"   RAM: {mcu['ram_kb']}KB")
   ```

4. **优雅处理失败**
   - 重试3次后仍失败则询问用户
   - 提供清晰的错误信息
   - 给出可能的解决方案

## 架构对比

### v1.0 (旧方式)

```
安装: 6步，20+分钟
1. git clone 仓库
2. cp -r mcp_build/ 项目/
3. cp -r mcp_flash/ 项目/
4. pip install -r requirements.txt
5. docker build ...
6. 配置 mcp.json
```

### v2.0 (新方式) ⭐

```
安装: 1步，3秒钟
1. 创建 .opencode/mcp.json（uvx stm32-mcp）

✅ 零配置，零污染，自动更新
```

## 限制和注意事项

1. **当前仅支持本地ST-Link**
   - ESP32远程烧录在后续版本实现

2. **自动修复能力**
   - 简单的编译错误（如拼写、头文件）可以自动修复
   - 逻辑错误需要用户介入
   - 最多自动重试3次

3. **Docker必需**
   - 编译依赖Docker环境
   - 镜像自动从Docker Hub拉取

## 版本历史

- **v2.0.0** (2026-02-13): 重大重构
  - ✅ uvx自动安装（`uvx stm32-mcp`）
  - ✅ 预构建Docker镜像（Docker Hub）
  - ✅ Makefile自动修复（Windows路径、GCC选项）
  - ✅ 统一MCP接口（stm32.build_firmware, stm32.flash_firmware）
  - ✅ 零配置，零污染
  
- **v1.0.0** (2026-02-12): 初始版本
  - 本地Build + Flash MCP
  - 手动安装（复制文件到项目）

## 参考链接

- PyPI: https://pypi.org/project/stm32-mcp/
- Docker Hub: https://hub.docker.com/r/legogogoagent/stm32-toolchain
- GitHub: https://github.com/legogogoagent/STM32-Complier_MCP
- Issue #1: https://github.com/legogogoagent/STM32-Complier-MCP/issues/1
