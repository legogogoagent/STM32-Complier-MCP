#!/usr/bin/env python3
"""测试新版Flash MCP架构 - Phase 1: 本地ST-Link

运行所有测试以确保本地功能正常工作。
"""

import asyncio
import sys
from pathlib import Path

# 添加项目路径
sys.path.insert(0, str(Path(__file__).parent.parent))


async def test_base_classes():
    """测试基础类导入"""
    print("=" * 60)
    print("测试1: 基础类导入")
    print("=" * 60)
    
    try:
        from mcp_flash.base_flasher import (
            BaseFlasher, FlashResult, MCUTargetInfo, 
            FlasherType, SerialClient
        )
        print("✓ BaseFlasher 导入成功")
        print("✓ FlashResult 导入成功")
        print("✓ MCUTargetInfo 导入成功")
        print("✓ FlasherType 导入成功")
        print("✓ SerialClient 导入成功")
        return True
    except Exception as e:
        print(f"✗ 导入失败: {e}")
        return False


async def test_local_flasher():
    """测试本地烧录器"""
    print("\n" + "=" * 60)
    print("测试2: 本地烧录器 (LocalOpenOCDFlasher)")
    print("=" * 60)
    
    try:
        from mcp_flash.local_flasher import LocalOpenOCDFlasher
        print("✓ LocalOpenOCDFlasher 导入成功")
        
        # 创建实例
        flasher = LocalOpenOCDFlasher()
        print(f"✓ 烧录器实例创建成功")
        print(f"  名称: {flasher.name}")
        print(f"  类型: {flasher.flasher_type.value}")
        
        # 测试连接（OpenOCD可用性）
        print("\n  检查OpenOCD可用性...")
        connected = await flasher.connect()
        if connected:
            print("  ✓ OpenOCD已安装")
        else:
            print("  ⚠ OpenOCD未安装或无法访问")
        
        # 检查ST-Link可用性
        print("\n  检查ST-Link可用性...")
        available = await flasher.is_available()
        if available:
            print("  ✓ ST-Link已连接")
            
            # 检测MCU
            print("\n  检测MCU...")
            target = await flasher.detect_target()
            if target.connected:
                print(f"  ✓ MCU已连接")
                print(f"    设备ID: {target.device_id}")
                print(f"    型号: {target.name or 'Unknown'}")
                print(f"    系列: {target.family or 'Unknown'}")
            else:
                print("  ⚠ 未检测到MCU")
        else:
            print("  ⚠ ST-Link未连接")
        
        await flasher.disconnect()
        return True
        
    except Exception as e:
        print(f"✗ 测试失败: {e}")
        import traceback
        traceback.print_exc()
        return False


async def test_router():
    """测试路由器"""
    print("\n" + "=" * 60)
    print("测试3: 烧录器路由器 (FlasherRouter)")
    print("=" * 60)
    
    try:
        from mcp_flash.flasher_router import FlasherRouter
        print("✓ FlasherRouter 导入成功")
        
        # 创建路由器
        router = FlasherRouter()
        print("✓ 路由器实例创建成功")
        
        # 列出所有烧录器
        print("\n  列出所有烧录器...")
        all_flashers = await router.list_all()
        print(f"  找到 {len(all_flashers)} 个烧录器:")
        
        for flasher in all_flashers:
            status = "✓ 可用" if flasher.available else "✗ 不可用"
            print(f"    - {flasher.name} ({flasher.type.value}): {status}")
            if flasher.target_connected:
                print(f"      MCU: {flasher.target_info}")
        
        # 健康检查
        print("\n  执行健康检查...")
        health = await router.health_check()
        print(f"  本地可用: {health['local_available']}")
        print(f"  远程数量: {health['remote_count']}")
        print(f"  远程可用: {health['remote_available']}")
        print(f"  检测到目标: {health['targets_detected']}")
        print(f"  建议: {health['recommendation']}")
        
        # 获取最佳烧录器
        print("\n  获取最佳烧录器...")
        best = await router.get_best_flasher()
        if best:
            print(f"  ✓ 选择烧录器: {best.name} ({best.flasher_type.value})")
        else:
            print("  ⚠ 未找到可用烧录器")
        
        return True
        
    except Exception as e:
        print(f"✗ 测试失败: {e}")
        import traceback
        traceback.print_exc()
        return False


async def test_mcu_database():
    """测试MCU数据库"""
    print("\n" + "=" * 60)
    print("测试4: MCU数据库")
    print("=" * 60)
    
    try:
        from mcp_flash.mcu_database import (
            get_mcu_info, list_supported_mcus, 
            get_supported_families, STM32_MCU_DATABASE
        )
        print("✓ MCU数据库模块导入成功")
        
        # 列出支持的MCU
        mcus = list_supported_mcus()
        print(f"\n  支持的MCU总数: {len(mcus)}")
        
        # 列出系列
        families = get_supported_families()
        print(f"  支持的系列数: {len(families)}")
        print("\n  系列详情:")
        for family in families:
            print(f"    - {family['name']}: {family['mcu_count']} 个MCU")
            print(f"      核心: {', '.join(family['cores'])}")
            print(f"      目标配置: {family['target_config']}")
        
        # 测试查询
        print("\n  测试MCU查询...")
        mcu = get_mcu_info("0x20036410")  # STM32F103C8
        if mcu:
            print(f"  ✓ 找到MCU: {mcu.name}")
            print(f"    Flash: {mcu.flash_size_kb}KB")
            print(f"    RAM: {mcu.ram_size_kb}KB")
            print(f"    核心: {mcu.core}")
        
        return True
        
    except Exception as e:
        print(f"✗ 测试失败: {e}")
        import traceback
        traceback.print_exc()
        return False


async def test_server_v2():
    """测试新版MCP Server函数"""
    print("\n" + "=" * 60)
    print("测试5: 新版MCP Server (v0.7.0)")
    print("=" * 60)
    
    try:
        from mcp_flash.stm32_flash_server_v2 import VERSION
        from mcp_flash.stm32_flash_server_v2 import (
            _resolve_hex_path, health_check, list_flashers, detect_mcu
        )
        print("✓ 新版Server模块导入成功")
        
        # 显示版本
        print(f"\n  服务器版本: {VERSION}")
        print("  ✓ 新版本架构已就绪")
        
        # 测试内部函数
        print("\n  测试hex路径解析...")
        test_path = _resolve_hex_path(".", "")
        print(f"    结果: {test_path}")
        
        # 健康检查
        print("\n  执行健康检查...")
        health = await health_check()
        if health['ok']:
            print(f"  ✓ 健康状态: {health['status']}")
            print(f"    本地可用: {health['local_available']}")
            print(f"    远程可用: {health['remote_available']}")
        
        # 列出烧录器
        print("\n  列出烧录器...")
        flashers = await list_flashers()
        if flashers['ok']:
            print(f"  总计: {flashers['total']}")
            print(f"  可用: {flashers['available']}")
        
        # 检测MCU
        print("\n  检测MCU...")
        detection = await detect_mcu()
        if detection['ok']:
            if detection['detected']:
                print(f"  ✓ 检测到MCU: {detection.get('name', 'Unknown')}")
            else:
                print(f"  ⚠ 未检测到MCU: {detection.get('message', '')}")
        
        return True
        
    except Exception as e:
        print(f"✗ 测试失败: {e}")
        import traceback
        traceback.print_exc()
        return False


async def main():
    """主函数"""
    print("\n" + "=" * 60)
    print("Flash MCP Phase 1 测试套件")
    print("测试本地ST-Link功能")
    print("=" * 60 + "\n")
    
    results = []
    
    # 运行所有测试
    results.append(("基础类导入", await test_base_classes()))
    results.append(("本地烧录器", await test_local_flasher()))
    results.append(("烧录器路由器", await test_router()))
    results.append(("MCU数据库", await test_mcu_database()))
    results.append(("新版MCP Server", await test_server_v2()))
    
    # 汇总结果
    print("\n" + "=" * 60)
    print("测试结果汇总")
    print("=" * 60)
    
    passed = sum(1 for _, result in results if result)
    total = len(results)
    
    for name, result in results:
        status = "✅ 通过" if result else "❌ 失败"
        print(f"  {status} - {name}")
    
    print(f"\n总计: {passed}/{total} 测试通过")
    
    if passed == total:
        print("\n🎉 所有测试通过！Phase 1 本地功能已就绪。")
    else:
        print(f"\n⚠️  {total - passed} 个测试失败，请检查配置。")
    
    print("\n下一步:")
    print("  1. 修复失败的测试")
    print("  2. 运行实际烧录测试")
    print("  3. 准备 Phase 2 ESP32远程烧录")
    print()


if __name__ == "__main__":
    asyncio.run(main())
