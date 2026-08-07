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

if(NOT AETHER_PLATFORM_WEB)
    set(CMAKE_EXECUTABLE_SUFFIX ".exe")
endif()

set(AETHER_ENGINE_SOURCES
    src/engine/renderer.cpp
    src/engine/renderer2d.cpp
    src/engine/mesh.cpp
    src/engine/shaders.cpp
    src/engine/texture.cpp
    src/engine/sprite_batch.cpp
    src/engine/particle_system.cpp
    src/engine/starfield.cpp
    src/engine/timer.cpp
    src/engine/input.cpp
    src/engine/audio.cpp
    src/engine/ui.cpp
)

if(AETHER_PLATFORM_WEB)
    list(APPEND AETHER_ENGINE_SOURCES
        src/webgpu/webgpu_device.cpp
        src/webgpu/webgpu_swapchain.cpp
        src/webgpu/webgpu_buffer.cpp
        src/webgpu/webgpu_shader.cpp
        src/webgpu/webgpu_pipeline.cpp
        src/platform/platform_web.cpp
    )
else()
    list(APPEND AETHER_ENGINE_SOURCES
        src/d3d11/d3d11_device.cpp
        src/d3d11/d3d11_swapchain.cpp
        src/d3d11/d3d11_buffer.cpp
        src/d3d11/d3d11_shader.cpp
        src/d3d11/d3d11_texture.cpp
        src/d3d11/d3d11_encoder.cpp
        src/platform/platform_win32.cpp
    )
endif()

add_library(aether_engine STATIC ${AETHER_ENGINE_SOURCES})

target_include_directories(aether_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(AETHER_PLATFORM_WEB)
    target_compile_options(aether_engine PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -O3
        --use-port=emdawnwebgpu
    )
else()
    target_compile_options(aether_engine PRIVATE /W3)
    target_compile_definitions(aether_engine PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(aether_engine PUBLIC d3d11 dxgi d3dcompiler dxguid xinput)
endif()

if(AETHER_PLATFORM_WEB)
    set(AETHER_LINK_FLAGS
        "--use-port=emdawnwebgpu -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s NO_EXIT_RUNTIME=1 -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap'] --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html"
    )
endif()

function(aether_target name)
    add_executable(${name} ${ARGN})
    target_include_directories(${name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    if(AETHER_PLATFORM_WEB)
        target_compile_options(${name} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -O3
            --use-port=emdawnwebgpu
        )
        target_link_libraries(${name} PRIVATE aether_engine)
        set_target_properties(${name} PROPERTIES LINK_FLAGS "${AETHER_LINK_FLAGS}")
    else()
        target_compile_definitions(${name} PRIVATE _CRT_SECURE_NO_WARNINGS)
        target_link_libraries(${name} PRIVATE aether_engine)
    endif()
endfunction()

aether_target(aether src/main.cpp)
aether_target(aether_demo
    src/demo/flight_shmup/main.cpp
    src/demo/flight_shmup/game.cpp
    src/demo/flight_shmup/game_ui.cpp
    src/demo/flight_shmup/art.cpp
)

if(AETHER_PLATFORM_WEB)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/web)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/web/index.html ${CMAKE_BINARY_DIR}/index.html COPYONLY)
endif()
