#!/bin/bash
# STM32 Dev Workflow Skill - 项目级自动安装脚本
# 此脚本由Agent自动调用，从GitHub安装Skill到当前项目

set -e

REPO_URL="https://github.com/legogogoagent/STM32-Complier-MCP.git"
TEMP_DIR="/tmp/stm32-skill-install-$$"
PROJECT_ROOT="$(pwd)"

echo "=========================================="
echo "STM32 Dev Workflow Skill - 自动安装"
echo "=========================================="
echo ""
echo "项目路径: $PROJECT_ROOT"
echo "仓库: $REPO_URL"
echo ""

# 检查是否在git项目中
if [ ! -d ".git" ] && [ ! -d ".opencode" ]; then
    echo "⚠️  警告: 当前目录可能不是项目根目录"
    read -p "是否继续? (y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# Step 1: 克隆仓库
echo "📦 Step 1: 从GitHub克隆仓库..."
mkdir -p "$TEMP_DIR"
cd "$TEMP_DIR"
git clone --depth 1 "$REPO_URL" stm32-mcp
echo "✓ 克隆完成"

# Step 2: 复制Skill
echo ""
echo "📂 Step 2: 复制Skill到项目..."
mkdir -p "$PROJECT_ROOT/.opencode/skills"

if [ -d "$PROJECT_ROOT/.opencode/skills/stm32-dev-workflow" ]; then
    echo "⚠️  Skill已存在，更新中..."
    rm -rf "$PROJECT_ROOT/.opencode/skills/stm32-dev-workflow"
fi

cp -r "$TEMP_DIR/stm32-mcp/.opencode/skills/stm32-dev-workflow" \
      "$PROJECT_ROOT/.opencode/skills/"
echo "✓ Skill复制完成"

# Step 3: 复制MCP代码
echo ""
echo "💻 Step 3: 复制MCP Server代码..."
if [ -d "$PROJECT_ROOT/mcp_build" ]; then
    echo "⚠️  mcp_build已存在，跳过"
else
    cp -r "$TEMP_DIR/stm32-mcp/mcp_build" "$PROJECT_ROOT/"
    echo "✓ mcp_build复制完成"
fi

if [ -d "$PROJECT_ROOT/mcp_flash" ]; then
    echo "⚠️  mcp_flash已存在，跳过"
else
    cp -r "$TEMP_DIR/stm32-mcp/mcp_flash" "$PROJECT_ROOT/"
    echo "✓ mcp_flash复制完成"
fi

if [ -f "$PROJECT_ROOT/requirements.txt" ]; then
    echo "⚠️  requirements.txt已存在，跳过"
else
    cp "$TEMP_DIR/stm32-mcp/requirements.txt" "$PROJECT_ROOT/"
    echo "✓ requirements.txt复制完成"
fi

# Step 4: 创建MCP配置
echo ""
echo "⚙️  Step 4: 创建MCP配置..."
if [ -f "$PROJECT_ROOT/.opencode/mcp.json" ]; then
    echo "⚠️  mcp.json已存在，备份为mcp.json.bak"
    cp "$PROJECT_ROOT/.opencode/mcp.json" "$PROJECT_ROOT/.opencode/mcp.json.bak"
fi

cat > "$PROJECT_ROOT/.opencode/mcp.json" << EOF
{
  "mcpServers": {
    "stm32-build": {
      "command": "python3",
      "args": ["-m", "mcp_build.stm32_build_server"],
      "cwd": "$PROJECT_ROOT",
      "env": {
        "PYTHONPATH": "$PROJECT_ROOT"
      }
    },
    "stm32-flash": {
      "command": "python3",
      "args": ["-m", "mcp_flash.stm32_flash_server_v2"],
      "cwd": "$PROJECT_ROOT",
      "env": {
        "PYTHONPATH": "$PROJECT_ROOT"
      }
    }
  }
}
EOF
echo "✓ mcp.json创建完成"

# Step 5: 清理临时文件
echo ""
echo "🧹 Step 5: 清理临时文件..."
cd "$PROJECT_ROOT"
rm -rf "$TEMP_DIR"
echo "✓ 清理完成"

# Step 6: 验证安装
echo ""
echo "🔍 Step 6: 验证安装..."

# 检查Skill
if [ -f "$PROJECT_ROOT/.opencode/skills/stm32-dev-workflow/SKILL.md" ]; then
    echo "✓ Skill文件存在"
else
    echo "❌ Skill文件缺失"
    exit 1
fi

# 检查MCP代码
if [ -f "$PROJECT_ROOT/mcp_build/stm32_build_server.py" ]; then
    echo "✓ Build MCP代码存在"
else
    echo "❌ Build MCP代码缺失"
    exit 1
fi

if [ -f "$PROJECT_ROOT/mcp_flash/stm32_flash_server_v2.py" ]; then
    echo "✓ Flash MCP代码存在"
else
    echo "❌ Flash MCP代码缺失"
    exit 1
fi

# 检查配置
if [ -f "$PROJECT_ROOT/.opencode/mcp.json" ]; then
    echo "✓ MCP配置存在"
else
    echo "❌ MCP配置缺失"
    exit 1
fi

# Python导入测试
echo ""
echo "🐍 Python模块测试..."
python3 -c "
import sys
sys.path.insert(0, '$PROJECT_ROOT')
try:
    from mcp_build.stm32_build_server import build_firmware
    print('✓ Build MCP导入成功')
except Exception as e:
    print(f'⚠️  Build MCP导入警告: {e}')

try:
    from mcp_flash.stm32_flash_server_v2 import flash_firmware
    print('✓ Flash MCP导入成功')
except Exception as e:
    print(f'⚠️  Flash MCP导入警告: {e}')
"

echo ""
echo "=========================================="
echo "🎉 安装完成！"
echo "=========================================="
echo ""
echo "已安装到当前项目:"
echo "  📁 .opencode/skills/stm32-dev-workflow/"
echo "  📁 mcp_build/"
echo "  📁 mcp_flash/"
echo "  📄 requirements.txt"
echo "  📄 .opencode/mcp.json"
echo ""
echo "使用方法:"
echo "  1. 安装依赖: pip install -r requirements.txt"
echo "  2. 在Agent中输入: /使用 stm32-dev-workflow"
echo "  3. 或直接说: \"编译STM32项目\""
echo ""
echo "📚 文档:"
echo "  - .opencode/skills/stm32-dev-workflow/SKILL.md"
echo "  - .opencode/skills/stm32-dev-workflow/SKILL_INSTALL.md"
echo "  - .opencode/skills/stm32-dev-workflow/QUICK_REFERENCE.md"
echo ""
