#!/usr/bin/env python3
"""
MCP配置验证和使用示例

此脚本验证MCP配置是否正确，并提供使用示例。
注意：实际运行时通过OpenCode Agent调用，不是直接调用。
"""

import json
from pathlib import Path
import sys

# 添加项目路径
sys.path.insert(0, str(Path(__file__).parent.parent))

def main():
    print("=" * 70)
    print(" MCP配置验证和技能使用指南")
    print("=" * 70)
    
    # 1. 验证配置文件
    print("\n📋 Step 1: 验证MCP配置文件")
    print("-" * 70)
    
    config_path = Path(".opencode/mcp.json")
    if config_path.exists():
        print(f"✅ MCP配置文件存在: {config_path.absolute()}")
        
        with open(config_path) as f:
            config = json.load(f)
        
        print("\n配置内容:")
        for name, server in config.get("mcpServers", {}).items():
            print(f"\n  📦 {name}:")
            print(f"     命令: {server.get('command')}")
            print(f"     参数: {' '.join(server.get('args', []))}")
            print(f"     工作目录: {server.get('cwd')}")
    else:
        print(f"❌ 未找到配置文件: {config_path}")
        print("   请确保配置文件在 .opencode/mcp.json")
        return
    
    # 2. 验证Skill
    print("\n\n📦 Step 2: 验证Skill安装")
    print("-" * 70)
    
    skill_path = Path(".opencode/skills/stm32-dev-workflow")
    if skill_path.exists():
        print(f"✅ Skill已安装: {skill_path.absolute()}")
        
        # 检查关键文件
        files = ["SKILL.md", "README.md", "QUICK_REFERENCE.md"]
        for file in files:
            if (skill_path / file).exists():
                print(f"   ✓ {file}")
            else:
                print(f"   ❌ {file}")
    else:
        print(f"⚠️  Skill未在默认位置安装")
        print(f"   路径: {skill_path}")
    
    # 3. 模块导入测试
    print("\n\n🐍 Step 3: Python模块验证")
    print("-" * 70)
    
    modules = [
        ("mcp_build.stm32_build_server", "Build MCP"),
        ("mcp_flash.stm32_flash_server_v2", "Flash MCP v2"),
        ("mcp_flash.local_flasher", "本地烧录器"),
        ("mcp_flash.flasher_router", "烧录器路由器"),
        ("mcp_flash.mcu_database", "MCU数据库"),
    ]
    
    for module, name in modules:
        try:
            __import__(module)
            print(f"✅ {name}: {module}")
        except ImportError as e:
            print(f"❌ {name}: {e}")
    
    # 4. 使用指南
    print("\n\n📖 Step 4: 使用指南")
    print("-" * 70)
    
    print("""
在OpenCode Agent中使用:

1. 加载Skill:
   /使用 stm32-dev-workflow

2. 编译项目:
   result = await self.mcp.stm32_build.build_firmware(
       workspace="./Test_Data/Elder_Lifter_STM32_V1.32/Elder_Lifter_STM32",
       clean=True
   )

3. 烧录固件:
   result = await self.mcp.stm32_flash.flash_firmware(
       workspace="./Test_Data/Elder_Lifter_STM32_V1.32/Elder_Lifter_STM32",
       auto_detect=True
   )

4. 健康检查:
   health = await self.mcp.stm32_flash.health_check()
   print(f"ST-Link可用: {health['local_available']}")

详细文档:
- .opencode/skills/stm32-dev-workflow/SKILL.md (完整文档)
- .opencode/skills/stm32-dev-workflow/QUICK_REFERENCE.md (快速参考)
    """)
    
    # 5. 验证Docker（Build MCP需要）
    print("\n\n🐳 Step 5: Docker环境检查")
    print("-" * 70)
    
    import subprocess
    try:
        result = subprocess.run(
            ["docker", "--version"],
            capture_output=True,
            text=True,
            timeout=5
        )
        if result.returncode == 0:
            print(f"✅ Docker已安装: {result.stdout.strip()}")
            
            # 检查镜像
            result = subprocess.run(
                ["docker", "images", "stm32-toolchain:latest", "-q"],
                capture_output=True,
                text=True
            )
            if result.stdout.strip():
                print("✅ stm32-toolchain镜像已存在")
            else:
                print("⚠️  stm32-toolchain镜像不存在")
                print("   请运行: docker build -f docker/Dockerfile -t stm32-toolchain:latest .")
        else:
            print("❌ Docker未安装或无法访问")
    except Exception as e:
        print(f"⚠️  Docker检查失败: {e}")
    
    # 6. 验证OpenOCD（Flash MCP需要）
    print("\n\n🔌 Step 6: OpenOCD检查")
    print("-" * 70)
    
    try:
        result = subprocess.run(
            ["openocd", "--version"],
            capture_output=True,
            text=True,
            timeout=5
        )
        if result.returncode == 0:
            version = result.stdout.split('\n')[0]
            print(f"✅ OpenOCD已安装: {version}")
        else:
            print("❌ OpenOCD未安装")
    except Exception as e:
        print(f"⚠️  OpenOCD检查失败: {e}")
    
    # 总结
    print("\n\n" + "=" * 70)
    print(" 配置验证完成")
    print("=" * 70)
    
    print("""
✅ 配置已就绪！

要在Agent中使用:
1. 启动OpenCode Agent在此项目目录
2. 在对话中输入: /使用 stm32-dev-workflow
3. Agent会自动加载Skill并提供STM32开发功能

注意事项:
- 确保ST-Link连接到电脑才能进行烧录
- 确保Docker运行正常才能进行编译
- 详细使用说明见 .opencode/skills/stm32-dev-workflow/SKILL.md
    """)

if __name__ == "__main__":
    main()
