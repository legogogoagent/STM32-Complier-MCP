#!/usr/bin/env python3
"""Flash MCP Docker环境测试脚本"""

import subprocess
import json
import os

def test_docker_environment():
    """测试Docker环境配置"""
    print("=" * 60)
    print("STM32 Flash MCP - Docker环境测试")
    print("=" * 60)
    print()
    
    # 1. 检查Docker安装
    print("✓ 检查Docker安装...")
    try:
        result = subprocess.run(
            ["docker", "--version"],
            capture_output=True,
            text=True,
            timeout=5
        )
        if result.returncode == 0:
            print(f"  Docker版本: {result.stdout.strip()}")
        else:
            print("  ✗ Docker未安装或无法访问")
            return False
    except Exception as e:
        print(f"  ✗ Docker检查失败: {e}")
        return False
    
    # 2. 检查镜像
    print("\n✓ 检查Docker镜像...")
    try:
        result = subprocess.run(
            ["docker", "images", "-q", "stm32-flash-toolchain:latest"],
            capture_output=True,
            text=True,
            timeout=5
        )
        if result.stdout.strip():
            print(f"  ✓ 镜像已存在: stm32-flash-toolchain:latest")
            
            # 获取镜像详情
            detail = subprocess.run(
                ["docker", "images", "stm32-flash-toolchain:latest", "--format", "table {{.Repository}}\t{{.Tag}}\t{{.Size}}"],
                capture_output=True,
                text=True,
                timeout=5
            )
            for line in detail.stdout.strip().split('\n')[1:]:
                print(f"    {line}")
        else:
            print("  ✗ 镜像不存在")
            print("  请运行: docker build -f docker/flash.Dockerfile -t stm32-flash-toolchain:latest .")
            return False
    except Exception as e:
        print(f"  ✗ 镜像检查失败: {e}")
        return False
    
    # 3. 检查OpenOCD版本
    print("\n✓ 检查容器内OpenOCD版本...")
    try:
        result = subprocess.run(
            ["docker", "run", "--rm", "stm32-flash-toolchain:latest", "openocd", "--version"],
            capture_output=True,
            text=True,
            timeout=10
        )
        if result.returncode == 0:
            version_line = result.stdout.split('\n')[0]
            print(f"  {version_line}")
        else:
            print(f"  ✗ OpenOCD检查失败")
    except Exception as e:
        print(f"  ✗ OpenOCD检查失败: {e}")
    
    # 4. 检查USB规则
    print("\n✓ 检查USB规则配置...")
    rules_files = [
        "/etc/udev/rules.d/99-stlink.rules",
        "/etc/udev/rules.d/99-jlink.rules",
        "/etc/udev/rules.d/99-cmsis-dap.rules"
    ]
    configured = []
    for rules_file in rules_files:
        if os.path.exists(rules_file):
            configured.append(os.path.basename(rules_file))
            print(f"  ✓ {rules_file}")
    
    if not configured:
        print("  ⚠ 未配置USB规则")
        print("  请运行: sudo ./scripts/setup-usb-rules.sh")
    
    # 5. 测试USB设备访问
    print("\n✓ 测试Docker USB设备访问...")
    try:
        result = subprocess.run(
            ["docker", "run", "--rm", "--privileged", 
             "-v", "/dev/bus/usb:/dev/bus/usb",
             "stm32-flash-toolchain:latest",
             "lsusb"],
            capture_output=True,
            text=True,
            timeout=10
        )
        if result.returncode == 0:
            devices = result.stdout.strip().split('\n')
            print(f"  发现 {len(devices)} 个USB设备")
            
            # 查找调试器
            debuggers = [d for d in devices if any(x in d.lower() for x in ['stlink', 'j-link', 'cmsis', 'debug', '0483', '1366'])]
            if debuggers:
                print(f"  发现潜在调试器:")
                for d in debuggers:
                    print(f"    {d}")
            else:
                print("  未连接调试器（请连接ST-Link/J-Link并重新插拔）")
        else:
            print(f"  ⚠ USB设备访问测试失败")
    except Exception as e:
        print(f"  ⚠ USB设备访问测试失败: {e}")
    
    # 6. 检查烧录脚本
    print("\n✓ 检查烧录脚本...")
    try:
        result = subprocess.run(
            ["docker", "run", "--rm", "stm32-flash-toolchain:latest", 
             "cat", "/usr/local/bin/flash.sh"],
            capture_output=True,
            text=True,
            timeout=5
        )
        if result.returncode == 0:
            print("  ✓ flash.sh 脚本已安装")
        else:
            print("  ✗ flash.sh 脚本未找到")
    except Exception as e:
        print(f"  ✗ 脚本检查失败: {e}")
    
    print()
    print("=" * 60)
    print("✅ Docker环境测试完成")
    print("=" * 60)
    print()
    print("📋 使用说明:")
    print("   1. Docker模式烧录: 使用 mcp_flash.stm32_flash_server 中的")
    print("      flash_firmware_docker() 工具")
    print("   2. 或直接运行: docker-compose -f docker/docker-compose.yml --profile flash-once run flash-once")
    print("   3. 检查环境: 使用 check_docker_environment() 工具")
    print()
    
    return True


if __name__ == "__main__":
    test_docker_environment()
