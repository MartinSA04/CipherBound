#!/usr/bin/env bash
set -euo pipefail

source_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$source_root"

if ! command -v doxygen >/dev/null 2>&1; then
    echo "doxygen is required to generate the documentation."
    exit 1
fi

if ! command -v dot >/dev/null 2>&1; then
    echo "graphviz 'dot' is required because HAVE_DOT=YES in Doxyfile."
    exit 1
fi

rm -rf docs/html docs/latex

log_file="$(mktemp)"
filtered_log_file="$(mktemp)"
trap 'rm -f "$log_file" "$filtered_log_file"' EXIT

if ! doxygen Doxyfile >"$log_file" 2>&1; then
    cat "$log_file"
    exit 1
fi

# Filter out known layout-schema warnings caused by Doxygen version differences
# between local development and GitHub Actions runners.
grep -Ev \
    "warning: ignoring unsupported tag 'PAGE_OUTLINE_PANEL'|warning: Unexpected start tag 'properties' found in scope='(namespace|file)/member(decl|def)/'!|warning: User defined layout misses entry '(namespace|file)/member(def|decl)/properties'" \
    "$log_file" >"$filtered_log_file" || true

cat "$filtered_log_file"

if grep -Eq 'warning:|error:' "$filtered_log_file"; then
    echo "Doxygen emitted warnings or errors."
    exit 1
fi

if [ ! -f docs/html/index.html ]; then
    echo "Documentation generation did not produce docs/html/index.html."
    exit 1
fi

echo "Documentation generated successfully."
