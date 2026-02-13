"""STM32 MCP Server – unified build + flash tools.

Exposes MCP tools for:
  - build_firmware   – compile STM32 firmware inside Docker
  - flash_firmware   – flash .hex/.bin via local OpenOCD / ST-Link
  - detect_mcu       – read MCU IDCODE via OpenOCD
  - check_environment – verify Docker & toolchain readiness
  - parse_gcc_errors – parse raw GCC log into structured errors
  - get_server_info  – version / capabilities
"""

import subprocess
from datetime import datetime
from pathlib import Path
from typing import Any, Dict, List

from fastmcp import FastMCP

from .docker_runner import DockerRunner
from .gcc_parse import (
    errors_to_dict,
    format_error_for_display,
    get_error_summary,
    parse_build_log,
)

# ── MCP server instance ─────────────────────────────────────

mcp = FastMCP("stm32-mcp")

# ── Shared constants ─────────────────────────────────────────

_VERSION = "2.0.0"
_DEFAULT_IMAGE = DockerRunner.DEFAULT_IMAGE
_MAX_TIMEOUT = 3600

_ALLOWED_ROOTS = [
    "/home",
    "/Users",
    "/tmp",
    "/workspace",
    str(Path.home()),
]


# ── Helpers ──────────────────────────────────────────────────

def _validate_workspace(workspace: str) -> Path:
    """Resolve *workspace* and sanity-check it."""
    path = Path(workspace).resolve()
    if not path.exists():
        raise ValueError(f"Workspace does not exist: {workspace}")
    if not path.is_dir():
        raise ValueError(f"Workspace is not a directory: {workspace}")

    path_str = str(path)
    allowed = any(path_str.startswith(root) for root in _ALLOWED_ROOTS)
    if not allowed:
        try:
            Path.cwd().joinpath(workspace).resolve().relative_to(Path.cwd())
            allowed = True
        except ValueError:
            pass
    if not allowed:
        raise ValueError(f"Workspace not in allowed roots: {workspace}")
    return path


# ═══════════════════════════════════════════════════════════
#  BUILD TOOLS
# ═══════════════════════════════════════════════════════════

@mcp.tool()
def build_firmware(
    workspace: str,
    project_subdir: str = "",
    clean: bool = True,
    jobs: int = 4,
    make_target: str = "all",
    timeout_sec: int = 600,
    max_log_tail_kb: int = 96,
    docker_image: str = "",
) -> Dict[str, Any]:
    """Compile STM32 firmware inside a Docker container.

    Source code is mounted **read-only**; build artifacts are written to
    ``workspace/out/``.

    Args:
        workspace:       Project root directory (must contain a Makefile).
        project_subdir:  Sub-directory where the Makefile lives.
        clean:           Run ``make clean`` first.
        jobs:            Parallel make jobs (1-32).
        make_target:     Make target (default ``all``).
        timeout_sec:     Build timeout in seconds (10-3600).
        max_log_tail_kb: Max log tail to return (KB).
        docker_image:    Override default Docker image.

    Returns:
        ``{ok, exit_code, workspace, outdir, artifacts, errors, error_summary,
        log_tail, duration_sec}``
    """
    start = datetime.now()

    # ── parameter checks ──
    if not 1 <= jobs <= 32:
        return {"ok": False, "error": "jobs must be 1-32"}
    if not 10 <= timeout_sec <= _MAX_TIMEOUT:
        return {"ok": False, "error": f"timeout_sec must be 10-{_MAX_TIMEOUT}"}

    try:
        ws = _validate_workspace(workspace)
    except ValueError as exc:
        return {"ok": False, "error": str(exc)}

    project_path = ws / project_subdir if project_subdir else ws
    if not (project_path / "Makefile").exists():
        return {"ok": False, "error": f"Makefile not found in {project_path}"}

    # ── run build via Docker ──
    image = docker_image or _DEFAULT_IMAGE
    runner = DockerRunner(image=image)

    # Ensure Docker image is available
    img_status = runner.ensure_image()
    if not img_status["ok"]:
        return {"ok": False, "error": img_status["message"]}

    result = runner.run_build(
        workspace=str(ws),
        project_subdir=project_subdir,
        clean=clean,
        jobs=jobs,
        make_target=make_target,
        timeout_sec=timeout_sec,
    )

    duration = (datetime.now() - start).total_seconds()
    outdir = ws / "out"

    # ── collect artifacts ──
    artifacts: List[str] = []
    artifacts_dir = outdir / "artifacts"
    if artifacts_dir.exists():
        for ext in (".elf", ".hex", ".bin", ".map"):
            for f in artifacts_dir.glob(f"*{ext}"):
                artifacts.append(str(f.relative_to(ws)))

    # ── read log tail ──
    log_tail = ""
    build_log = outdir / "build.log"
    if build_log.exists():
        content = build_log.read_text(errors="replace")
        max_bytes = max_log_tail_kb * 1024
        log_tail = content[-max_bytes:] if len(content) > max_bytes else content

    # ── parse errors ──
    log_for_parse = ""
    if build_log.exists():
        log_for_parse = build_log.read_text(errors="replace")
    if not log_for_parse or result.get("exit_code", -1) != 0:
        log_for_parse += "\n" + result.get("stderr", "")

    errors: List[Dict[str, Any]] = []
    error_summary = None
    if log_for_parse.strip():
        parsed = parse_build_log(log_for_parse, str(ws))
        errors = errors_to_dict(parsed)
        error_summary = get_error_summary(parsed)

    return {
        "ok": result.get("ok", False),
        "exit_code": result.get("exit_code", -1),
        "workspace": str(ws),
        "outdir": str(outdir),
        "artifacts": artifacts,
        "errors": errors,
        "error_summary": error_summary,
        "log_tail": log_tail,
        "duration_sec": duration,
    }


@mcp.tool()
def check_environment() -> Dict[str, Any]:
    """Check whether Docker and the toolchain image are available.

    Returns:
        ``{ready, docker_available, docker_version, image_exists, image}``
    """
    runner = DockerRunner()
    docker_ok = runner.is_docker_available()
    image_ok = runner.is_image_available() if docker_ok else False

    return {
        "ready": docker_ok and image_ok,
        "docker_available": docker_ok,
        "docker_version": runner.docker_version(),
        "image_exists": image_ok,
        "image": runner.image,
        "hint": (
            None if (docker_ok and image_ok)
            else "Docker not found" if not docker_ok
            else f"Run: docker pull {runner.image}"
        ),
    }


@mcp.tool()
def parse_gcc_errors(
    log_content: str,
    workspace: str = "",
) -> Dict[str, Any]:
    """Parse a raw GCC / LD build log into structured error records.

    Args:
        log_content: Raw build log text.
        workspace:   Project root (used to normalise file paths).

    Returns:
        ``{ok, errors, summary, formatted, total}``
    """
    try:
        parsed = parse_build_log(log_content, workspace)
        return {
            "ok": True,
            "errors": errors_to_dict(parsed),
            "summary": get_error_summary(parsed),
            "formatted": [format_error_for_display(e) for e in parsed],
            "total": len(parsed),
        }
    except Exception as exc:
        return {"ok": False, "error": f"Parse failed: {exc}"}


# ═══════════════════════════════════════════════════════════
#  FLASH TOOLS
# ═══════════════════════════════════════════════════════════

@mcp.tool()
def flash_firmware(
    workspace: str,
    hex_file: str = "",
    programmer: str = "stlink",
    interface: str = "swd",
    target_cfg: str = "stm32f1x.cfg",
    verify: bool = True,
    reset: bool = True,
    timeout_sec: int = 120,
) -> Dict[str, Any]:
    """Flash firmware to an STM32 MCU via local OpenOCD / ST-Link.

    Args:
        workspace:   Project root (will look for hex in ``out/artifacts/``).
        hex_file:    Explicit hex/bin file path (relative to workspace).
        programmer:  ``stlink`` (default) or ``cmsis-dap``.
        interface:   ``swd`` (default) or ``jtag``.
        target_cfg:  OpenOCD target config (e.g. ``stm32f4x.cfg``).
        verify:      Verify after programming.
        reset:       Reset MCU after programming.
        timeout_sec: Timeout in seconds.

    Returns:
        ``{ok, exit_code, hex_file, stdout, stderr, device_id}``
    """
    start = datetime.now()

    try:
        ws = _validate_workspace(workspace)
    except ValueError as exc:
        return {"ok": False, "error": str(exc)}

    # ── locate hex file ──
    if hex_file:
        hex_path = ws / hex_file
    else:
        # Auto-discover in out/artifacts/
        hex_path = None
        artifacts_dir = ws / "out" / "artifacts"
        if artifacts_dir.exists():
            for ext in (".hex", ".bin"):
                candidates = list(artifacts_dir.glob(f"*{ext}"))
                if candidates:
                    hex_path = candidates[0]
                    break
        if hex_path is None:
            return {"ok": False, "error": "No hex/bin file found in out/artifacts/"}

    if not hex_path.exists():
        return {"ok": False, "error": f"File not found: {hex_path}"}

    # ── determine interface config ──
    if programmer == "stlink":
        interface_cfg = "interface/stlink.cfg"
    elif programmer == "cmsis-dap":
        interface_cfg = "interface/cmsis-dap.cfg"
    else:
        interface_cfg = f"interface/{programmer}.cfg"

    # ── build OpenOCD command ──
    ocd_cmd = [
        "openocd",
        "-f", interface_cfg,
        "-f", f"target/{target_cfg}",
        "-c", "init",
        "-c", "reset halt",
    ]

    # program command
    file_str = str(hex_path)
    if file_str.endswith(".hex"):
        ocd_cmd += ["-c", f"flash write_image erase {file_str}"]
    else:
        ocd_cmd += ["-c", f"flash write_image erase {file_str} 0x08000000"]

    if verify:
        ocd_cmd += ["-c", f"verify_image {file_str}"]
    if reset:
        ocd_cmd += ["-c", "reset run"]

    ocd_cmd += ["-c", "shutdown"]

    try:
        r = subprocess.run(
            ocd_cmd,
            capture_output=True,
            text=True,
            timeout=timeout_sec,
        )
        duration = (datetime.now() - start).total_seconds()

        # try to extract device id
        device_id = ""
        for line in r.stderr.splitlines():
            if "device id" in line.lower() or "idcode" in line.lower():
                device_id = line.strip()
                break

        return {
            "ok": r.returncode == 0,
            "exit_code": r.returncode,
            "hex_file": str(hex_path.relative_to(ws)),
            "stdout": r.stdout,
            "stderr": r.stderr,
            "device_id": device_id,
            "duration_sec": duration,
        }
    except FileNotFoundError:
        return {"ok": False, "error": "openocd not found. Install OpenOCD first."}
    except subprocess.TimeoutExpired:
        return {"ok": False, "error": f"Flash timed out after {timeout_sec}s"}
    except Exception as exc:
        return {"ok": False, "error": str(exc)}


@mcp.tool()
def detect_mcu() -> Dict[str, Any]:
    """Detect connected STM32 MCU via OpenOCD / ST-Link.

    Returns:
        ``{ok, device_id, stdout, stderr}``
    """
    ocd_cmd = [
        "openocd",
        "-f", "interface/stlink.cfg",
        "-f", "target/stm32f1x.cfg",
        "-c", "init",
        "-c", "shutdown",
    ]
    try:
        r = subprocess.run(ocd_cmd, capture_output=True, text=True, timeout=15)

        device_id = ""
        for line in r.stderr.splitlines():
            if "device id" in line.lower() or "idcode" in line.lower():
                device_id = line.strip()
                break

        return {
            "ok": r.returncode == 0,
            "device_id": device_id,
            "stdout": r.stdout,
            "stderr": r.stderr,
        }
    except FileNotFoundError:
        return {"ok": False, "error": "openocd not found. Install OpenOCD first."}
    except subprocess.TimeoutExpired:
        return {"ok": False, "error": "MCU detection timed out"}
    except Exception as exc:
        return {"ok": False, "error": str(exc)}


# ═══════════════════════════════════════════════════════════
#  INFO
# ═══════════════════════════════════════════════════════════

@mcp.tool()
def get_server_info() -> Dict[str, Any]:
    """Return server version and capability summary."""
    runner = DockerRunner()
    return {
        "name": "stm32-mcp",
        "version": _VERSION,
        "docker_image": runner.image,
        "tools": [
            "build_firmware",
            "flash_firmware",
            "detect_mcu",
            "check_environment",
            "parse_gcc_errors",
            "get_server_info",
        ],
        "supported_families": ["STM32F1", "STM32F4", "STM32L4", "STM32F7", "STM32H7"],
    }
