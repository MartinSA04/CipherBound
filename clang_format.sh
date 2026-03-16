#!/usr/bin/env bash
set -euo pipefail

# Check if clang-format is installed
if ! command -v clang-format &> /dev/null; then
    echo "Error: clang-format is not installed or not in PATH"
    exit 1
fi

# Check if src directory exists
if [ ! -d "./src" ]; then
    echo "Error: ./src directory not found"
    exit 1
fi

find ./src \( -name '*.h' -o -name '*.cpp' \) -print0 | xargs -0 clang-format -i

echo "Formatting C++ files in ./src directory..."
echo "Done formatting files."
