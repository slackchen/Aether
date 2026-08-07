#pragma once

#include "core/math/vec2.h"
#include "core/math/vec3.h"
#include "core/math/vec4.h"
#include "core/math/mat4.h"

namespace aether::math {

constexpr f32 PI = 3.14159265358979323846f;
constexpr f32 DEG_TO_RAD = PI / 180.0f;
constexpr f32 RAD_TO_DEG = 180.0f / PI;

inline f32 deg_to_rad(f32 deg) { return deg * DEG_TO_RAD; }
inline f32 rad_to_deg(f32 rad) { return rad * RAD_TO_DEG; }

}
