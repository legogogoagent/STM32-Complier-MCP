# STM32多目标支持指南

Flash MCP现已支持多目标MCU系列，可自动检测和配置STM32F1/F4/F7/H7等系列。

## 支持的MCU系列

| 系列 | 核心 | 最大频率 | 目标配置 | MCU数量 |
|------|------|----------|----------|---------|
| STM32F1 | Cortex-M3 | 72MHz | stm32f1x.cfg | 6 |
| STM32F4 | Cortex-M4 | 180MHz | stm32f4x.cfg | 16 |
| STM32F7 | Cortex-M7 | 216MHz | stm32f7x.cfg | 11 |
| STM32H7 | Cortex-M7 | 550MHz | stm32h7x.cfg | 7 |

**总计**: 40个MCU型号

## 新增MCP工具

### 1. `detect_mcu()` - 自动检测MCU

自动识别连接的MCU型号和系列。

```python
result = await mcp_flash.detect_mcu(
    programmer="stlink",
    interface="swd",
    timeout_sec=10
)
```

返回示例：
```python
{
    "ok": True,
    "detected": True,
    "device_id": "0x20036410",
    "mcu_info": {
        "name": "STM32F103C8",
        "family": "STM32F1",
        "flash_kb": 64,
        "ram_kb": 20,
        "core": "Cortex-M3",
        "description": "STM32F103C8T6 - Medium-density",
        "max_clock_mhz": 72
    },
    "target_config": "stm32f1x.cfg",
    "programmer": "stlink"
}
```

### 2. `list_supported_mcus_tool()` - 列出支持的MCU

获取数据库中所有支持的MCU列表。

```python
result = await mcp_flash.list_supported_mcus_tool()
```

返回：
```python
{
    "ok": True,
    "total": 40,
    "families": [
        {"name": "STM32F1", "mcu_count": 6, "cores": ["Cortex-M3"], ...},
        {"name": "STM32F4", "mcu_count": 16, "cores": ["Cortex-M4"], ...},
        ...
    ],
    "mcus": [
        {"name": "STM32F103C8", "family": "STM32F1", "flash_kb": 64, ...},
        ...
    ]
}
```

### 3. `get_mcu_database_info()` - 获取数据库信息

获取MCU数据库的统计信息。

```python
result = await mcp_flash.get_mcu_database_info()
```

### 4. 增强的 `flash_firmware()`

现在支持自动检测和手动指定MCU系列。

#### 自动检测模式（推荐）
```python
result = await mcp_flash.flash_firmware(
    workspace="/path/to/project",
    hex_file="firmware.hex",
    auto_detect=True  # 自动检测MCU类型
)
```

#### 手动指定系列
```python
result = await mcp_flash.flash_firmware(
    workspace="/path/to/project",
    hex_file="firmware.hex",
    auto_detect=False,
    target_family="F4"  # 指定STM32F4系列
)
```

## 使用流程

### 1. 自动检测并烧录（最简单）

```python
# Flash MCP会自动检测MCU类型并选择正确的配置
result = await mcp_flash.flash_firmware(
    workspace="/path/to/project",
    auto_detect=True
)

if result["ok"]:
    print(f"✓ 烧录成功: {result['mcu_info']['name']}")
    print(f"  系列: {result['mcu_info']['family']}")
    print(f"  Flash: {result['mcu_info']['flash_kb']}KB")
else:
    print(f"✗ 烧录失败: {result['error']}")
```

### 2. 先检测再烧录

```python
# 第一步：检测MCU
detect_result = await mcp_flash.detect_mcu()

if detect_result["detected"]:
    mcu = detect_result["mcu_info"]
    print(f"检测到: {mcu['name']} ({mcu['family']})")
    
    # 第二步：烧录
    flash_result = await mcp_flash.flash_firmware(
        workspace="/path/to/project",
        target_family=mcu["family"].replace("STM32", "")
    )
```

### 3. 手动指定系列

当你知道目标MCU系列时，可以直接指定：

```python
# 烧录到STM32F4系列
result = await mcp_flash.flash_firmware(
    workspace="/path/to/project",
    auto_detect=False,
    target_family="F4"
)
```

## MCU数据库

### 数据结构

```python
MCUInfo:
    name: str           # MCU型号名称
    family: MCUFamily   # 系列枚举
    device_id: str      # IDCODE (如 "0x20036410")
    flash_size_kb: int  # Flash大小(KB)
    ram_size_kb: int    # RAM大小(KB)
    core: str           # 核心类型
    target_config: str  # OpenOCD目标配置
    description: str    # 描述
    max_clock_mhz: int  # 最大时钟频率
```

### 设备IDCODE

MCU通过IDCODE识别，这是每个MCU唯一的标识符：

- **STM32F1**: `0x20036xxx`
- **STM32F4**: `0x100164xx`, `0x100764xx`
- **STM32F7**: `0x100164xx`
- **STM32H7**: `0x100064xx`

## 扩展MCU数据库

要添加新的MCU支持，编辑 `mcp_flash/mcu_database.py`：

```python
STM32_MCU_DATABASE: Dict[str, MCUInfo] = {
    # 添加新的MCU
    "0xXXXXXXXX": MCUInfo(
        name="STM32FxxxXX",
        family=MCUFamily.F4,
        device_id="0xXXXXXXXX",
        flash_size_kb=512,
        ram_size_kb=128,
        core="Cortex-M4",
        target_config="stm32f4x.cfg",
        description="STM32FxxxXX - Description",
        max_clock_mhz=180
    ),
}
```

## 故障排查

### MCU无法识别

```bash
# 检查OpenOCD是否能读取设备ID
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "init" -c "exit"

# 查看输出中的 device id = 0xXXXXXXXX
```

如果设备ID不在数据库中，会显示：
```
⚠ 未知具体型号，使用系列默认配置
```

这通常也能正常工作，因为系列配置（如stm32f4x.cfg）支持该系列的所有MCU。

### 烧录失败

1. **检查连接**：确保调试器和MCU正确连接
2. **检查电源**：目标电压应在正常范围（通常3.3V）
3. **检查目标配置**：尝试手动指定系列
4. **查看详细输出**：检查OpenOCD的错误信息

### 添加新的MCU系列支持

1. 确认OpenOCD支持该系列：
```bash
docker run --rm stm32-flash-toolchain:latest \
  ls /usr/share/openocd/scripts/target/ | grep stm32
```

2. 在 `mcu_database.py` 中添加系列到 `MCUFamily` 枚举

3. 在 `FAMILY_TARGET_MAP` 中添加系列到配置的映射

4. 添加该系列的具体MCU到 `STM32_MCU_DATABASE`

## 版本信息

- **Flash MCP版本**: 0.6.0
- **MCU数据库版本**: 1.0.0
- **新增功能**: 多目标支持、自动检测
- **支持系列**: F1, F4, F7, H7
- **支持MCU数**: 40+
