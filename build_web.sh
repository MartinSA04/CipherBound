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

DEFAULT_SITE_URL="https://cipherbound.com/"
SITE_URL="${SITE_URL:-$DEFAULT_SITE_URL}"
BUILD_VERSION="${BUILD_VERSION:-$(git rev-parse --short HEAD 2>/dev/null || echo unknown)}"
BUILD_COMMIT_SHA="${BUILD_COMMIT_SHA:-$(git rev-parse HEAD 2>/dev/null || echo unknown)}"
BUILD_COMMIT_SHORT="${BUILD_COMMIT_SHORT:-$(printf '%.7s' "$BUILD_COMMIT_SHA")}"
BUILD_TIMESTAMP="${BUILD_TIMESTAMP:-$(date -u +%Y-%m-%dT%H:%M:%SZ)}"
BUILD_REF_NAME="${BUILD_REF_NAME:-$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo unknown)}"

case "$SITE_URL" in
    http://*|https://*) ;;
    *)
        echo "Error: SITE_URL must start with http:// or https://"
        exit 1
        ;;
esac

case "$SITE_URL" in
    */) SITE_ROOT_URL="$SITE_URL" ;;
    *) SITE_ROOT_URL="$SITE_URL/" ;;
esac

SITE_ROOT_URL_SED="$(printf '%s' "$SITE_ROOT_URL" | sed 's/[&|]/\\&/g')"
BUILD_VERSION_SED="$(printf '%s' "$BUILD_VERSION" | sed 's/[&|]/\\&/g')"
BUILD_COMMIT_SHA_SED="$(printf '%s' "$BUILD_COMMIT_SHA" | sed 's/[&|]/\\&/g')"
BUILD_COMMIT_SHORT_SED="$(printf '%s' "$BUILD_COMMIT_SHORT" | sed 's/[&|]/\\&/g')"
BUILD_TIMESTAMP_SED="$(printf '%s' "$BUILD_TIMESTAMP" | sed 's/[&|]/\\&/g')"
BUILD_REF_NAME_SED="$(printf '%s' "$BUILD_REF_NAME" | sed 's/[&|]/\\&/g')"

if [ "$SITE_ROOT_URL" = "$DEFAULT_SITE_URL" ]; then
    echo "Using default production SITE_URL: $SITE_ROOT_URL"
fi

if [ ! -f "$BUILD_DIR/build.ninja" ]; then
    echo "=== Configuring Emscripten build ==="
    meson setup "$BUILD_DIR" \
        --cross-file "$MESON_CROSS_FILE" \
        --default-library static \
        --buildtype release \
        -Dcpp_std=c++20 \
        -Dc_std=c11 \
        -Dbuild_tests=false
else
    echo "=== Reconfiguring Emscripten build options ==="
    meson setup "$BUILD_DIR" \
        --reconfigure \
        -Dbuild_tests=false
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
    -e "s|__SITE_URL__|${SITE_ROOT_URL_SED}|g" \
    -e "s|__BUILD_VERSION__|${BUILD_VERSION_SED}|g" \
    -e "s|__BUILD_COMMIT_SHA__|${BUILD_COMMIT_SHA_SED}|g" \
    -e "s|__BUILD_COMMIT_SHORT__|${BUILD_COMMIT_SHORT_SED}|g" \
    -e "s|__BUILD_TIMESTAMP__|${BUILD_TIMESTAMP_SED}|g" \
    -e "s|__BUILD_REF_NAME__|${BUILD_REF_NAME_SED}|g" \
    "$DEPLOY_DIR/index.html"

cat > "$DEPLOY_DIR/build_version.json" <<EOF
{
  "version": "${BUILD_VERSION}",
  "commit_sha": "${BUILD_COMMIT_SHA}",
  "commit_short": "${BUILD_COMMIT_SHORT}",
  "build_timestamp": "${BUILD_TIMESTAMP}",
  "ref_name": "${BUILD_REF_NAME}",
  "site_url": "${SITE_ROOT_URL}"
}
EOF

cat > "$DEPLOY_DIR/robots.txt" <<EOF
User-agent: *
Allow: /

Sitemap: ${SITE_ROOT_URL}sitemap.xml
EOF

cat > "$DEPLOY_DIR/sitemap.xml" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">
  <url>
    <loc>${SITE_ROOT_URL}</loc>
    <changefreq>weekly</changefreq>
    <priority>1.0</priority>
  </url>
</urlset>
EOF

echo
echo "Build complete."
echo
echo "Deploy directory contents:"
echo "  $DEPLOY_DIR/index.html"
echo "  $DEPLOY_DIR/build_version.json"
echo "  $DEPLOY_DIR/$HASHED_JS"
echo "  $DEPLOY_DIR/$HASHED_WASM"
echo "  $DEPLOY_DIR/$HASHED_DATA"
echo "  $DEPLOY_DIR/robots.txt"
echo "  $DEPLOY_DIR/sitemap.xml"

if [ "${1:-}" = "serve" ] || [ "${1:-}" = "--serve" ]; then
    echo
    echo "Starting local server at http://localhost:8080/index.html"
    cd "$DEPLOY_DIR"
    python3 -m http.server 8080
fi
