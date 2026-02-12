---
name: stm32-dev-workflow
description: |-
  从GitHub自动安装STM32开发工作流Skill。用于项目级安装，支持一键配置Build MCP和Flash MCP。
  
  Use proactively when:
  - 用户要求"安装STM32 skill"、"配置STM32开发环境"、"从GitHub安装skill"
  - 检测到项目需要STM32开发但没有配置skill
  - 用户提到stm32-dev-workflow或STM32_Complier_MCP仓库
  
  Examples:
  - user: "帮我安装https://github.com/legogogoagent/STM32-Complier-MCP的skill" → clone仓库，复制skill到.opencode/skills/，配置mcp.json
  - user: "配置STM32开发环境" → 检查skill是否存在，不存在则自动安装
  - user: "安装stm32-dev-workflow" → 从GitHub安装到当前项目
  - user: "这个项目需要STM32支持" → 安装skill并配置
---

# STM32开发工作流 - 自动安装指南

## 🎯 快速安装（Agent自动执行）

### 方式1: 直接告诉Agent

```
User: 帮我安装https://github.com/legogogoagent/STM32-Complier-MCP的skill

Agent会自动：
1. 克隆仓库到临时目录
2. 复制skill到项目 .opencode/skills/
3. 创建MCP配置文件
4. 验证安装
5. 返回结果
```

### 方式2: 简洁指令

```
User: 安装STM32开发skill

Agent会自动检测并安装
```

## 📦 Agent安装步骤

Agent执行以下命令自动安装：

```bash
# Step 1: 克隆仓库
mkdir -p /tmp/stm32-mcp-install
cd /tmp/stm32-mcp-install
git clone https://github.com/legogogoagent/STM32-Complier-MCP.git

# Step 2: 复制skill到项目
mkdir -p {PROJECT_ROOT}/.opencode/skills
cp -r STM32-Complier-MCP/.opencode/skills/stm32-dev-workflow \
      {PROJECT_ROOT}/.opencode/skills/

# Step 3: 配置MCP
cat > {PROJECT_ROOT}/.opencode/mcp.json << 'EOF'
{
  "mcpServers": {
    "stm32-build": {
      "command": "python3",
      "args": ["-m", "mcp_build.stm32_build_server"],
      "cwd": "{PROJECT_ROOT}",
      "env": {
        "PYTHONPATH": "{PROJECT_ROOT}"
      }
    },
    "stm32-flash": {
      "command": "python3",
      "args": ["-m", "mcp_flash.stm32_flash_server_v2"],
      "cwd": "{PROJECT_ROOT}",
      "env": {
        "PYTHONPATH": "{PROJECT_ROOT}"
      }
    }
  }
}
EOF

# Step 4: 复制MCP代码到项目（可选）
cp -r STM32-Complier-MCP/mcp_build {PROJECT_ROOT}/
cp -r STM32-Complier-MCP/mcp_flash {PROJECT_ROOT}/
cp STM32-Complier-MCP/requirements.txt {PROJECT_ROOT}/

# Step 5: 清理
cd {PROJECT_ROOT}
rm -rf /tmp/stm32-mcp-install
```

## 🔧 手动安装步骤

如果Agent自动安装失败，可以手动执行：

### 1. 克隆仓库

```bash
cd /tmp
git clone https://github.com/legogogoagent/STM32-Complier-MCP.git
cd STM32-Complier-MCP
```

### 2. 复制Skill

```bash
# 在你的项目根目录
mkdir -p .opencode/skills
cp -r /tmp/STM32-Complier-MCP/.opencode/skills/stm32-dev-workflow \
      .opencode/skills/
```

### 3. 复制MCP代码

```bash
# 复制MCP Server代码到项目
cp -r /tmp/STM32-Complier-MCP/mcp_build ./
cp -r /tmp/STM32-Complier-MCP/mcp_flash ./
cp /tmp/STM32-Complier-MCP/requirements.txt ./
```

### 4. 配置MCP

创建 `.opencode/mcp.json`：

```json
{
  "mcpServers": {
    "stm32-build": {
      "command": "python3",
      "args": ["-m", "mcp_build.stm32_build_server"],
      "cwd": "{你的项目绝对路径}",
      "env": {
        "PYTHONPATH": "{你的项目绝对路径}"
      }
    },
    "stm32-flash": {
      "command": "python3",
      "args": ["-m", "mcp_flash.stm32_flash_server_v2"],
      "cwd": "{你的项目绝对路径}",
      "env": {
        "PYTHONPATH": "{你的项目绝对路径}"
      }
    }
  }
}
```

### 5. 安装依赖

```bash
pip install -r requirements.txt
```

### 6. 验证安装

```bash
python3 -c "import sys; sys.path.insert(0, '.'); from mcp_build.stm32_build_server import build_firmware; print('✓ Build MCP OK')"
python3 -c "import sys; sys.path.insert(0, '.'); from mcp_flash.stm32_flash_server_v2 import flash_firmware; print('✓ Flash MCP OK')"
```

## 📝 使用Prompt示例

### 示例1: 完整安装指令

```
User: 帮我安装https://github.com/legogogoagent/STM32-Complier-MCP的skill，配置好STM32开发环境

Agent执行:
  1. 克隆GitHub仓库
  2. 复制skill到 .opencode/skills/
  3. 复制MCP代码到项目
  4. 创建mcp.json配置
  5. 验证安装
  
Agent回复:
  ✅ Skill安装完成！
  
  已安装:
  - Skill: stm32-dev-workflow → .opencode/skills/
  - Build MCP: mcp_build/
  - Flash MCP: mcp_flash/
  - 配置: .opencode/mcp.json
  
  现在你可以使用:
  - "编译STM32项目"
  - "烧录固件"
  - "/使用 stm32-dev-workflow"
```

### 示例2: 简洁指令

```
User: 配置STM32开发环境

Agent:
  检测到项目需要STM32支持，正在安装stm32-dev-workflow skill...
  
  [自动执行安装步骤]
  
  ✅ 安装完成！现在你可以编译和烧录STM32项目了。
```

### 示例3: 检查并安装

```
User: 这个项目能编译STM32代码吗？

Agent:
  检查中...
  
  未找到STM32开发配置。是否安装stm32-dev-workflow skill？
  
User: 是的，安装

Agent:
  正在从GitHub安装...
  ✅ 安装完成！
```

### 示例4: 指定仓库

```
User: 从legogogoagent/STM32-Complier-MCP安装skill

Agent:
  克隆 https://github.com/legogogoagent/STM32-Complier-MCP.git
  复制skill文件...
  配置MCP...
  ✅ 完成！
```

## 🗂️ 安装后的项目结构

```
your-project/
├── .opencode/
│   ├── mcp.json                    # MCP配置
│   └── skills/
│       └── stm32-dev-workflow/     # Skill目录
│           ├── SKILL.md
│           ├── README.md
│           ├── QUICK_REFERENCE.md
│           └── ...
│
├── mcp_build/                      # Build MCP代码
│   ├── stm32_build_server.py
│   └── ...
│
├── mcp_flash/                      # Flash MCP代码
│   ├── stm32_flash_server_v2.py
│   ├── local_flasher.py
│   └── ...
│
├── requirements.txt                # Python依赖
│
└── your-stm32-project/            # 你的STM32代码
    ├── Core/
    ├── Makefile
    └── ...
```

## 🔍 故障排查

### 问题1: git clone失败

```bash
# 检查网络连接
ping github.com

# 或者使用ssh
# 将 https://github.com/legogogoagent/STM32-Complier-MCP.git
# 改为 git@github.com:legogogoagent/STM32-Complier-MCP.git
```

### 问题2: MCP导入失败

```bash
# 检查PYTHONPATH设置
# 确保mcp.json中的cwd和PYTHONPATH指向项目根目录

# 手动测试导入
python3 -c "
import sys
sys.path.insert(0, '.')
from mcp_build.stm32_build_server import build_firmware
print('OK')
"
```

### 问题3: Skill未加载

```bash
# 检查skill位置
ls -la .opencode/skills/stm32-dev-workflow/SKILL.md

# 检查文件权限
chmod -R 755 .opencode/skills/
```

## 🎓 进阶配置

### 自定义Docker镜像路径

如果你的Docker镜像名称不同，修改 `.opencode/mcp.json`：

```json
{
  "mcpServers": {
    "stm32-build": {
      "command": "python3",
      "args": ["-m", "mcp_build.stm32_build_server"],
      "cwd": "{PROJECT_ROOT}",
      "env": {
        "PYTHONPATH": "{PROJECT_ROOT}",
        "DOCKER_IMAGE": "your-custom-image:latest"
      }
    }
  }
}
```

### 使用远程MCP Server

如果MCP Server部署在远程：

```json
{
  "mcpServers": {
    "stm32-build": {
      "url": "http://your-server:50051",
      "token": "your-auth-token"
    }
  }
}
```

## 📚 相关文档

- [SKILL.md](SKILL.md) - Skill完整文档
- [QUICK_REFERENCE.md](QUICK_REFERENCE.md) - API速查
- [GitHub仓库](https://github.com/legogogoagent/STM32-Complier-MCP)

---

**版本**: v1.0.0  
**更新日期**: 2026-02-12  
**作者**: STM32 MCP Team
