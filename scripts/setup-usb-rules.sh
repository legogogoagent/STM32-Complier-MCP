#!/bin/bash
# USB权限自动配置脚本 - 为Docker容器化烧录配置USB设备权限
# Usage: sudo ./setup-usb-rules.sh

set -e

echo "========================================"
echo "STM32 USB权限配置脚本"
echo "========================================"
echo ""

# 检查是否以root权限运行
if [ "$EUID" -ne 0 ]; then 
    echo "❌ 错误: 请使用sudo运行此脚本"
    echo "   sudo $0"
    exit 1
fi

echo "📝 步骤1/4: 创建udev规则文件..."

# 创建udev规则文件
cat > /etc/udev/rules.d/99-stlink.rules << 'EOF'
# ST-Link/V1
SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="3744", MODE="666", GROUP="plugdev"

# ST-Link/V2
SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="3748", MODE="666", GROUP="plugdev"

# ST-Link/V2.1
SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="374b", MODE="666", GROUP="plugdev"

# ST-Link/V3
SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="374d", MODE="666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="374e", MODE="666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="374f", MODE="666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="3752", MODE="666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="3754", MODE="666", GROUP="plugdev"
EOF

echo "📝 步骤2/4: 创建J-Link规则文件..."

cat > /etc/udev/rules.d/99-jlink.rules << 'EOF'
# J-Link USB设备
SUBSYSTEM=="usb", ATTR{idVendor}=="1366", ATTR{idProduct}=="0101", MODE="666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor}=="1366", ATTR{idProduct}=="0102", MODE="666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor}=="1366", ATTR{idProduct}=="0103", MODE="666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor}=="1366", ATTR{idProduct}=="0104", MODE="666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor}=="1366", ATTR{idProduct}=="0105", MODE="666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor}=="1366", ATTR{idProduct}=="0107", MODE="666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor}=="1366", ATTR{idProduct}=="0108", MODE="666", GROUP="plugdev"
EOF

echo "📝 步骤3/4: 创建CMSIS-DAP规则文件..."

cat > /etc/udev/rules.d/99-cmsis-dap.rules << 'EOF'
# CMSIS-DAP/DAP-Link设备
SUBSYSTEM=="usb", ATTR{idVendor}=="c251", ATTR{idProduct}=="f001", MODE="666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor}=="c251", ATTR{idProduct}=="f002", MODE="666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor}=="c251", ATTR{idProduct}=="f003", MODE="666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor}=="0d28", ATTR{idProduct}=="0204", MODE="666", GROUP="plugdev"
EOF

echo "📝 步骤4/4: 重新加载udev规则..."

# 重新加载udev规则
udevadm control --reload-rules
udevadm trigger

# 创建plugdev组（如果不存在）
if ! getent group plugdev > /dev/null 2>&1; then
    groupadd plugdev
    echo "✓ 创建plugdev组"
fi

# 将当前用户添加到plugdev组（如果通过sudo运行）
SUDO_USER=${SUDO_USER:-$USER}
if [ "$SUDO_USER" != "root" ]; then
    usermod -a -G plugdev "$SUDO_USER"
    echo "✓ 将用户 $SUDO_USER 添加到plugdev组"
fi

echo ""
echo "========================================"
echo "✅ USB权限配置完成！"
echo "========================================"
echo ""
echo "📋 配置摘要:"
echo "   • ST-Link规则: /etc/udev/rules.d/99-stlink.rules"
echo "   • J-Link规则: /etc/udev/rules.d/99-jlink.rules"
echo "   • CMSIS-DAP规则: /etc/udev/rules.d/99-cmsis-dap.rules"
echo ""
echo "⚠️  重要提示:"
echo "   1. 请重新插拔调试器使其生效"
echo "   2. 或者运行: sudo udevadm trigger"
echo "   3. 如果使用sudo运行，当前用户已添加到plugdev组"
echo "   4. 注销并重新登录以应用组权限更改"
echo ""
echo "🔍 验证方法:"
echo "   lsusb | grep -i 'st-link\|j-link\|cmsis'"
echo ""
