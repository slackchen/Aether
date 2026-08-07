#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <array>
#include <optional>
#include <cassert>

#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
    #include <emscripten/html5.h>
    #include <webgpu/webgpu.h>
    #define AETHER_PLATFORM_WEB 1
#else
    #define AETHER_PLATFORM_WEB 0
#endif

namespace aether {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using f32 = float;
using f64 = double;

}
