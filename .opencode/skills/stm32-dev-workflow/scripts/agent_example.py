#!/usr/bin/env python3
"""
STM32开发工作流 - Agent使用示例

展示如何在Agent中调用Build MCP和Flash MCP完成完整的开发工作流。

使用方法:
    1. 确保MCP Server已配置在 .opencode/mcp.json
    2. Agent通过 /使用 stm32-dev-workflow 加载此Skill
    3. 调用示例中的函数完成任务
"""

import asyncio
from typing import Dict, Any


class STM32DevelopmentAgent:
    """STM32开发Agent示例
    
    封装了完整的Build-Flash-Verify工作流。
    """
    
    def __init__(self, mcp_client):
        """
        Args:
            mcp_client: OpenCode MCP客户端实例
        """
        self.mcp = mcp_client
        self.workspace = ""
    
    async def build_firmware(
        self, 
        workspace: str,
        clean: bool = True,
        max_retries: int = 3
    ) -> Dict[str, Any]:
        """编译固件，支持自动重试
        
        Args:
            workspace: 工程根目录
            clean: 是否先make clean
            max_retries: 最大重试次数
            
        Returns:
            编译结果
        """
        self.workspace = workspace
        
        for attempt in range(max_retries):
            print(f"\n🔨 编译尝试 {attempt + 1}/{max_retries}...")
            
            result = await self.mcp.stm32_build.build_firmware(
                workspace=workspace,
                clean=clean,
                jobs=4,
                timeout_sec=600
            )
            
            if result.ok:
                print("✅ 编译成功!")
                print(f"   耗时: {result.duration_sec:.1f}秒")
                print(f"   产物: {', '.join(result.artifacts)}")
                return result
            
            # 编译失败
            print(f"❌ 编译失败 (退出码: {result.exit_code})")
            
            if result.errors:
                print(f"\n发现 {len(result.errors)} 个错误:")
                for error in result.errors[:5]:  # 显示前5个
                    print(f"  - {error.file}:{error.line}: {error.message}")
                
                # 尝试自动修复（简化版）
                if attempt < max_retries - 1:
                    print("\n🔧 尝试自动修复...")
                    await self._auto_fix_errors(result.errors)
            else:
                print(f"错误输出: {result.log_tail[:500]}")
                
        return result
    
    async def flash_firmware(
        self,
        workspace: str = "",
        auto_detect: bool = True,
        verify: bool = True
    ) -> Dict[str, Any]:
        """烧录固件
        
        Args:
            workspace: 工程目录（默认为上一次build的目录）
            auto_detect: 自动检测MCU
            verify: 验证烧录
            
        Returns:
            烧录结果
        """
        workspace = workspace or self.workspace
        
        print("\n🔌 检查烧录器...")
        health = await self.mcp.stm32_flash.health_check()
        
        if not health.local_available:
            print("❌ 未找到本地ST-Link")
            print(f"   建议: {health.recommendation}")
            return {"ok": False, "error": "No flasher available"}
        
        print("✅ 发现ST-Link")
        
        if auto_detect:
            print("\n🔍 检测MCU...")
            detection = await self.mcp.stm32_flash.detect_mcu()
            if detection.detected:
                print(f"✅ 检测到: {detection.get('name', 'Unknown')}")
                print(f"   设备ID: {detection.device_id}")
            else:
                print("⚠️  未检测到MCU")
        
        print("\n📤 开始烧录...")
        result = await self.mcp.stm32_flash.flash_firmware(
            workspace=workspace,
            auto_detect=auto_detect,
            verify=verify,
            prefer_local=True
        )
        
        if result.ok:
            print("✅ 烧录成功!")
            if result.mcu_info:
                mcu = result.mcu_info
                print(f"   MCU: {mcu.get('name', 'Unknown')}")
                print(f"   Flash: {mcu.get('flash_kb', '?')}KB")
                print(f"   RAM: {mcu.get('ram_kb', '?')}KB")
            print(f"   耗时: {result.duration_sec:.1f}秒")
        else:
            print(f"❌ 烧录失败: {result.get('error', 'Unknown error')}")
        
        return result
    
    async def build_and_flash(
        self,
        workspace: str,
        clean: bool = True
    ) -> Dict[str, Any]:
        """编译并烧录
        
        完整的工作流：编译 → 烧录
        
        Args:
            workspace: 工程目录
            clean: 是否clean
            
        Returns:
            最终结果
        """
        # Step 1: 编译
        build = await self.build_firmware(workspace, clean)
        if not build.ok:
            return {
                "ok": False,
                "stage": "build",
                "error": "Build failed",
                "details": build
            }
        
        # Step 2: 烧录
        flash = await self.flash_firmware(workspace)
        if not flash.ok:
            return {
                "ok": False,
                "stage": "flash",
                "error": "Flash failed",
                "details": flash
            }
        
        return {
            "ok": True,
            "message": "Build and flash successful",
            "build_duration": build.duration_sec,
            "flash_duration": flash.duration_sec,
            "mcu_info": flash.get("mcu_info")
        }
    
    async def _auto_fix_errors(self, errors):
        """自动修复编译错误（简化示例）
        
        实际实现中，这里会：
        1. 分析错误类型
        2. 读取源文件
        3. 应用修复
        4. 保存文件
        """
        # 示例：简单的错误修复逻辑
        for error in errors:
            if "undefined reference" in error.message.lower():
                print(f"   修复: 添加缺失的函数定义 {error.message}")
                # 实际这里会修改代码
            elif "implicit declaration" in error.message.lower():
                print(f"   修复: 添加头文件包含 {error.message}")
                # 实际这里会修改代码
        
        print("   已应用修复，重新编译...")


# 使用示例
async def example_usage():
    """示例：如何在Agent中使用"""
    
    # 假设我们有MCP客户端（实际由OpenCode Agent提供）
    class MockMCP:
        class stm32_build:
            @staticmethod
            async def build_firmware(**kwargs):
                # 模拟编译成功
                class Result:
                    ok = True
                    duration_sec = 15.3
                    artifacts = ["firmware.hex"]
                    errors = []
                return Result()
        
        class stm32_flash:
            @staticmethod
            async def health_check():
                class Health:
                    local_available = True
                    recommendation = "使用本地ST-Link"
                return Health()
            
            @staticmethod
            async def detect_mcu():
                class Detection:
                    detected = True
                    device_id = "0x20036410"
                return Detection()
            
            @staticmethod
            async def flash_firmware(**kwargs):
                class Result:
                    ok = True
                    duration_sec = 5.2
                    mcu_info = {
                        "name": "STM32F103C8",
                        "flash_kb": 64,
                        "ram_kb": 20
                    }
                return Result()
    
    # 创建Agent
    agent = STM32DevelopmentAgent(MockMCP())
    
    # 示例1: 只编译
    print("=" * 60)
    print("示例1: 编译项目")
    print("=" * 60)
    result = await agent.build_firmware(
        workspace="./Test_Data/Elder_Lifter_STM32_V1.32/Elder_Lifter_STM32"
    )
    
    # 示例2: 只烧录
    print("\n" + "=" * 60)
    print("示例2: 烧录固件")
    print("=" * 60)
    result = await agent.flash_firmware()
    
    # 示例3: 编译+烧录
    print("\n" + "=" * 60)
    print("示例3: 编译并烧录")
    print("=" * 60)
    result = await agent.build_and_flash(
        workspace="./Test_Data/Elder_Lifter_STM32_V1.32/Elder_Lifter_STM32"
    )
    
    if result["ok"]:
        print(f"\n🎉 {result['message']}")
        print(f"   编译耗时: {result['build_duration']:.1f}秒")
        print(f"   烧录耗时: {result['flash_duration']:.1f}秒")


if __name__ == "__main__":
    print("STM32开发工作流 - Agent使用示例")
    print("=" * 60)
    print()
    print("此脚本展示了如何在Agent中调用Build MCP和Flash MCP")
    print()
    print("实际使用时，Agent应该:")
    print("  1. 通过 /使用 stm32-dev-workflow 加载Skill")
    print("  2. 使用 self.mcp.stm32_build 调用Build MCP")
    print("  3. 使用 self.mcp.stm32_flash 调用Flash MCP")
    print()
    
    # 运行示例
    asyncio.run(example_usage())
