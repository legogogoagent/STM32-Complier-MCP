# Flash MCP Docker化部署指南

## 概述

Flash MCP现已支持Docker容器化运行，通过USB设备透传实现对STM32 MCU的隔离烧录。

## 新功能

### 1. Docker镜像
- **镜像名称**: `stm32-flash-toolchain:latest`
- **基础镜像**: Ubuntu 24.04
- **包含工具**: OpenOCD 0.12.0, USB工具链
- **镜像大小**: 731MB

### 2. MCP工具增强

#### `flash_firmware_docker()`
在隔离的Docker容器中执行烧录操作：
```python
{
    "workspace": "/path/to/project",
    "hex_file": "Elder_Lifter_STM32.hex",  # 可选，自动查找
    "programmer": "stlink",
    "interface": "swd",
    "verify": True,
    "reset": True,
    "timeout_sec": 180,
    "docker_image": "stm32-flash-toolchain:latest"
}
```

返回结果：
```python
{
    "ok": True,
    "exit_code": 0,
    "device_id": "0x20036410",
    "duration_sec": 5.2,
    "docker_info": {
        "image": "stm32-flash-toolchain:latest",
        "privileged": True,
        "usb_passthrough": True
    }
}
```

#### `check_docker_environment()`
检查Docker环境是否就绪：
```python
{
    "ok": True,
    "docker_installed": True,
    "docker_version": "Docker version 29.2.1...",
    "image_exists": True,
    "usb_rules_configured": True,
    "details": {...}
}
```

## 部署步骤

### 1. 配置USB权限（仅需一次）

```bash
sudo ./scripts/setup-usb-rules.sh
```

此脚本会：
- 创建ST-Link USB规则
- 创建J-Link USB规则
- 创建CMSIS-DAP规则
- 将当前用户添加到plugdev组

### 2. 构建Docker镜像

```bash
docker build -f docker/flash.Dockerfile -t stm32-flash-toolchain:latest .
```

### 3. 验证环境

```bash
python3 scripts/test_docker_env.py
```

## 使用方式

### 方式1: 通过MCP工具调用

```python
# 检查环境
result = await mcp_flash.check_docker_environment()

# Docker模式烧录
result = await mcp_flash.flash_firmware_docker(
    workspace="/path/to/project",
    hex_file="Elder_Lifter_STM32.hex"
)
```

### 方式2: Docker Compose

```bash
# 启动持久化服务
docker-compose -f docker/docker-compose.yml --profile all up -d

# 一次性编译+烧录
docker-compose -f docker/docker-compose.yml --profile build-once run build-once
docker-compose -f docker/docker-compose.yml --profile flash-once run flash-once
```

### 方式3: 直接Docker命令

```bash
# 手动烧录
docker run --rm --privileged \
  -v /dev/bus/usb:/dev/bus/usb \
  -v ./out/artifacts:/out:ro \
  stm32-flash-toolchain:latest \
  flash.sh /out/Elder_Lifter_STM32.hex
```

## 架构对比

### 主机模式（原有）
```
Flash MCP → 主机OpenOCD → USB → MCU
```

### Docker模式（新增）
```
Flash MCP → Docker容器 → 容器OpenOCD → USB透传 → MCU
```

## 安全特性

- **网络隔离**: `--network none`
- **只读挂载**: hex文件以只读方式挂载
- **特权限制**: 仅USB设备需要特权模式
- **自动清理**: 容器运行后自动删除 (`--rm`)

## 支持的调试器

| 调试器 | 状态 | USB ID |
|--------|------|--------|
| ST-Link/V2 | ✅ 支持 | 0483:3748 |
| ST-Link/V2-1 | ✅ 支持 | 0483:374b |
| ST-Link/V3 | ✅ 支持 | 0483:374f |
| J-Link | ✅ 支持 | 1366:0101+ |
| CMSIS-DAP | ✅ 支持 | 0d28:0204 |

## 故障排查

### USB权限问题
```bash
# 检查规则是否加载
ls -la /etc/udev/rules.d/99-*.rules

# 重新加载规则
sudo udevadm control --reload-rules
sudo udevadm trigger

# 检查设备权限
ls -la /dev/bus/usb/*/*
```

### Docker访问问题
```bash
# 检查Docker组权限
groups $USER

# 重新登录或运行
newgrp docker
```

### 镜像不存在
```bash
# 构建镜像
docker build -f docker/flash.Dockerfile -t stm32-flash-toolchain:latest .
```

## 性能对比

| 模式 | 启动时间 | 烧录时间 | 隔离性 |
|------|----------|----------|--------|
| 主机模式 | 0s | ~5s | 低 |
| Docker模式 | ~2s | ~5s | 高 |

## 更新日志

### v0.5.0 - Flash MCP Docker支持
- ✅ 新增 `flash_firmware_docker()` 工具
- ✅ 新增 `check_docker_environment()` 工具
- ✅ 创建 `docker/flash.Dockerfile`
- ✅ 创建 `docker/docker-compose.yml`
- ✅ 创建 `scripts/setup-usb-rules.sh`
- ✅ 创建 `scripts/test_docker_env.py`
- ✅ 支持ST-Link/J-Link/CMSIS-DAP
- ✅ USB设备透传
