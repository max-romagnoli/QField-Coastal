# Quickly builds the project for development on Linux using CMake and Ninja.
#!/bin/bash

BUILD_DIR=$(git rev-parse --show-toplevel)/build-x64-linux

if [ ! -d "$BUILD_DIR" ] || [ -z "$(ls -A "$BUILD_DIR")" ]; then
    cmake -S . -B "$BUILD_DIR" -GNinja -DWITH_VCPKG=ON -DWITH_SPIX=ON -DENABLE_TESTS=ON
fi

cmake --build "$BUILD_DIR" --parallel 8