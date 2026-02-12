"""STM32 MCP Build Server - FastMCP Server Implementation

提供STM32固件编译的MCP工具，通过Docker容器实现安全的编译环境。
"""

import os
import sys
import subprocess
import tempfile
import shutil
from pathlib import Path
from typing import Optional, List, Dict, Any
from datetime import datetime

from fastmcp import FastMCP
from pydantic import Field

# 初始化MCP服务器
mcp = FastMCP("stm32-build-server")

# 配置常量
DEFAULT_DOCKER_IMAGE = "stm32-toolchain:latest"
DEFAULT_TIMEOUT = 600
MAX_TIMEOUT = 3600
ALLOWED_ROOTS = [
    "/home",
    "/Users",
    "/tmp",
    "/workspace",
    str(Path.home()),
]


def _validate_workspace(workspace: str) -> Path:
    """验证workspace路径安全
    
    Args:
        workspace: 工作目录路径
        
    Returns:
        验证后的Path对象
        
    Raises:
        ValueError: 路径不安全或不存在
    """
    path = Path(workspace).resolve()
    
    # 检查路径是否存在
    if not path.exists():
        raise ValueError(f"工作目录不存在: {workspace}")
    
    # 检查是否为目录
    if not path.is_dir():
        raise ValueError(f"工作目录必须是目录: {workspace}")
    
    # 安全检查：确保路径在允许的根目录下
    path_str = str(path)
    allowed = any(path_str.startswith(root) for root in ALLOWED_ROOTS)
    
    if not allowed:
        # 额外检查：是否是相对路径或当前目录的子目录
        try:
            Path.cwd().joinpath(workspace).resolve().relative_to(Path.cwd())
            allowed = True
        except ValueError:
            pass
    
    if not allowed:
        raise ValueError(f"工作目录不在允许的路径范围内: {workspace}")
    
    return path


def _get_build_script_path() -> str:
    """获取build.sh脚本的绝对路径
    
    Returns:
        build.sh脚本的绝对路径
    """
    # 首先尝试从包目录查找
    module_dir = Path(__file__).parent.parent
    script_path = module_dir / "tools" / "build.sh"
    
    if script_path.exists():
        return str(script_path.resolve())
    
    # 尝试从当前工作目录查找
    cwd_script = Path.cwd() / "tools" / "build.sh"
    if cwd_script.exists():
        return str(cwd_script.resolve())
    
    # 使用容器内的默认路径
    return "/tools/build.sh"


@mcp.tool()
def build_firmware(
    workspace: str,
    project_subdir: str = "",
    clean: bool = True,
    jobs: int = 4,
    make_target: str = "all",
    timeout_sec: int = 600,
    max_log_tail_kb: int = 96,
    docker_image: str = "stm32-toolchain:latest",
) -> Dict[str, Any]:
    """编译STM32固件
    
    使用Docker容器编译STM32工程，返回编译结果和错误信息。
    源码以只读方式挂载，编译产物输出到workspace/out目录。
    
    Args:
        workspace: 工程根目录绝对路径，必须包含Makefile
        project_subdir: Makefile所在子目录（相对于workspace）
        clean: 是否先执行make clean
        jobs: 并行编译任务数（推荐4-8）
        make_target: make目标（默认all）
        timeout_sec: 编译超时秒数（默认600，最大3600）
        max_log_tail_kb: 返回日志尾部大小KB（默认96）
        docker_image: Docker镜像名称
    
    Returns:
        {
            "ok": bool,              # 编译是否成功
            "exit_code": int,        # make退出码
            "workspace": str,        # 工作目录
            "outdir": str,           # 输出目录
            "artifacts": List[str],  # 编译产物文件列表
            "errors": List[dict],    # 解析后的错误列表
            "log_tail": str,         # 日志尾部
            "docker_tail": str,      # Docker输出尾部
            "duration_sec": float,   # 编译耗时
        }
    """
    start_time = datetime.now()
    
    try:
        # 参数验证
        if jobs < 1 or jobs > 32:
            return {"ok": False, "error": "jobs必须在1-32之间"}
        
        if timeout_sec < 10 or timeout_sec > MAX_TIMEOUT:
            return {"ok": False, "error": f"timeout_sec必须在10-{MAX_TIMEOUT}之间"}
        
        # 验证workspace路径
        try:
            workspace_path = _validate_workspace(workspace)
        except ValueError as e:
            return {"ok": False, "error": str(e)}
        
        # 确定工程路径
        if project_subdir:
            project_path = workspace_path / project_subdir
        else:
            project_path = workspace_path
        
        # 检查Makefile是否存在
        makefile_path = project_path / "Makefile"
        if not makefile_path.exists():
            return {
                "ok": False, 
                "error": f"未找到Makefile: {makefile_path}"
            }
        
        # 创建输出目录
        outdir = workspace_path / "out"
        outdir.mkdir(exist_ok=True)
        
        # 获取build.sh路径
        build_script = _get_build_script_path()
        
        # 构建Docker命令
        docker_cmd = [
            "docker", "run", "--rm",
            "--network=none",  # 禁用网络
            "-v", f"{workspace_path}:/src:ro",  # 只读挂载源码
            "-v", f"{outdir}:/out:rw",  # 可写挂载输出目录
            "-v", f"{build_script}:/tools/build.sh:ro",  # 挂载build脚本
            "-e", f"CLEAN={1 if clean else 0}",
            "-e", f"JOBS={jobs}",
            "-e", f"MAKE_TARGET={make_target}",
            "-e", "PROJECT_SUBDIR=" + (project_subdir if project_subdir else ""),
            docker_image,
            "bash", "/tools/build.sh"
        ]
        
        # 执行编译
        result = subprocess.run(
            docker_cmd,
            capture_output=True,
            text=True,
            timeout=timeout_sec
        )
        
        # 计算耗时
        duration = (datetime.now() - start_time).total_seconds()
        
        # 收集编译产物
        artifacts = []
        artifacts_dir = outdir / "artifacts"
        if artifacts_dir.exists():
            for ext in [".elf", ".hex", ".bin", ".map"]:
                for file in artifacts_dir.glob(f"*{ext}"):
                    artifacts.append(str(file.relative_to(workspace_path)))
        
        # 读取日志
        log_tail = ""
        build_log = outdir / "build.log"
        if build_log.exists():
            log_content = build_log.read_text()
            max_bytes = max_log_tail_kb * 1024
            if len(log_content) > max_bytes:
                log_tail = log_content[-max_bytes:]
            else:
                log_tail = log_content
        
        # 解析错误（简化版，完整版在Phase 3实现）
        errors = []
        if result.returncode != 0:
            for line in result.stderr.splitlines():
                if "error:" in line.lower():
                    errors.append({
                        "type": "compiler",
                        "severity": "error",
                        "message": line,
                        "raw": line
                    })
        
        return {
            "ok": result.returncode == 0,
            "exit_code": result.returncode,
            "workspace": str(workspace_path),
            "outdir": str(outdir),
            "artifacts": artifacts,
            "errors": errors,
            "log_tail": log_tail[-max_log_tail_kb*1024:],
            "docker_tail": result.stderr[-max_log_tail_kb*512:],
            "duration_sec": duration,
        }
        
    except subprocess.TimeoutExpired:
        return {
            "ok": False,
            "error": f"编译超时（>{timeout_sec}秒）",
            "duration_sec": timeout_sec,
        }
    except Exception as e:
        return {
            "ok": False,
            "error": f"编译异常: {str(e)}",
        }


@mcp.tool()
def check_environment() -> Dict[str, Any]:
    """检查编译环境是否就绪
    
    检查Docker、工具链、镜像等是否可用。
    
    Returns:
        {
            "ready": bool,
            "docker_available": bool,
            "image_exists": bool,
            "image_tag": str,
            "details": dict
        }
    """
    result = {
        "ready": False,
        "docker_available": False,
        "image_exists": False,
        "image_tag": DEFAULT_DOCKER_IMAGE,
        "details": {}
    }
    
    # 检查Docker
    try:
        docker_version = subprocess.run(
            ["docker", "--version"],
            capture_output=True,
            text=True,
            timeout=5
        )
        result["docker_available"] = docker_version.returncode == 0
        result["details"]["docker_version"] = docker_version.stdout.strip()
    except Exception as e:
        result["details"]["docker_error"] = str(e)
    
    # 检查镜像
    if result["docker_available"]:
        try:
            image_check = subprocess.run(
                ["docker", "images", "-q", DEFAULT_DOCKER_IMAGE],
                capture_output=True,
                text=True,
                timeout=5
            )
            result["image_exists"] = len(image_check.stdout.strip()) > 0
        except Exception as e:
            result["details"]["image_error"] = str(e)
    
    # 总体状态
    result["ready"] = result["docker_available"] and result["image_exists"]
    
    return result


@mcp.tool()
def get_build_info() -> Dict[str, Any]:
    """获取构建服务器信息
    
    Returns:
        服务器版本、配置等信息
    """
    return {
        "name": "stm32-build-server",
        "version": "0.2.0",
        "default_docker_image": DEFAULT_DOCKER_IMAGE,
        "default_timeout": DEFAULT_TIMEOUT,
        "allowed_roots": ALLOWED_ROOTS,
        "supported_mcu_families": ["STM32F1", "STM32F4", "STM32L4"],
    }


def main():
    """主函数 - 启动MCP服务器"""
    mcp.run()


if __name__ == "__main__":
    main()
