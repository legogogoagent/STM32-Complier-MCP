# STM32 MCP

[![PyPI](https://img.shields.io/pypi/v/stm32-mcp)](https://pypi.org/project/stm32-mcp/)
[![Docker](https://img.shields.io/docker/v/legogogoagent/stm32-toolchain?label=docker)](https://hub.docker.com/r/legogogoagent/stm32-toolchain)
[![License](https://img.shields.io/badge/license-MIT-blue)](LICENSE)

> AI-powered STM32 development via Model Context Protocol (MCP)

Compile and flash STM32 firmware through AI agents with zero local toolchain installation.

## ✨ Features

- 🚀 **Zero Installation** - No ARM toolchain, no dependencies, just Docker
- 🤖 **AI-Native** - Designed for MCP-compatible agents (OpenCode, Claude, etc.)
- 🔧 **Auto-Fix** - Automatic Makefile repair (Windows paths, GCC options)
- 📦 **Pre-built Images** - Ready-to-use Docker images with GCC 12.3
- 🎯 **One Command** - `uvx stm32-mcp` and you're ready

## 🚀 Quick Start

### 1. Configure MCP Server

Create `.opencode/opencode.json` in your STM32 project:

```json
{
  "$schema": "https://opencode.ai/config.json",
  "mcp": {
    "stm32": {
      "type": "local",
      "command": ["uvx", "stm32-mcp"],
      "enabled": true
    }
  }
}
```

**Important**: Restart your agent after configuration!

### 2. Use via Agent

```
User: Compile this STM32 project

Agent:
  ✅ Found STM32 project
  🐳 Pulling Docker image (first time only)
  🔨 Building firmware...
  ✓ Compilation successful!
     - firmware.hex (145KB)
     - firmware.bin (52KB)
```

## 📋 Requirements

- Docker
- Python 3.10+ (with uv/uvx)

Optional for flashing:
- OpenOCD
- ST-Link or CMSIS-DAP debugger

## 🔧 Available Tools

### Build

```python
# Compile firmware
result = await mcp.stm32.build_firmware(
    workspace="/path/to/project",
    clean=True,
    jobs=4
)
```

### Flash

```python
# Flash to MCU
result = await mcp.stm32.flash_firmware(
    workspace="/path/to/project",
    programmer="stlink",  # or "cmsis-dap"
    verify=True
)
```

### Detect

```python
# Auto-detect connected MCU
result = await mcp.stm32.detect_mcu()
```

## 🛠️ Manual CLI Usage

```bash
# Install
pip install stm32-mcp

# Or use with uvx (no install needed)
uvx stm32-mcp

# Check environment
python -m stm32_mcp check_environment
```

## 🐳 Docker Images

```bash
# Pull pre-built image
docker pull legogogoagent/stm32-toolchain:12.3

# Tags available
# - :12.3 - GCC 12.3 (stable)
# - :latest - Latest release
```

## 📁 Project Structure

```
STM32_Complier_MCP/
├── src/stm32_mcp/          # MCP Server implementation
│   ├── server.py           # Main MCP server with tools
│   ├── docker_runner.py    # Docker image management
│   ├── gcc_parse.py        # GCC error parser
│   └── build.sh            # Container build script
├── docker/                 # Docker configurations
├── ESP32_STM32_Bridge/     # ESP32 remote flashing (optional)
├── Test_Data/              # Example STM32 projects
├── pyproject.toml          # Package configuration
└── README.md               # This file
```

## 🔍 How It Works

```
Agent (AI)
    ↓
MCP Protocol
    ↓
stm32-mcp (PyPI)
    ↓
Docker Container
    ↓
arm-none-eabi-gcc
    ↓
STM32 Firmware
```

1. Agent sends compile request via MCP
2. `stm32-mcp` pulls Docker image if needed
3. Container auto-fixes Makefile issues
4. GCC compiles firmware inside container
5. Results returned to Agent

## 🐛 Troubleshooting

### "MCP server 'stm32' not found"

**Cause**: Wrong config file path

**Fix**: Use `.opencode/opencode.json` (not `mcp.json`):
```json
{
  "mcp": {
    "stm32": {
      "type": "local",
      "command": ["uvx", "stm32-mcp"],
      "enabled": true
    }
  }
}
```
Then restart your agent.

### Docker pull fails

```bash
# Manual pull
docker pull legogogoagent/stm32-toolchain:12.3
```

### OpenOCD not found (for flashing)

```bash
# Ubuntu/Debian
sudo apt install openocd

# macOS
brew install openocd
```

## 📚 Documentation

- [Architecture](ARCHITECTURE.md) - Design decisions and v2.0 refactor
- [Skill Guide](.opencode/skills/stm32-dev-workflow/SKILL.md) - OpenCode integration
- [Changelog](CHANGELOG.md) - Version history

## 🤝 Contributing

1. Fork the repository
2. Create feature branch (`git checkout -b feature/amazing`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing`)
5. Open Pull Request

## 📄 License

MIT License - see [LICENSE](LICENSE) file

## 🔗 Links

- PyPI: https://pypi.org/project/stm32-mcp/
- Docker Hub: https://hub.docker.com/r/legogogoagent/stm32-toolchain
- Issues: https://github.com/legogogoagent/STM32-Complier-MCP/issues

---

<p align="center">
  Made with ❤️ for AI-powered embedded development
</p>
