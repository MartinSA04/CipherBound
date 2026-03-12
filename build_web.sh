#!/usr/bin/env bash
# Build the project for WebAssembly using Emscripten.
#
# Usage:
#   ./build_web.sh
#   ./build_web.sh serve
#   ./build_web.sh --serve
#
# Output:
#   buildw/deploy/
#     index.html
#     program.<hash>.js
#     program.<hash>.wasm
#     program.<hash>.data
#
# Local:
#   serves from same folder, so .data loads locally
#
# CI:
#   rewrite ASSET_BASE in index.html to https://assets.cipherbound.com
#   upload .data to R2
#   delete .data from deploy/
#   deploy deploy/ to Cloudflare Pages

set -euo pipefail

BUILD_DIR="buildw"
DEPLOY_DIR="$BUILD_DIR/deploy"
PROGRAM_NAME="program"
HTML_NAME="index.html"
MESON_CROSS_FILE="emscripten-cross.ini"

if ! command -v emcc >/dev/null 2>&1; then
    echo "Error: emcc not found. Activate Emscripten first:"
    echo "  source /path/to/emsdk/emsdk_env.sh"
    exit 1
fi

for tool in meson sha256sum sed cp rm mkdir python3; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "Error: required tool '$tool' not found."
        exit 1
    fi
done

if [ ! -f "$BUILD_DIR/build.ninja" ]; then
    echo "=== Configuring Emscripten build ==="
    meson setup "$BUILD_DIR" \
        --cross-file "$MESON_CROSS_FILE" \
        --default-library static \
        --buildtype release \
        -Dcpp_std=c++20 \
        -Dc_std=c11
fi

echo "=== Building for WebAssembly ==="
meson compile -C "$BUILD_DIR"

SRC_HTML="$BUILD_DIR/index.html"
SRC_JS="$BUILD_DIR/${PROGRAM_NAME}.js"
SRC_WASM="$BUILD_DIR/${PROGRAM_NAME}.wasm"
SRC_DATA="$BUILD_DIR/${PROGRAM_NAME}.data"

for f in "$SRC_HTML" "$SRC_JS" "$SRC_WASM" "$SRC_DATA"; do
    if [ ! -f "$f" ]; then
        echo "Error: expected build output not found: $f"
        exit 1
    fi
done

echo "=== Creating hashed asset filenames ==="

rm -f "$BUILD_DIR"/"${PROGRAM_NAME}".*.js
rm -f "$BUILD_DIR"/"${PROGRAM_NAME}".*.wasm
rm -f "$BUILD_DIR"/"${PROGRAM_NAME}".*.data

JS_HASH="$(sha256sum "$SRC_JS" | cut -c1-8)"
WASM_HASH="$(sha256sum "$SRC_WASM" | cut -c1-8)"
DATA_HASH="$(sha256sum "$SRC_DATA" | cut -c1-8)"

HASHED_JS="${PROGRAM_NAME}.${JS_HASH}.js"
HASHED_WASM="${PROGRAM_NAME}.${WASM_HASH}.wasm"
HASHED_DATA="${PROGRAM_NAME}.${DATA_HASH}.data"

cp "$SRC_JS"   "$BUILD_DIR/$HASHED_JS"
cp "$SRC_WASM" "$BUILD_DIR/$HASHED_WASM"
cp "$SRC_DATA" "$BUILD_DIR/$HASHED_DATA"

sed -i \
    -e "s|'${PROGRAM_NAME}\.wasm'|'${HASHED_WASM}'|g" \
    -e "s|\"${PROGRAM_NAME}\.wasm\"|\"${HASHED_WASM}\"|g" \
    -e "s|'${PROGRAM_NAME}\.data'|'${HASHED_DATA}'|g" \
    -e "s|\"${PROGRAM_NAME}\.data\"|\"${HASHED_DATA}\"|g" \
    "$BUILD_DIR/$HASHED_JS"

echo "=== Creating clean deploy directory ==="
rm -rf "$DEPLOY_DIR"
mkdir -p "$DEPLOY_DIR"

cp "$BUILD_DIR/$HASHED_JS"   "$DEPLOY_DIR/$HASHED_JS"
cp "$BUILD_DIR/$HASHED_WASM" "$DEPLOY_DIR/$HASHED_WASM"
cp "$BUILD_DIR/$HASHED_DATA" "$DEPLOY_DIR/$HASHED_DATA"
cp "$SRC_HTML"               "$DEPLOY_DIR/$HTML_NAME"

echo
echo "Build complete."
echo
echo "Deploy directory contents:"
echo "  $DEPLOY_DIR/$HTML_NAME"
echo "  $DEPLOY_DIR/$HASHED_JS"
echo "  $DEPLOY_DIR/$HASHED_WASM"
echo "  $DEPLOY_DIR/$HASHED_DATA"
echo
echo "Local test:"
echo "  cd $DEPLOY_DIR && python3 -m http.server 8080"
echo "  open http://localhost:8080/$HTML_NAME"
echo
echo "CI steps:"
echo "  1. rewrite ASSET_BASE to https://assets.cipherbound.com"
echo "  2. upload $HASHED_DATA to R2"
echo "  3. remove .data from $DEPLOY_DIR"
echo "  4. deploy $DEPLOY_DIR to Cloudflare Pages"

if [ "${1:-}" = "serve" ] || [ "${1:-}" = "--serve" ]; then
    echo
    echo "Starting local server at http://localhost:8080/$HTML_NAME"
    cd "$DEPLOY_DIR"
    python3 -m http.server 8080
fi