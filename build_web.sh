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

# ─── Cache-busting: rename outputs with content hashes ───
echo "=== Adding content hashes to filenames ==="

# Remove old hashed files from previous builds
rm -f "$BUILD_DIR"/program.*.js "$BUILD_DIR"/program.*.wasm "$BUILD_DIR"/program.*.data

# Compute short hashes (first 8 hex chars of sha256)
JS_HASH=$(sha256sum "$BUILD_DIR/program.js"   | cut -c1-8)
WASM_HASH=$(sha256sum "$BUILD_DIR/program.wasm" | cut -c1-8)
DATA_HASH=$(sha256sum "$BUILD_DIR/program.data" | cut -c1-8)

# Copy to hashed filenames (keep originals for meson's incremental builds)
cp "$BUILD_DIR/program.js"   "$BUILD_DIR/program.${JS_HASH}.js"
cp "$BUILD_DIR/program.wasm" "$BUILD_DIR/program.${WASM_HASH}.wasm"
cp "$BUILD_DIR/program.data" "$BUILD_DIR/program.${DATA_HASH}.data"

# Patch the hashed JS to reference hashed wasm + data
sed -i \
    -e "s|'program\.data'|'program.${DATA_HASH}.data'|g" \
    -e "s|'program\.wasm'|'program.${WASM_HASH}.wasm'|g" \
    "$BUILD_DIR/program.${JS_HASH}.js"

# Patch the HTML to load the hashed JS
sed -i \
    "s|src=\"program\.js\"|src=\"program.${JS_HASH}.js\"|" \
    "$BUILD_DIR/program.html"

echo ""
echo "Build complete!  Output files:"
echo "  $BUILD_DIR/program.html"
echo "  $BUILD_DIR/program.${JS_HASH}.js"
echo "  $BUILD_DIR/program.${WASM_HASH}.wasm"
echo "  $BUILD_DIR/program.${DATA_HASH}.data"

# Optional: serve
if [ "${1:-}" = "serve" ] || [ "${1:-}" = "--serve" ]; then
    echo ""
    echo "Starting local server at http://localhost:8080/program.html"
    cd "$BUILD_DIR"
    python3 -m http.server 8080
fi
