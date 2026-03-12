#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="buildw"
DEPLOY_DIR="$BUILD_DIR/deploy"
BASENAME="index"
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

SRC_HTML="$BUILD_DIR/${BASENAME}.html"
SRC_JS="$BUILD_DIR/${BASENAME}.js"
SRC_WASM="$BUILD_DIR/${BASENAME}.wasm"
SRC_DATA="$BUILD_DIR/${BASENAME}.data"

for f in "$SRC_HTML" "$SRC_JS" "$SRC_WASM" "$SRC_DATA"; do
    if [ ! -f "$f" ]; then
        echo "Error: expected build output not found: $f"
        exit 1
    fi
done

echo "=== Creating hashed asset filenames ==="

rm -f "$BUILD_DIR"/"${BASENAME}".*.js
rm -f "$BUILD_DIR"/"${BASENAME}".*.wasm
rm -f "$BUILD_DIR"/"${BASENAME}".*.data

JS_HASH="$(sha256sum "$SRC_JS" | cut -c1-8)"
WASM_HASH="$(sha256sum "$SRC_WASM" | cut -c1-8)"
DATA_HASH="$(sha256sum "$SRC_DATA" | cut -c1-8)"

HASHED_JS="${BASENAME}.${JS_HASH}.js"
HASHED_WASM="${BASENAME}.${WASM_HASH}.wasm"
HASHED_DATA="${BASENAME}.${DATA_HASH}.data"

cp "$SRC_JS"   "$BUILD_DIR/$HASHED_JS"
cp "$SRC_WASM" "$BUILD_DIR/$HASHED_WASM"
cp "$SRC_DATA" "$BUILD_DIR/$HASHED_DATA"

sed -i \
    -e "s|'${BASENAME}\.wasm'|'${HASHED_WASM}'|g" \
    -e "s|\"${BASENAME}\.wasm\"|\"${HASHED_WASM}\"|g" \
    -e "s|'${BASENAME}\.data'|'${HASHED_DATA}'|g" \
    -e "s|\"${BASENAME}\.data\"|\"${HASHED_DATA}\"|g" \
    "$BUILD_DIR/$HASHED_JS"

echo "=== Creating clean deploy directory ==="
rm -rf "$DEPLOY_DIR"
mkdir -p "$DEPLOY_DIR"

cp "$BUILD_DIR/$HASHED_JS"   "$DEPLOY_DIR/$HASHED_JS"
cp "$BUILD_DIR/$HASHED_WASM" "$DEPLOY_DIR/$HASHED_WASM"
cp "$BUILD_DIR/$HASHED_DATA" "$DEPLOY_DIR/$HASHED_DATA"
cp "$SRC_HTML"               "$DEPLOY_DIR/index.html"

sed -i \
    -e "s|src=\"${BASENAME}\.js\"|src=\"${HASHED_JS}\"|g" \
    -e "s|src='${BASENAME}\.js'|src='${HASHED_JS}'|g" \
    "$DEPLOY_DIR/index.html"

echo
echo "Build complete."
echo
echo "Deploy directory contents:"
echo "  $DEPLOY_DIR/index.html"
echo "  $DEPLOY_DIR/$HASHED_JS"
echo "  $DEPLOY_DIR/$HASHED_WASM"
echo "  $DEPLOY_DIR/$HASHED_DATA"

if [ "${1:-}" = "serve" ] || [ "${1:-}" = "--serve" ]; then
    echo
    echo "Starting local server at http://localhost:8080/index.html"
    cd "$DEPLOY_DIR"
    python3 -m http.server 8080
fi