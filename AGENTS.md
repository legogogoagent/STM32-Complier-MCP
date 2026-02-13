# AGENTS.md - STM32 MCP Project Guide

**Project**: STM32 MCP - AI-powered embedded development  
**Version**: v2.0 (uvx + Docker)  
**Purpose**: MCP Server for STM32 compile/flash via AI agents

---

## Quick Reference

### Project Structure

```
STM32_Complier_MCP/
├── src/stm32_mcp/          # v2.0 MCP Server (PyPI package)
│   ├── server.py           # Main MCP server (build + flash tools)
│   ├── docker_runner.py    # Docker image management
│   ├── gcc_parse.py        # GCC error parser
│   └── build.sh            # Container build script
├── docker/                 # Docker configurations
├── ESP32_STM32_Bridge/     # Optional: ESP32 remote flashing
├── Test_Data/              # Example STM32 projects
├── pyproject.toml          # Package config
└── README.md
```

### Installation (One Line)

```json
// .opencode/opencode.json
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

Then restart your agent.

---

## Key Components

### 1. PyPI Package (`stm32-mcp`)

Published to: https://pypi.org/project/stm32-mcp/

Install:
```bash
pip install stm32-mcp
# or
uvx stm32-mcp
```

### 2. Docker Image (`legogogoagent/stm32-toolchain`)

Published to: https://hub.docker.com/r/legogogoagent/stm32-toolchain

Pull:
```bash
docker pull legogogoagent/stm32-toolchain:12.3
```

### 3. MCP Tools

Available via `mcp.stm32.*`:

```python
# Build firmware
await mcp.stm32.build_firmware(
    workspace="/path/to/project",
    clean=True,
    jobs=4
)

# Flash to MCU
await mcp.stm32.flash_firmware(
    workspace="/path/to/project",
    programmer="stlink",
    verify=True
)

# Detect MCU
await mcp.stm32.detect_mcu()

# Check environment
await mcp.stm32.check_environment()
```

---

## Development Guidelines

### Adding New Features

1. **Edit src/stm32_mcp/server.py** - Add new `@mcp.tool()` decorated functions
2. **Test locally**:
   ```bash
   pip install -e .
   python -m stm32_mcp
   ```
3. **Update version** in `src/stm32_mcp/__init__.py`
4. **Build and upload**:
   ```bash
   python -m build
   twine upload dist/*
   ```

### Docker Image Updates

1. Edit `docker/Dockerfile`
2. Build and push:
   ```bash
   docker build -f docker/Dockerfile -t legogogoagent/stm32-toolchain:12.3 .
   docker push legogogoagent/stm32-toolchain:12.3
   ```

### Testing

Use Test_Data projects:
```bash
cd Test_Data/Elder_Lifter_STM32_V1.32/Elder_Lifter_STM32
# Then compile via Agent
```

---

## Common Issues

### "MCP server not found"
- Wrong config file: must be `.opencode/opencode.json` (not `mcp.json`)
- Must restart agent after config change
- Format must include `"type": "local"` and `"enabled": true`

### Docker pull fails
- Check internet connection
- Manual pull: `docker pull legogogoagent/stm32-toolchain:12.3`

### OpenOCD not found (for flashing)
- Install OpenOCD: `sudo apt install openocd` (Ubuntu) or `brew install openocd` (macOS)

---

## Git Commit Format

```
<type>(<scope>): <subject>

<body>

<footer>
```

Types: `feat`, `fix`, `docs`, `refactor`, `test`, `chore`

Example:
```
feat(server): add detect_mcu tool

- Auto-detect connected STM32 MCU via OpenOCD
- Returns device ID and family info

Closes #3
```

---

## References

- [MCP Specification](https://modelcontextprotocol.io/)
- [MCP Python SDK](https://github.com/modelcontextprotocol/python-sdk)
- [PyPI Package](https://pypi.org/project/stm32-mcp/)
- [Docker Hub](https://hub.docker.com/r/legogogoagent/stm32-toolchain)
