#!/bin/bash

set -euo pipefail

TARGET="raylib_boilerplate"

if [ ! -d "build" ]; then
    echo "Configuring CMake..."
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
fi

echo "Building ${TARGET}..."
cmake --build build -j --target "${TARGET}"

echo "Launching ${TARGET} (close the window to exit)..."
"./build/${TARGET}"
