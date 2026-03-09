#!/usr/bin/env bash
# Build the project for WebAssembly using Emscripten.
#
# Prerequisites:
#   - Install Emscripten SDK: https://emscripten.org/docs/getting_started/downloads.html
#   - Activate it:  source /path/to/emsdk/emsdk_env.sh
#
# Usage:
#   ./build_web.sh          # configure + build
#   ./build_web.sh serve    # build then start a local HTTP server on port 8080
#   ./build_web.sh --serve  # same as above

set -euo pipefail

BUILD_DIR="buildw"

# Verify emcc is available
if ! command -v emcc &>/dev/null; then
    echo "Error: emcc not found.  Activate the Emscripten SDK first:"
    echo "  source /path/to/emsdk/emsdk_env.sh"
    exit 1
fi

# Configure (only if not already configured)
if [ ! -f "$BUILD_DIR/build.ninja" ]; then
    echo "=== Configuring Emscripten build ==="
    meson setup "$BUILD_DIR" \
        --cross-file emscripten-cross.ini \
        --default-library static \
        -Dcpp_std=c++20 \
        -Dc_std=c11
fi

# Build
echo "=== Building for WebAssembly ==="
meson compile -C "$BUILD_DIR"

echo ""
echo "Build complete!  Output files:"
echo "  $BUILD_DIR/program.html"
echo "  $BUILD_DIR/program.js"
echo "  $BUILD_DIR/program.wasm"
echo "  $BUILD_DIR/program.data"

# Optional: serve
if [ "${1:-}" = "serve" ] || [ "${1:-}" = "--serve" ]; then
    echo ""
    echo "Starting local server at http://localhost:8080/program.html"
    cd "$BUILD_DIR"
    python3 -m http.server 8080
fi
