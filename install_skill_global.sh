#!/bin/bash
# STM32 Skill 全局安装脚本
# 使用方法: ./install_skill_global.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_SKILL="$SCRIPT_DIR/.opencode/skills/stm32-dev-workflow"
GLOBAL_SKILL_DIR="$HOME/.config/opencode/skill"

echo "=========================================="
echo "STM32 Dev Workflow Skill - 全局安装"
echo "=========================================="
echo ""

# 检查源skill是否存在
if [ ! -d "$SOURCE_SKILL" ]; then
    echo "❌ 错误: 找不到Skill目录"
    echo "   路径: $SOURCE_SKILL"
    echo ""
    echo "请确保在STM32_Complier_MCP项目根目录运行此脚本"
    exit 1
fi

echo "✓ 找到Skill源目录"
echo "  源: $SOURCE_SKILL"

# 创建全局skill目录
echo ""
echo "📁 创建全局Skill目录..."
mkdir -p "$GLOBAL_SKILL_DIR"
echo "✓ 目录: $GLOBAL_SKILL_DIR"

# 复制skill
echo ""
echo "📦 安装Skill到全局..."
if [ -d "$GLOBAL_SKILL_DIR/stm32-dev-workflow" ]; then
    echo "⚠️  Skill已存在，更新中..."
    rm -rf "$GLOBAL_SKILL_DIR/stm32-dev-workflow"
fi

cp -r "$SOURCE_SKILL" "$GLOBAL_SKILL_DIR/"
echo "✓ 安装完成"

# 验证
echo ""
echo "🔍 验证安装..."
if [ -f "$GLOBAL_SKILL_DIR/stm32-dev-workflow/SKILL.md" ]; then
    echo "✓ SKILL.md 存在"
else
    echo "❌ 安装失败"
    exit 1
fi

if [ -f "$GLOBAL_SKILL_DIR/stm32-dev-workflow/README.md" ]; then
    echo "✓ README.md 存在"
fi

echo ""
echo "=========================================="
echo "🎉 安装成功!"
echo "=========================================="
echo ""
echo "现在你可以在任何OpenCode项目中使用:"
echo "  /使用 stm32-dev-workflow"
echo ""
echo "或者Agent会自动检测并在需要时使用"
echo ""
echo "测试方法:"
echo "  1. 打开任意项目"
echo "  2. 输入: 编译STM32项目"
echo "  3. Agent会自动加载Skill并执行"
echo ""
