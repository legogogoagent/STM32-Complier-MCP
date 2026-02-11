#!/usr/bin/env bash
# STM32 MCP Build Server - 容器内编译入口脚本
# 在Docker容器内执行，负责拷贝源码、编译、收集产物

set -euo pipefail

# ============================================
# 配置变量（通过环境变量传入）
# ============================================
SRC_DIR="${SRC_DIR:-/src}"                    # 源码只读挂载点
WORK_DIR="${WORK_DIR:-/work}"                 # 容器内可写工作目录
OUT_DIR="${OUT_DIR:-/out}"                    # 输出目录（日志、产物）
PROJECT_SUBDIR="${PROJECT_SUBDIR:-}"          # Makefile所在子目录
MAKE_TARGET="${MAKE_TARGET:-all}"             # make目标
JOBS="${JOBS:-}"                              # 并行数，空则不指定
CLEAN="${CLEAN:-0}"                           # 是否先make clean（1或0）

# ============================================
# 函数定义
# ============================================

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*"
}

cleanup() {
    local exit_code=$?
    log "编译脚本结束，退出码: $exit_code"
    exit $exit_code
}

trap cleanup EXIT

# ============================================
# 主流程
# ============================================

log "========================================"
log "STM32 MCP Build Server - 编译开始"
log "========================================"
log "SRC_DIR: $SRC_DIR"
log "WORK_DIR: $WORK_DIR"
log "OUT_DIR: $OUT_DIR"
log "PROJECT_SUBDIR: $PROJECT_SUBDIR"
log "MAKE_TARGET: $MAKE_TARGET"
log "JOBS: $JOBS"
log "CLEAN: $CLEAN"
log "========================================"

# 1. 检查源目录
if [[ ! -d "$SRC_DIR" ]]; then
    log "错误: 源目录不存在: $SRC_DIR"
    exit 1
fi

# 2. 创建并清空工作目录和输出目录
log "准备工作环境..."
rm -rf "${WORK_DIR}/project"
mkdir -p "${WORK_DIR}/project"
mkdir -p "${OUT_DIR}/artifacts"

# 3. 拷贝源码到可写目录
log "拷贝源码到工作目录..."
cp -a "${SRC_DIR}/." "${WORK_DIR}/project/"
log "源码拷贝完成"

# 4. 进入工程目录
PROJECT_PATH="${WORK_DIR}/project"
if [[ -n "$PROJECT_SUBDIR" ]]; then
    PROJECT_PATH="${PROJECT_PATH}/${PROJECT_SUBDIR}"
fi

if [[ ! -d "$PROJECT_PATH" ]]; then
    log "错误: 工程目录不存在: $PROJECT_PATH"
    exit 1
fi

cd "$PROJECT_PATH"
log "进入工程目录: $(pwd)"

# 5. 检查Makefile
if [[ ! -f "Makefile" ]]; then
    log "错误: 未找到 Makefile"
    exit 1
fi
log "找到 Makefile"

# 6. 可选: 执行 make clean
if [[ "$CLEAN" == "1" ]]; then
    log "执行 make clean..."
    make clean 2>&1 | tee -a "${OUT_DIR}/build.log" || true
    log "make clean 完成"
fi

# 7. 编译
log "========================================"
log "开始编译..."
log "========================================"

MAKE_CMD="make ${MAKE_TARGET}"
if [[ -n "$JOBS" ]]; then
    MAKE_CMD="make -j${JOBS} ${MAKE_TARGET}"
fi

log "执行: $MAKE_CMD"
BUILD_EXIT=0

# 执行make并同时输出到终端和日志文件
$MAKE_CMD 2>&1 | tee "${OUT_DIR}/build.log" || BUILD_EXIT=${PIPESTATUS[0]}

log "========================================"
if [[ $BUILD_EXIT -eq 0 ]]; then
    log "✓ 编译成功"
else
    log "✗ 编译失败，退出码: $BUILD_EXIT"
fi
log "========================================"

# 8. 收集编译产物
log "收集编译产物..."

# 查找.elf, .hex, .bin文件并复制到artifacts目录
find "${WORK_DIR}/project" -maxdepth 3 -type f \( \
    -name "*.elf" -o \
    -name "*.hex" -o \
    -name "*.bin" -o \
    -name "*.map" -o \
    -name "*.lst" \
    \) -print0 2>/dev/null | while IFS= read -r -d '' file; do
    log "  收集: $(basename "$file")"
    cp "$file" "${OUT_DIR}/artifacts/"
done

# 统计收集到的产物
ARTIFACT_COUNT=$(find "${OUT_DIR}/artifacts" -type f | wc -l)
log "共收集 $ARTIFACT_COUNT 个产物文件"

# 9. 输出摘要
log "========================================"
log "编译摘要:"
log "  退出码: $BUILD_EXIT"
log "  日志文件: ${OUT_DIR}/build.log"
log "  产物目录: ${OUT_DIR}/artifacts/"
log "========================================"

exit $BUILD_EXIT
