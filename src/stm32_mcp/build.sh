#!/usr/bin/env bash
# STM32 MCP Build Server – container build entry-point (v2.0)
#
# Runs INSIDE the Docker container.  Responsibilities:
#   1. Copy source from read-only /src to writable /work
#   2. Auto-fix Makefile (Windows paths, incompatible GCC flags)
#   3. Compile
#   4. Collect artifacts to /out

set -euo pipefail

# ── Configuration (via environment) ──────────────────────────
SRC_DIR="${SRC_DIR:-/src}"
WORK_DIR="${WORK_DIR:-/work}"
OUT_DIR="${OUT_DIR:-/out}"
PROJECT_SUBDIR="${PROJECT_SUBDIR:-}"
MAKE_TARGET="${MAKE_TARGET:-all}"
JOBS="${JOBS:-}"
CLEAN="${CLEAN:-0}"

# ── Helpers ──────────────────────────────────────────────────
log() { echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*"; }

cleanup() {
    local ec=$?
    log "Build script exiting with code $ec"
    exit $ec
}
trap cleanup EXIT

# ── 1. Sanity checks ────────────────────────────────────────
log "========================================"
log "STM32 MCP Build Server v2.0 – start"
log "========================================"
log "SRC=$SRC_DIR  WORK=$WORK_DIR  OUT=$OUT_DIR"
log "SUBDIR=$PROJECT_SUBDIR  TARGET=$MAKE_TARGET  JOBS=$JOBS  CLEAN=$CLEAN"

if [[ ! -d "$SRC_DIR" ]]; then
    log "ERROR: source directory does not exist: $SRC_DIR"
    exit 1
fi

# ── 2. Prepare writable workspace ───────────────────────────
log "Copying source to writable workspace …"
rm -rf "${WORK_DIR}/project"
mkdir -p "${WORK_DIR}/project"
mkdir -p "${OUT_DIR}/artifacts"
cp -a "${SRC_DIR}/." "${WORK_DIR}/project/"
log "Copy complete"

# ── 3. Resolve project path ─────────────────────────────────
PROJECT_PATH="${WORK_DIR}/project"
if [[ -n "$PROJECT_SUBDIR" ]]; then
    PROJECT_PATH="${PROJECT_PATH}/${PROJECT_SUBDIR}"
fi

if [[ ! -d "$PROJECT_PATH" ]]; then
    log "ERROR: project path does not exist: $PROJECT_PATH"
    exit 1
fi

cd "$PROJECT_PATH"
log "Working in $(pwd)"

# ── 4. Auto-fix Makefile ────────────────────────────────────
MAKEFILE="${PROJECT_PATH}/Makefile"
if [[ -f "$MAKEFILE" ]]; then
    log "Auto-fixing Makefile compatibility issues …"

    # 4a. Replace Windows absolute paths with basenames
    #     C:\Users\foo\bar\file.ld  →  file.ld
    #     C:/Users/foo/bar/file.ld  →  file.ld
    sed -i 's|[A-Z]:\\\\[^ ]*\\\\||g' "$MAKEFILE"
    sed -i 's|[A-Z]:/[^ ]*/||g' "$MAKEFILE"

    # 4b. Remove GCC 13.x incompatible flags
    sed -i 's/-fcyclomatic-complexity//g' "$MAKEFILE"
    sed -i 's/-fstack-usage//g' "$MAKEFILE"

    log "Makefile auto-fix complete"
else
    log "ERROR: Makefile not found"
    exit 1
fi

# ── 5. Optional clean ───────────────────────────────────────
if [[ "$CLEAN" == "1" ]]; then
    log "Running make clean …"
    make clean 2>&1 | tee -a "${OUT_DIR}/build.log" || true
fi

# ── 6. Compile ──────────────────────────────────────────────
log "========================================"
log "Compiling …"
log "========================================"

MAKE_CMD="make ${MAKE_TARGET}"
if [[ -n "$JOBS" ]]; then
    MAKE_CMD="make -j${JOBS} ${MAKE_TARGET}"
fi

log "Executing: $MAKE_CMD"
BUILD_EXIT=0
$MAKE_CMD 2>&1 | tee "${OUT_DIR}/build.log" || BUILD_EXIT=${PIPESTATUS[0]}

if [[ $BUILD_EXIT -eq 0 ]]; then
    log "✓ Build succeeded"
else
    log "✗ Build failed (exit $BUILD_EXIT)"
fi

# ── 7. Collect artifacts ────────────────────────────────────
log "Collecting build artifacts …"
find "${WORK_DIR}/project" -maxdepth 3 -type f \( \
    -name "*.elf" -o \
    -name "*.hex" -o \
    -name "*.bin" -o \
    -name "*.map" \
    \) -print0 2>/dev/null | while IFS= read -r -d '' file; do
    log "  → $(basename "$file")"
    cp "$file" "${OUT_DIR}/artifacts/"
done

ARTIFACT_COUNT=$(find "${OUT_DIR}/artifacts" -type f | wc -l)
log "Collected $ARTIFACT_COUNT artifact(s)"

# ── Done ─────────────────────────────────────────────────────
log "========================================"
log "Build complete.  Exit code: $BUILD_EXIT"
log "========================================"

exit $BUILD_EXIT
