"""Docker container runner for STM32 compilation.

Manages Docker image lifecycle (check/pull/build) and runs
STM32 builds inside isolated containers.
"""

import importlib.resources
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any, Dict, Optional


class DockerRunner:
    """Manages Docker toolchain images and runs STM32 builds."""

    DEFAULT_IMAGE = "legogogoagent/stm32-toolchain:latest"

    def __init__(self, image: str = DEFAULT_IMAGE) -> None:
        self.image = image

    # ── Docker availability ──────────────────────────────────

    def is_docker_available(self) -> bool:
        """Return True if the Docker CLI is installed and the daemon is reachable."""
        try:
            r = subprocess.run(
                ["docker", "info"],
                capture_output=True,
                timeout=10,
            )
            return r.returncode == 0
        except (FileNotFoundError, subprocess.TimeoutExpired):
            return False

    def docker_version(self) -> str:
        """Return human-readable Docker version string."""
        try:
            r = subprocess.run(
                ["docker", "--version"],
                capture_output=True,
                text=True,
                timeout=5,
            )
            return r.stdout.strip() if r.returncode == 0 else ""
        except Exception:
            return ""

    # ── Image management ─────────────────────────────────────

    def is_image_available(self) -> bool:
        """Return True if *self.image* exists in the local Docker cache."""
        try:
            r = subprocess.run(
                ["docker", "images", "-q", self.image],
                capture_output=True,
                text=True,
                timeout=10,
            )
            return bool(r.stdout.strip())
        except Exception:
            return False

    def pull_image(self) -> bool:
        """Pull *self.image* from the registry.  Returns True on success."""
        try:
            print(f"Pulling Docker image {self.image} …", file=sys.stderr)
            r = subprocess.run(
                ["docker", "pull", self.image],
                capture_output=True,
                text=True,
                timeout=600,  # large images may take a while
            )
            return r.returncode == 0
        except Exception:
            return False

    def ensure_image(self) -> Dict[str, Any]:
        """Make sure the toolchain image is available.

        Strategy: check local cache → pull from registry.

        Returns a status dict ``{ok, source, message}``.
        """
        if self.is_image_available():
            return {"ok": True, "source": "local", "message": "Image already cached"}

        if self.pull_image():
            return {"ok": True, "source": "pulled", "message": f"Pulled {self.image}"}

        return {
            "ok": False,
            "source": "none",
            "message": (
                f"Cannot obtain image {self.image}.  "
                "Please run:  docker pull " + self.image
            ),
        }

    # ── Build execution ──────────────────────────────────────

    def get_build_script_path(self) -> str:
        """Return the absolute path to the *build.sh* bundled with this package."""
        ref = importlib.resources.files("stm32_mcp").joinpath("build.sh")
        # importlib.resources may return a Traversable; materialise to a real path
        with importlib.resources.as_file(ref) as p:
            return str(p)

    def run_build(
        self,
        workspace: str,
        project_subdir: str = "",
        clean: bool = True,
        jobs: int = 4,
        make_target: str = "all",
        timeout_sec: int = 600,
    ) -> Dict[str, Any]:
        """Run an STM32 build inside a Docker container.

        * *workspace* is mounted **read-only** at ``/src``.
        * A temporary ``out/`` directory is mounted **read-write** at ``/out``.
        * The bundled ``build.sh`` is mounted at ``/tools/build.sh``.

        Returns ``{ok, exit_code, outdir, stdout, stderr}``.
        """
        workspace_path = Path(workspace).resolve()
        if not workspace_path.is_dir():
            return {"ok": False, "exit_code": -1, "error": f"Not a directory: {workspace}"}

        # Ensure output directory
        outdir = workspace_path / "out"
        outdir.mkdir(exist_ok=True)

        # Locate bundled build script
        build_script = self.get_build_script_path()

        docker_cmd = [
            "docker", "run", "--rm",
            "--network=none",
            "-v", f"{workspace_path}:/src:ro",
            "-v", f"{outdir}:/out:rw",
            "-v", f"{build_script}:/tools/build.sh:ro",
            "-e", f"CLEAN={1 if clean else 0}",
            "-e", f"JOBS={jobs}",
            "-e", f"MAKE_TARGET={make_target}",
            "-e", f"PROJECT_SUBDIR={project_subdir}",
            self.image,
            "bash", "/tools/build.sh",
        ]

        try:
            result = subprocess.run(
                docker_cmd,
                capture_output=True,
                text=True,
                timeout=timeout_sec,
            )
            return {
                "ok": result.returncode == 0,
                "exit_code": result.returncode,
                "outdir": str(outdir),
                "stdout": result.stdout,
                "stderr": result.stderr,
            }
        except subprocess.TimeoutExpired:
            return {
                "ok": False,
                "exit_code": -1,
                "error": f"Build timed out after {timeout_sec}s",
            }
        except Exception as exc:
            return {
                "ok": False,
                "exit_code": -1,
                "error": str(exc),
            }
