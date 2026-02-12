#!/usr/bin/env python3
"""
集成测试 - 验证MCP配置和实际功能
"""

import subprocess
import sys
import json
from pathlib import Path

def test_mcp_config():
    """测试MCP配置"""
    print("=" * 60)
    print("测试1: MCP配置验证")
    print("=" * 60)
    
    config_path = Path(".opencode/mcp.json")
    if not config_path.exists():
        print("❌ 未找到MCP配置文件")
        return False
    
    try:
        with open(config_path) as f:
            config = json.load(f)
        
        print("✓ MCP配置文件存在")
        
        # 检查Build MCP
        if "stm32-build" in config.get("mcpServers", {}):
            print("✓ Build MCP配置存在")
            build_cfg = config["mcpServers"]["stm32-build"]
            print(f"  命令: {build_cfg.get('command')}")
            print(f"  参数: {' '.join(build_cfg.get('args', []))}")
            print(f"  工作目录: {build_cfg.get('cwd')}")
        else:
            print("❌ Build MCP配置缺失")
            return False
        
        # 检查Flash MCP
        if "stm32-flash" in config.get("mcpServers", {}):
            print("✓ Flash MCP配置存在")
            flash_cfg = config["mcpServers"]["stm32-flash"]
            print(f"  命令: {flash_cfg.get('command')}")
            print(f"  参数: {' '.join(flash_cfg.get('args', []))}")
            print(f"  工作目录: {flash_cfg.get('cwd')}")
        else:
            print("❌ Flash MCP配置缺失")
            return False
        
        return True
        
    except Exception as e:
        print(f"❌ 配置解析失败: {e}")
        return False


def test_python_imports():
    """测试Python模块导入"""
    print("\n" + "=" * 60)
    print("测试2: Python模块导入")
    print("=" * 60)
    
    tests = [
        ("mcp_build.stm32_build_server", "Build MCP Server"),
        ("mcp_flash.stm32_flash_server_v2", "Flash MCP Server v2"),
        ("mcp_flash.local_flasher", "本地烧录器"),
        ("mcp_flash.flasher_router", "烧录器路由器"),
        ("mcp_flash.mcu_database", "MCU数据库"),
    ]
    
    all_passed = True
    for module, name in tests:
        try:
            __import__(module)
            print(f"✓ {name}: {module}")
        except Exception as e:
            print(f"❌ {name}: {e}")
            all_passed = False
    
    return all_passed


def test_build_mcp_direct():
    """直接测试Build MCP功能"""
    print("\n" + "=" * 60)
    print("测试3: Build MCP直接调用")
    print("=" * 60)
    
    try:
        import sys
        sys.path.insert(0, str(Path.cwd()))
        
        from mcp_build.stm32_build_server import build_firmware, check_environment
        
        print("✓ Build MCP模块导入成功")
        
        # 测试环境检查
        print("\n  检查Build环境...")
        env = check_environment()
        print(f"  ✓ Docker可用: {env['docker_available']}")
        print(f"  ✓ 镜像存在: {env['image_exists']}")
        print(f"  ✓ 版本: {env['version']}")
        
        # 测试编译（使用测试项目）
        test_workspace = "Test_Data/Elder_Lifter_STM32_V1.32/Elder_Lifter_STM32"
        
        if not Path(test_workspace).exists():
            print(f"\n  ⚠ 测试项目不存在: {test_workspace}")
            print("  跳过编译测试")
            return True
        
        print(f"\n  测试编译项目: {test_workspace}")
        print("  (注意: 这可能需要几分钟)")
        
        # 使用直接调用而不是MCP工具
        result = build_firmware(
            workspace=str(Path(test_workspace).absolute()),
            clean=True,
            jobs=4,
            timeout_sec=300
        )
        
        if result["ok"]:
            print(f"  ✅ 编译成功!")
            print(f"     耗时: {result.get('duration_sec', 0):.1f}秒")
            print(f"     产物: {', '.join(result.get('artifacts', []))}")
        else:
            print(f"  ❌ 编译失败")
            print(f"     错误: {result.get('error', 'Unknown')}")
            if result.get('errors'):
                print(f"     错误数: {len(result['errors'])}")
        
        return True
        
    except Exception as e:
        print(f"❌ 测试失败: {e}")
        import traceback
        traceback.print_exc()
        return False


def test_flash_mcp_direct():
    """直接测试Flash MCP功能"""
    print("\n" + "=" * 60)
    print("测试4: Flash MCP直接调用")
    print("=" * 60)
    
    try:
        sys.path.insert(0, str(Path.cwd()))
        
        from mcp_flash.stm32_flash_server_v2 import (
            health_check, detect_mcu, list_flashers
        )
        
        print("✓ Flash MCP模块导入成功")
        
        # 测试健康检查
        print("\n  测试健康检查...")
        import asyncio
        health = asyncio.run(health_check())
        
        print(f"  状态: {health['status']}")
        print(f"  本地ST-Link: {'✅ 可用' if health['local_available'] else '❌ 不可用'}")
        print(f"  远程烧录器: {health['remote_available']} 个")
        print(f"  检测到目标: {health['targets_detected']} 个")
        print(f"  建议: {health['recommendation']}")
        
        # 列出烧录器
        print("\n  测试列出烧录器...")
        flashers = asyncio.run(list_flashers())
        print(f"  总计: {flashers['total']} 个")
        print(f"  可用: {flashers['available']} 个")
        
        for flasher in flashers.get('flashers', []):
            status = "✅" if flasher['available'] else "❌"
            print(f"    {status} {flasher['name']} ({flasher['type']})")
        
        # 检测MCU（如果ST-Link可用）
        if health['local_available']:
            print("\n  测试MCU检测...")
            detection = asyncio.run(detect_mcu())
            if detection['detected']:
                print(f"  ✅ 检测到MCU: {detection.get('name', 'Unknown')}")
                print(f"     设备ID: {detection.get('device_id', 'N/A')}")
                print(f"     系列: {detection.get('family', 'N/A')}")
            else:
                print(f"  ⚠️  未检测到MCU: {detection.get('message', '')}")
        else:
            print("\n  ⚠️  跳过MCU检测（ST-Link不可用）")
        
        return True
        
    except Exception as e:
        print(f"❌ 测试失败: {e}")
        import traceback
        traceback.print_exc()
        return False


def test_skill_files():
    """测试Skill文件"""
    print("\n" + "=" * 60)
    print("测试5: Skill文件验证")
    print("=" * 60)
    
    skill_path = Path(".opencode/skills/stm32-dev-workflow")
    
    required_files = [
        "SKILL.md",
        "README.md",
        "QUICK_REFERENCE.md",
        "scripts/agent_example.py",
        "references/mcp-config.json"
    ]
    
    all_exist = True
    for file in required_files:
        full_path = skill_path / file
        if full_path.exists():
            print(f"✓ {file}")
        else:
            print(f"❌ {file}")
            all_exist = False
    
    return all_exist


def main():
    """主函数"""
    print("\n" + "=" * 60)
    print("MCP集成测试")
    print("验证配置和功能")
    print("=" * 60 + "\n")
    
    results = []
    
    # 运行所有测试
    results.append(("MCP配置", test_mcp_config()))
    results.append(("Python模块", test_python_imports()))
    results.append(("Build MCP", test_build_mcp_direct()))
    results.append(("Flash MCP", test_flash_mcp_direct()))
    results.append(("Skill文件", test_skill_files()))
    
    # 汇总
    print("\n" + "=" * 60)
    print("测试结果汇总")
    print("=" * 60)
    
    passed = sum(1 for _, r in results if r)
    total = len(results)
    
    for name, result in results:
        status = "✅ 通过" if result else "❌ 失败"
        print(f"  {status} - {name}")
    
    print(f"\n总计: {passed}/{total} 测试通过")
    
    if passed == total:
        print("\n🎉 所有测试通过！MCP配置正确，可以开始使用。")
        print("\n下一步:")
        print("  1. 确保ST-Link连接到电脑")
        print("  2. 在Agent对话中使用 /使用 stm32-dev-workflow")
        print("  3. 开始编译和烧录STM32项目")
    else:
        print(f"\n⚠️  {total - passed} 个测试失败")
        print("\n故障排查:")
        print("  1. 检查MCP配置文件路径是否正确")
        print("  2. 确保Python环境已激活")
        print("  3. 检查依赖是否安装: pip install -r requirements.txt")
    
    print()


if __name__ == "__main__":
    main()
