#!/usr/bin/env python3
"""多目标MCU烧录测试脚本

测试Flash MCP的多目标支持和自动检测功能
"""

import subprocess
import sys
import json

def test_mcu_detection():
    """测试MCU自动检测"""
    print("=" * 60)
    print("测试1: MCU自动检测")
    print("=" * 60)
    
    try:
        # 尝试检测连接的MCU
        result = subprocess.run(
            [
                "openocd",
                "-f", "interface/stlink.cfg",
                "-f", "target/stm32f1x.cfg",
                "-c", "init",
                "-c", "exit"
            ],
            capture_output=True,
            text=True,
            timeout=10
        )
        
        output = result.stdout + result.stderr
        
        # 解析设备ID
        import re
        match = re.search(r"device id\s*=\s*(0x[0-9a-fA-F]+)", output)
        if match:
            device_id = match.group(1)
            print(f"✓ 检测到设备ID: {device_id}")
            
            # 查询MCU数据库
            sys.path.insert(0, '.')
            from mcp_flash.mcu_database import get_mcu_info
            
            mcu_info = get_mcu_info(device_id)
            if mcu_info:
                print(f"✓ MCU型号: {mcu_info.name}")
                print(f"✓ 系列: {mcu_info.family.value}")
                print(f"✓ Flash: {mcu_info.flash_size_kb} KB")
                print(f"✓ RAM: {mcu_info.ram_size_kb} KB")
                print(f"✓ 核心: {mcu_info.core}")
                print(f"✓ 目标配置: {mcu_info.target_config}")
            else:
                print(f"⚠ 设备ID {device_id} 不在数据库中")
        else:
            print("⚠ 未检测到MCU，请连接调试器和目标板")
            print("  输出:", output[:500])
            
    except subprocess.TimeoutExpired:
        print("✗ 检测超时")
    except Exception as e:
        print(f"✗ 检测失败: {e}")
    
    print()


def test_mcu_database():
    """测试MCU数据库"""
    print("=" * 60)
    print("测试2: MCU数据库")
    print("=" * 60)
    
    sys.path.insert(0, '.')
    from mcp_flash.mcu_database import (
        list_supported_mcus, 
        get_supported_families,
        STM32_MCU_DATABASE
    )
    
    # 列出支持的系列
    families = get_supported_families()
    print(f"✓ 支持的系列数量: {len(families)}")
    for family in families:
        print(f"  - {family['name']}: {family['mcu_count']} 个MCU")
        print(f"    核心: {', '.join(family['cores'])}")
        print(f"    目标配置: {family['target_config']}")
    
    print()
    
    # 列出所有MCU
    mcus = list_supported_mcus()
    print(f"✓ MCU总数: {len(mcus)}")
    
    # 显示每个系列的第一个MCU作为示例
    shown_families = set()
    print("\n  MCU示例（每个系列一个）:")
    for mcu in mcus:
        if mcu['family'] not in shown_families:
            print(f"    - {mcu['name']} ({mcu['family']}): {mcu['flash_kb']}KB Flash, {mcu['core']}")
            shown_families.add(mcu['family'])
    
    print()


def test_target_configs():
    """测试目标配置"""
    print("=" * 60)
    print("测试3: OpenOCD目标配置")
    print("=" * 60)
    
    # 检查Docker镜像中的目标配置
    try:
        result = subprocess.run(
            ["docker", "run", "--rm", "stm32-flash-toolchain:latest",
             "ls", "/usr/share/openocd/scripts/target/"],
            capture_output=True,
            text=True,
            timeout=10
        )
        
        if result.returncode == 0:
            configs = result.stdout.strip().split('\n')
            stm32_configs = [c for c in configs if 'stm32' in c.lower()]
            print(f"✓ 找到 {len(stm32_configs)} 个STM32目标配置:")
            for cfg in sorted(stm32_configs):
                print(f"    - {cfg}")
        else:
            print(f"✗ 获取配置失败: {result.stderr}")
    except Exception as e:
        print(f"✗ 测试失败: {e}")
    
    print()


def test_flash_with_detection():
    """测试自动检测烧录"""
    print("=" * 60)
    print("测试4: 自动检测烧录流程")
    print("=" * 60)
    
    print("模拟烧录流程:")
    print("  1. 检测MCU类型 → 自动识别目标配置")
    print("  2. 使用正确的目标配置进行烧录")
    print()
    
    # 显示各系列的目标配置
    sys.path.insert(0, '.')
    from mcp_flash.mcu_database import FAMILY_TARGET_MAP, MCUFamily
    
    print("  系列 → 目标配置映射:")
    for family in MCUFamily:
        if family in FAMILY_TARGET_MAP:
            print(f"    - {family.value} → {FAMILY_TARGET_MAP[family]}")
    
    print()
    print("  使用说明:")
    print("    • 自动检测模式: flash_firmware(auto_detect=True)")
    print("    • 手动指定系列: flash_firmware(auto_detect=False, target_family='F4')")
    print()


def main():
    """主函数"""
    print("\n" + "=" * 60)
    print("STM32 Flash MCP - 多目标支持测试")
    print("=" * 60 + "\n")
    
    test_mcu_database()
    test_target_configs()
    test_mcu_detection()
    test_flash_with_detection()
    
    print("=" * 60)
    print("测试完成")
    print("=" * 60)
    print()
    print("📋 新增MCP工具:")
    print("  • detect_mcu() - 自动检测MCU类型")
    print("  • list_supported_mcus_tool() - 列出支持的MCU")
    print("  • get_mcu_database_info() - 获取数据库信息")
    print()
    print("  增强工具:")
    print("  • flash_firmware(auto_detect=True) - 自动检测并烧录")
    print("  • flash_firmware(target_family='F4') - 指定系列烧录")
    print()


if __name__ == "__main__":
    main()
