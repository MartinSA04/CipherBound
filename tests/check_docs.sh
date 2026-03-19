#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source_root="${1:-$(cd "$script_dir/.." && pwd)}"

"$source_root/generate_docs.sh"

index_html="$source_root/docs/html/index.html"
pages_html="$source_root/docs/html/pages.html"
topics_html="$source_root/docs/html/topics.html"

test -f "$index_html"
test -f "$pages_html"
test -f "$topics_html"

grep -q 'CipherBound Documentation' "$index_html"
grep -q 'How To Add A Map' "$index_html"
grep -q 'How To Script A Cutscene' "$pages_html"
grep -q 'How Save Compatibility Works' "$pages_html"
grep -q 'How Battle Turn Resolution Works' "$pages_html"

echo "Documentation smoke checks passed."
