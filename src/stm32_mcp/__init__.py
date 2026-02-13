"""STM32 MCP Server - AI-powered STM32 build and flash via Docker.

Usage:
    uvx stm32-mcp
"""

__version__ = "2.0.0"

from .server import mcp


def main() -> None:
    """Entry point for uvx / CLI."""
    mcp.run()
