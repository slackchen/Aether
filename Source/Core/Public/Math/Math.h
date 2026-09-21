#pragma once

#include "Core.h"

#include <cmath>

namespace Aether::Math {

constexpr f32 PI = 3.14159265358979323846f;
constexpr f32 TWO_PI = PI * 2.0f;
constexpr f32 HALF_PI = PI * 0.5f;
constexpr f32 DEG_TO_RAD = PI / 180.0f;
constexpr f32 RAD_TO_DEG = 180.0f / PI;
constexpr f32 EPSILON = 1e-6f;

inline f32 DegToRad(f32 deg) { return deg * DEG_TO_RAD; }
inline f32 RadToDeg(f32 rad) { return rad * RAD_TO_DEG; }

template<typename T>
constexpr T Min(T a, T b) { return a < b ? a : b; }

template<typename T>
constexpr T Max(T a, T b) { return a < b ? b : a; }

template<typename T>
constexpr T Clamp(T v, T lo, T hi) { return v < lo ? lo : (hi < v ? hi : v); }

template<typename T>
constexpr T Abs(T v) { return v < T(0) ? -v : v; }

template<typename T>
constexpr T Lerp(T a, T b, T t) { return a + (b - a) * t; }

inline f32 Saturate(f32 v) { return Clamp(v, 0.0f, 1.0f); }

inline f32 SmoothStep(f32 edge0, f32 edge1, f32 x)
{
    f32 t = Saturate((x - edge0) / (edge1 - edge0));
    return t * t * (3.0f - 2.0f * t);
}

inline bool NearlyEqual(f32 a, f32 b, f32 epsilon = EPSILON)
{
    return Abs(a - b) <= epsilon;
}

}
