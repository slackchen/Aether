@echo off
cd /d "%~dp0.."
echo Aether 3D Engine - Build Script (Windows)
echo ==========================================
echo.

where emcmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Emscripten not found!
    echo Please install Emscripten SDK and activate it first:
    echo   1. Install from: https://emscripten.org/docs/getting_started/downloads.html
    echo   2. Run: emsdk install latest
    echo   3. Run: emsdk activate latest
    echo   4. Run: emsdk_env.bat
    exit /b 1
)

if not exist build mkdir build
cd build

echo Configuring with CMake...
emcmake cmake .. -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed!
    cd ..
    exit /b 1
)

echo.
echo Building...
emmake make -j%NUMBER_OF_PROCESSORS%
if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    cd ..
    exit /b 1
)

cd ..
echo.
echo Build successful!
echo.
echo Output: build/aether.html
echo.
echo To run:
echo   cd build
echo   emrun aether.html
echo.
echo Or serve with any HTTP server, e.g.:
echo   python -m http.server 8000 --directory build
echo   Then open http://localhost:8000/aether.html
echo.
