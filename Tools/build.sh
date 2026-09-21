#!/bin/bash
cd "$(dirname "$0")/.."
echo "Aether 3D Engine - Build Script (Linux/macOS)"
echo "============================================="
echo

if ! command -v emcmake &> /dev/null; then
    echo "ERROR: Emscripten not found!"
    echo "Please install Emscripten SDK and activate it first:"
    echo "  1. Install from: https://emscripten.org/docs/getting_started/downloads.html"
    echo "  2. Run: ThirdParty/emsdk/emsdk install latest"
    echo "  3. Run: ThirdParty/emsdk/emsdk activate latest"
    echo "  4. Run: source ThirdParty/emsdk/emsdk_env.sh"
    exit 1
fi

mkdir -p build
cd build

echo "Configuring with CMake..."
emcmake cmake .. -DCMAKE_BUILD_TYPE=Release
if [ $? -ne 0 ]; then
    echo "CMake configuration failed!"
    cd ..
    exit 1
fi

echo
echo "Building..."
emmake make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
if [ $? -ne 0 ]; then
    echo "Build failed!"
    cd ..
    exit 1
fi

cd ..
echo
echo "Build successful!"
echo
echo "Output: build/aether.html"
echo
echo "To run:"
echo "  cd build"
echo "  emrun aether.html"
echo
echo "Or serve with any HTTP server, e.g.:"
echo "  python3 -m http.server 8000 --directory build"
echo "  Then open http://localhost:8000/aether.html"
echo
