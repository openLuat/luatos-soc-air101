#!/bin/bash
# build.sh - One-click build and comparison script for secboot.bin decompilation
#
# Usage:
#   ./build.sh              - Build and compare
#   ./build.sh --report     - Build, compare, and generate full report
#   ./build.sh --install    - Install toolchain first, then build
#   ./build.sh --clean      - Clean and rebuild
#
# Requirements:
#   - csky-elfabiv2-gcc toolchain (auto-detected from /tmp/csky-tools/gcc/bin/)
#   - python3 (for comparison report)

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
TOOLCHAIN_URL="https://github.com/openLuat/luatos-soc-air101/releases/download/v2001.gcc/csky-elfabiv2-tools-x86_64-minilibc-20230301.zip"
TOOLCHAIN_DIR="/tmp/csky-tools"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# ============================================================
# Helper functions
# ============================================================

info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# ============================================================
# Install toolchain if needed
# ============================================================
install_toolchain() {
    if [ -f "$TOOLCHAIN_DIR/gcc/bin/csky-elfabiv2-gcc" ]; then
        info "C-SKY toolchain already installed"
        return 0
    fi

    info "Downloading C-SKY toolchain..."
    cd /tmp
    curl -sL -o csky-tools.zip "$TOOLCHAIN_URL"
    unzip -q -o csky-tools.zip -d "$TOOLCHAIN_DIR"
    rm -f csky-tools.zip

    if [ -f "$TOOLCHAIN_DIR/gcc/bin/csky-elfabiv2-gcc" ]; then
        info "Toolchain installed to $TOOLCHAIN_DIR/gcc/bin/"
    else
        error "Toolchain installation failed!"
        exit 1
    fi
}

# ============================================================
# Check toolchain
# ============================================================
check_toolchain() {
    if command -v csky-elfabiv2-gcc &>/dev/null; then
        return 0
    fi

    if [ -f "$TOOLCHAIN_DIR/gcc/bin/csky-elfabiv2-gcc" ]; then
        export PATH="$TOOLCHAIN_DIR/gcc/bin:$PATH"
        return 0
    fi

    warn "C-SKY toolchain not found. Installing..."
    install_toolchain
    export PATH="$TOOLCHAIN_DIR/gcc/bin:$PATH"
}

# ============================================================
# Main
# ============================================================

cd "$SCRIPT_DIR"

# Parse arguments
DO_REPORT=0
DO_CLEAN=0
DO_INSTALL=0

for arg in "$@"; do
    case "$arg" in
        --report)  DO_REPORT=1 ;;
        --clean)   DO_CLEAN=1 ;;
        --install) DO_INSTALL=1 ;;
        --help|-h)
            echo "Usage: $0 [--report] [--clean] [--install]"
            echo ""
            echo "  --report   Generate full comparison report"
            echo "  --clean    Clean build directory first"
            echo "  --install  Install toolchain before building"
            exit 0
            ;;
    esac
done

# Install toolchain if requested
if [ $DO_INSTALL -eq 1 ]; then
    install_toolchain
fi

# Check toolchain availability
check_toolchain

GCC_VERSION=$(csky-elfabiv2-gcc --version | head -1)
info "Using toolchain: $GCC_VERSION"

# Clean if requested
if [ $DO_CLEAN -eq 1 ]; then
    info "Cleaning build directory..."
    make clean
fi

# Build
info "Building secboot.bin..."
echo ""
make all 2>&1
echo ""

# Compare
info "Comparing with original binary..."
echo ""
make compare
echo ""

# Run full report if requested
if [ $DO_REPORT -eq 1 ]; then
    info "Generating full comparison report..."
    echo ""
    make report
    echo ""
fi

# Summary
ORIG_SIZE=$(wc -c < "$REPO_ROOT/tools/xt804/xt804_secboot.bin")
BUILD_BIN="build/secboot.bin"
if [ -f "$BUILD_BIN" ]; then
    NEW_SIZE=$(wc -c < "$BUILD_BIN")
    info "Build complete!"
    echo "  Original:    $ORIG_SIZE bytes"
    echo "  Recompiled:  $NEW_SIZE bytes"
    echo "  Build output: $SCRIPT_DIR/build/"
    echo ""
    echo "  Files:"
    echo "    build/secboot.elf  - ELF executable"
    echo "    build/secboot.bin  - Raw binary (for flashing)"
    echo "    build/secboot.dis  - Disassembly listing"
    echo "    build/secboot.map  - Linker map file"
else
    error "Build failed - no output binary"
    exit 1
fi
