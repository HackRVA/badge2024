#!/bin/sh
set -e

EMSDK_DIR="./build_wasm/_deps/emsdk-src"

if [ ! -d "$EMSDK_DIR" ]; then
    echo "Emscripten SDK not found. Cloning..."
    mkdir -p build_wasm/_deps
    git clone --depth 1 https://github.com/emscripten-core/emsdk.git "$EMSDK_DIR"
    cd "$EMSDK_DIR"
    ./emsdk install latest
    ./emsdk activate latest
    cd ../../..
fi

if [ -f "${EMSDK_DIR}/emsdk_env.sh" ]; then
    echo "Sourcing emsdk_env.sh..."
    source "${EMSDK_DIR}/emsdk_env.sh"
    export PATH="$EMSDK_DIR/upstream/emscripten:$EMSDK_DIR/upstream/bin:$PATH"
else
    echo "Error: emsdk_env.sh not found after installation."
    exit 1
fi

if ! command -v emcmake &> /dev/null; then
    echo "Error: emcmake command not found. Manually adding to PATH..."
    export PATH="$EMSDK_DIR/upstream/emscripten:$EMSDK_DIR/upstream/bin:$PATH"
fi

cd ./build_wasm/

which emcmake || { echo "Error: emcmake still not found."; exit 1; }

echo "Running emcmake cmake ..."
emcmake cmake .. -DTARGET=WASM -DCMAKE_EXPORT_COMPILE_COMMANDS=1 || exit 1

echo "Building with emmake ..."
emmake make

echo "Starting local HTTP server ..."
cmake --build . --target run_server
