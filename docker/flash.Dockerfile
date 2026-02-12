# STM32 MCP Flash Server - Docker烧录环境
# 基于Ubuntu 24.04 + OpenOCD + ST-Link驱动

FROM ubuntu:24.04

# 避免交互式配置提示
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC
ENV OPENOCD_VERSION=0.11.0

# 安装OpenOCD、ST-Link驱动和USB工具
RUN apt-get update && apt-get install -y --no-install-recommends \
    openocd \
    libusb-1.0-0 \
    libusb-1.0-0-dev \
    udev \
    usbutils \
    libtool \
    make \
    pkg-config \
    autoconf \
    automake \
    texinfo \
    libftdi1-2 \
    libftdi1-dev \
    libhidapi-hidraw0 \
    libhidapi-dev \
    python3 \
    python3-pip \
    bash \
    coreutils \
    file \
    && rm -rf /var/lib/apt/lists/*

# 创建udev规则目录（容器内使用）
RUN mkdir -p /etc/udev/rules.d

# 添加ST-Link USB权限规则
RUN echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="3744", MODE="666", GROUP="plugdev"' > /etc/udev/rules.d/99-stlink.rules && \
    echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="3748", MODE="666", GROUP="plugdev"' >> /etc/udev/rules.d/99-stlink.rules && \
    echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="374b", MODE="666", GROUP="plugdev"' >> /etc/udev/rules.d/99-stlink.rules && \
    echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="374d", MODE="666", GROUP="plugdev"' >> /etc/udev/rules.d/99-stlink.rules && \
    echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="374e", MODE="666", GROUP="plugdev"' >> /etc/udev/rules.d/99-stlink.rules && \
    echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="374f", MODE="666", GROUP="plugdev"' >> /etc/udev/rules.d/99-stlink.rules && \
    echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="3752", MODE="666", GROUP="plugdev"' >> /etc/udev/rules.d/99-stlink.rules && \
    echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="3754", MODE="666", GROUP="plugdev"' >> /etc/udev/rules.d/99-stlink.rules && \
    echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="1366", ATTR{idProduct}=="0101", MODE="666", GROUP="plugdev"' >> /etc/udev/rules.d/99-stlink.rules && \
    echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="1366", ATTR{idProduct}=="0102", MODE="666", GROUP="plugdev"' >> /etc/udev/rules.d/99-stlink.rules && \
    echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="1366", ATTR{idProduct}=="0103", MODE="666", GROUP="plugdev"' >> /etc/udev/rules.d/99-stlink.rules && \
    echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="1366", ATTR{idProduct}=="0104", MODE="666", GROUP="plugdev"' >> /etc/udev/rules.d/99-stlink.rules && \
    echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="1366", ATTR{idProduct}=="0105", MODE="666", GROUP="plugdev"' >> /etc/udev/rules.d/99-stlink.rules && \
    echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="1366", ATTR{idProduct}=="0107", MODE="666", GROUP="plugdev"' >> /etc/udev/rules.d/99-stlink.rules && \
    echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="1366", ATTR{idProduct}=="0108", MODE="666", GROUP="plugdev"' >> /etc/udev/rules.d/99-stlink.rules && \
    echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="c251", ATTR{idProduct}=="f001", MODE="666", GROUP="plugdev"' >> /etc/udev/rules.d/99-stlink.rules && \
    echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="c251", ATTR{idProduct}=="f002", MODE="666", GROUP="plugdev"' >> /etc/udev/rules.d/99-stlink.rules && \
    echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="c251", ATTR{idProduct}=="f003", MODE="666", GROUP="plugdev"' >> /etc/udev/rules.d/99-stlink.rules

# 创建工作目录
# /src: 只读挂载源码目录
# /out: 输出目录（编译产物和日志）
# /dev/bus/usb: USB设备透传
RUN mkdir -p /src /out /work

# 设置工作目录
WORKDIR /work

# 验证OpenOCD安装
RUN openocd --version && \
    ls -la /usr/share/openocd/scripts/interface/ && \
    ls -la /usr/share/openocd/scripts/target/

# 复制烧录脚本
COPY tools/flash.sh /usr/local/bin/flash.sh
RUN chmod +x /usr/local/bin/flash.sh

# 容器启动时默认保持运行(等待命令)
CMD ["bash"]
