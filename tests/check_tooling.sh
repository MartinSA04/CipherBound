#!/usr/bin/env bash
set -euo pipefail

source_root="$1"
build_root="$2"
compile_db="$build_root/compile_commands.json"

if [ ! -f "$source_root/.clang-format" ]; then
    echo "Missing .clang-format in source root"
    exit 1
fi

if [ ! -f "$source_root/clang_format.sh" ]; then
    echo "Missing clang_format.sh helper"
    exit 1
fi

if [ ! -f "$compile_db" ]; then
    echo "Missing compile_commands.json in build root"
    exit 1
fi

grep -q -- '"file": "../src/main.cpp"' "$compile_db"
grep -q -- '"file": "../src/core/Session.cpp"' "$compile_db"
grep -q -- '"file": "../src/ui/gameui/Battle.cpp"' "$compile_db"
grep -q -- '-std=c++20' "$compile_db"

echo "Tooling smoke checks passed."
