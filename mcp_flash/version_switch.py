"""版本切换器 - 在旧版和新版Flash MCP之间切换

使用方法:
    # 切换到新版 (Phase 1)
    from mcp_flash.version_switch import use_v2
    use_v2()
    
    # 切换回旧版
    from mcp_flash.version_switch import use_v1
    use_v1()
"""

import sys
from pathlib import Path

# 当前版本
_current_version = "v1"  # 默认使用旧版

def use_v1():
    """使用旧版Flash MCP (v0.6.0及以下)"""
    global _current_version
    _current_version = "v1"
    
    # 确保导入的是旧版模块
    if "mcp_flash.stm32_flash_server" in sys.modules:
        del sys.modules["mcp_flash.stm32_flash_server"]
    if "mcp_flash" in sys.modules:
        del sys.modules["mcp_flash"]
    
    print("[VersionSwitch] 已切换到旧版 (v1)")
    print("  使用: stm32_flash_server.py")


def use_v2():
    """使用新版Flash MCP (v0.7.0+)"""
    global _current_version
    _current_version = "v2"
    
    # 将v2模块路径添加到系统路径
    mcp_flash_dir = Path(__file__).parent
    v2_path = mcp_flash_dir / "stm32_flash_server_v2.py"
    
    if v2_path.exists():
        # 通过重命名导入的方式使用v2
        import importlib.util
        spec = importlib.util.spec_from_file_location(
            "mcp_flash.stm32_flash_server", 
            str(v2_path)
        )
        module = importlib.util.module_from_spec(spec)
        sys.modules["mcp_flash.stm32_flash_server"] = module
        spec.loader.exec_module(module)
        
        print("[VersionSwitch] 已切换到新版 (v2)")
        print("  使用: stm32_flash_server_v2.py")
        print("  特性: 统一接口、智能路由、Phase 1本地支持")
    else:
        print(f"[VersionSwitch] 错误: 找不到 {v2_path}")


def get_current_version() -> str:
    """获取当前版本"""
    return _current_version


def is_v2() -> bool:
    """是否使用v2"""
    return _current_version == "v2"


# 自动检测版本
def auto_detect():
    """自动检测并使用最新版本
    
    如果v2存在则使用v2，否则使用v1
    """
    mcp_flash_dir = Path(__file__).parent
    v2_path = mcp_flash_dir / "stm32_flash_server_v2.py"
    
    if v2_path.exists():
        use_v2()
    else:
        use_v1()


# 默认行为: 自动检测
auto_detect()
