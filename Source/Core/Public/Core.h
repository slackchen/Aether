#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

//
// Platform
//
#ifdef __EMSCRIPTEN__
    #define AETHER_PLATFORM_WEB 1
#else
    #define AETHER_PLATFORM_WEB 0
#endif

//
// Compiler
//
#if defined(_MSC_VER)
    #define AETHER_FORCEINLINE __forceinline
#else
    #define AETHER_FORCEINLINE inline __attribute__((always_inline))
#endif

#define AETHER_ASSERT(cond) assert(cond)
#define AETHER_UNUSED(x) (void)(x)

namespace Aether {

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
