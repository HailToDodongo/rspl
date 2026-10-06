#!/usr/bin/env bash
set -e
cd "$(dirname "$0")"
shopt -s globstar nullglob

FILES=(src/**/*.cpp src/**/*.h tests/**/*.cpp tests/**/*.h)

[ -f build/compile_commands.json ] || cmake -S . -B build > /dev/null

# clang-tidy -p build --fix --quiet "${FILES[@]}"
clang-format -i "${FILES[@]}"
