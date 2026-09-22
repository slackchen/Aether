cmake_minimum_required(VERSION 3.16)
project(aether_engine VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

if(EMSCRIPTEN)
    set(AETHER_PLATFORM_WEB TRUE)
else()
    set(AETHER_PLATFORM_WEB FALSE)
endif()

if(AETHER_PLATFORM_WEB)
    # Emit a full HTML page (with --shell-file) instead of Emscripten's default bare .js
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
else()
    set(CMAKE_EXECUTABLE_SUFFIX ".exe")
endif()

if(MSVC)
    add_compile_options(/utf-8)
endif()

#
# Module interface directories (Unreal-style module boundaries). Each module
# exposes only its Public headers; consumers add the Public directory of the
# modules they depend on, and includes are written relative to that module
# root ("Container/Array.h", "RHI.h", ...). Private headers are never on any
# consumer's include path.
#
set(AETHER_CORE_PUBLIC     ${CMAKE_CURRENT_SOURCE_DIR}/Source/Core/Public)
set(AETHER_RHI_PUBLIC      ${CMAKE_CURRENT_SOURCE_DIR}/Source/RHI/Public)
set(AETHER_PLATFORM_PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Source/Platform/Public)
set(AETHER_ENGINE_PUBLIC   ${CMAKE_CURRENT_SOURCE_DIR}/Source/Engine/Public)

set(AETHER_ENGINE_SOURCES
    Source/Platform/Private/Threading/Threading.cpp
    Source/Engine/Private/Renderer.cpp
    Source/Engine/Private/Renderer2D.cpp
    Source/Engine/Private/Mesh.cpp
    Source/Engine/Private/Shaders.cpp
    Source/Engine/Private/Texture.cpp
    Source/Engine/Private/SpriteBatch.cpp
    Source/Engine/Private/ParticleSystem.cpp
    Source/Engine/Private/Starfield.cpp
    Source/Engine/Private/Timer.cpp
    Source/Engine/Private/Input.cpp
    Source/Engine/Private/Audio.cpp
    Source/Engine/Private/UI.cpp
)

if(AETHER_PLATFORM_WEB)
    list(APPEND AETHER_ENGINE_SOURCES
        Source/WebGPU/Private/WebGPUDevice.cpp
        Source/WebGPU/Private/WebGPUSwapchain.cpp
        Source/WebGPU/Private/WebGPUBuffer.cpp
        Source/WebGPU/Private/WebGPUShader.cpp
        Source/WebGPU/Private/WebGPUPipeline.cpp
        Source/Platform/Private/PlatformWeb.cpp
    )
else()
    list(APPEND AETHER_ENGINE_SOURCES
        Source/D3D11/Private/D3D11Device.cpp
        Source/D3D11/Private/D3D11Swapchain.cpp
        Source/D3D11/Private/D3D11Buffer.cpp
        Source/D3D11/Private/D3D11Shader.cpp
        Source/D3D11/Private/D3D11Texture.cpp
        Source/D3D11/Private/D3D11Encoder.cpp
        Source/Platform/Private/PlatformWin32.cpp
    )
endif()

add_library(aether_engine STATIC ${AETHER_ENGINE_SOURCES})

target_include_directories(aether_engine
    PUBLIC
        ${AETHER_CORE_PUBLIC}
        ${AETHER_RHI_PUBLIC}
        ${AETHER_PLATFORM_PUBLIC}
        ${AETHER_ENGINE_PUBLIC}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/Engine/Private
)

if(AETHER_PLATFORM_WEB)
    target_include_directories(aether_engine PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/WebGPU/Private
    )
    target_compile_options(aether_engine PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -O3
        --use-port=emdawnwebgpu
    )
else()
    target_include_directories(aether_engine PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/D3D11/Private
    )
    target_compile_options(aether_engine PRIVATE /W3)
    target_compile_definitions(aether_engine PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(aether_engine PUBLIC d3d11 dxgi d3dcompiler dxguid xinput)
endif()

if(AETHER_PLATFORM_WEB)
    set(AETHER_LINK_FLAGS
        "--use-port=emdawnwebgpu -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s NO_EXIT_RUNTIME=1 -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap']"
    )
    set(AETHER_DEFAULT_SHELL Web/shell.html)
endif()

# aether_target(name [SHELL path] sources...)
# Executables inherit the module Public include paths through the link.
function(aether_target name)
    cmake_parse_arguments(AT "" "SHELL" "" ${ARGN})
    add_executable(${name} ${AT_UNPARSED_ARGUMENTS})
    if(AETHER_PLATFORM_WEB)
        target_compile_options(${name} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -O3
            --use-port=emdawnwebgpu
        )
        target_link_libraries(${name} PRIVATE aether_engine)
        if(AT_SHELL)
            set(shell_file ${CMAKE_CURRENT_SOURCE_DIR}/${AT_SHELL})
        else()
            set(shell_file ${CMAKE_CURRENT_SOURCE_DIR}/${AETHER_DEFAULT_SHELL})
        endif()
        set_target_properties(${name} PROPERTIES LINK_FLAGS "${AETHER_LINK_FLAGS} --shell-file ${shell_file}")
    else()
        target_compile_definitions(${name} PRIVATE _CRT_SECURE_NO_WARNINGS)
        if(MSVC)
            target_compile_options(${name} PRIVATE /utf-8)
        endif()
        target_link_libraries(${name} PRIVATE aether_engine)
    endif()
endfunction()

aether_target(aether Source/main.cpp)
aether_target(aether_demo
    Source/Demo/FlightShmup/Main.cpp
    Source/Demo/FlightShmup/Game.cpp
    Source/Demo/FlightShmup/GameUI.cpp
    Source/Demo/FlightShmup/Art.cpp
)
aether_target(aether_dsp
    SHELL Web/shell_dsp.html
    Source/Demo/DysonSphere/Main.cpp
    Source/Demo/DysonSphere/Game.cpp
    Source/Demo/DysonSphere/Terrain.cpp
    Source/Demo/DysonSphere/World3D.cpp
    Source/Demo/DysonSphere/Planet.cpp
    Source/Demo/DysonSphere/PlanetGrid.cpp
    Source/Demo/DysonSphere/Mecha.cpp
    Source/Demo/DysonSphere/Factory.cpp
    Source/Demo/DysonSphere/Power.cpp
    Source/Demo/DysonSphere/DysonSphere.cpp
    Source/Demo/DysonSphere/TechTree.cpp
    Source/Demo/DysonSphere/Blueprint.cpp
    Source/Demo/DysonSphere/Universe.cpp
    Source/Demo/DysonSphere/DSPUI.cpp
    Source/Demo/DysonSphere/DSPAudio.cpp
    Source/Demo/DysonSphere/DSPArt.cpp
)

if(AETHER_PLATFORM_WEB)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/web)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Web/index.html ${CMAKE_BINARY_DIR}/index.html COPYONLY)
endif()
