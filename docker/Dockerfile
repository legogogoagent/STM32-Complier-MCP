# STM32 MCP Build Server - Docker编译环境
# 基于Ubuntu 24.04 + arm-none-eabi-gcc工具链

FROM ubuntu:24.04

# 避免交互式配置提示
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

# 安装arm-none-eabi-gcc工具链和基础工具
RUN apt-get update && apt-get install -y --no-install-recommends \
    gcc-arm-none-eabi \
    binutils-arm-none-eabi \
    make \
    python3 \
    bash \
    coreutils \
    findutils \
    sed \
    grep \
    gawk \
    file \
    && rm -rf /var/lib/apt/lists/*

# 创建工作目录
# /src: 只读挂载源码目录 (从宿主机workspace挂载)
# /work: 容器内可写工作目录 (编译在此执行)
# /out: 输出目录 (编译日志和产物)
RUN mkdir -p /src /work /out

# 设置工作目录
WORKDIR /work

# 验证工具链安装
RUN arm-none-eabi-gcc --version && \
    arm-none-eabi-ld --version && \
    make --version

# 容器启动时默认保持运行(等待命令)
CMD ["bash"]
