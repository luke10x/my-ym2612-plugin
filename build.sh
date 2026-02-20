#!/usr/bin/env bash
# build.sh – configure and build ARM2612
# Usage:
#   ./build.sh                          # auto-fetch JUCE, Release build
#   ./build.sh --juce /path/to/JUCE     # use local JUCE checkout
#   ./build.sh --debug                  # Debug build
#   ./build.sh --clean                  # wipe build dir first

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
BUILD_TYPE="Release"
JUCE_DIR=""
CLEAN=false
JOBS=$(getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 4)

# ── Argument parsing ──────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        --juce)   JUCE_DIR="$2"; shift 2 ;;
        --debug)  BUILD_TYPE="Debug"; shift ;;
        --clean)  CLEAN=true; shift ;;
        *) echo "Unknown argument: $1"; exit 1 ;;
    esac
done

if $CLEAN && [[ -d "$BUILD_DIR" ]]; then
    echo "→ Cleaning build directory…"
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# ── Configure ─────────────────────────────────────────────────────────────────
CMAKE_ARGS=(
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
)

if [[ -n "$JUCE_DIR" ]]; then
    CMAKE_ARGS+=( -DJUCE_SOURCE_DIR="${JUCE_DIR}" )
fi

echo "→ Configuring (${BUILD_TYPE})…"
cmake "${SCRIPT_DIR}" "${CMAKE_ARGS[@]}"

# ── Build ─────────────────────────────────────────────────────────────────────
echo "→ Building with ${JOBS} jobs…"
cmake --build . --config "${BUILD_TYPE}" -j "${JOBS}"

# ── Summary ───────────────────────────────────────────────────────────────────
echo ""
echo "✓ Build complete!"
echo ""
echo "Artefacts:"
find . -name "*.vst3" -o -name "*.component" 2>/dev/null | while read -r f; do
    echo "  $f"
done
echo ""
echo "To install VST3 (macOS/Linux):"
echo "  cp -r build/ARM2612_artefacts/${BUILD_TYPE}/VST3/*.vst3 ~/Library/Audio/Plug-Ins/VST3/"
echo "To install AU (macOS only):"
echo "  cp -r build/ARM2612_artefacts/${BUILD_TYPE}/AU/*.component ~/Library/Audio/Plug-Ins/Components/"
