#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")"
mkdir -p build

"${CXX:-g++}" -O3 -Werror -std=c++17 correctness.cpp -o build/correctness
STATUS=0
build/correctness > build/correctness.csv || STATUS=$?
MPLCONFIGDIR="$PWD/build/.matplotlib" uv run plot_correctness.py build/correctness.csv
exit "$STATUS"
