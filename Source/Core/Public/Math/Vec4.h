#pragma once

#include "Core.h"
#include "Math.h"

#include <cmath>

namespace Aether::Math {

struct Vec4
{
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 z = 0.0f;
    f32 w = 0.0f;

    Vec4() = default;
    Vec4(f32 ix, f32 iy, f32 iz, f32 iw)
        : x(ix)
        , y(iy)
        , z(iz)
        , w(iw)
    {
    }

    f32& operator[](u32 i) { return (&x)[i]; }
    const f32& operator[](u32 i) const { return (&x)[i]; }

    Vec4 operator+(const Vec4& v) const { return {x + v.x, y + v.y, z + v.z, w + v.w}; }
    Vec4 operator-(const Vec4& v) const { return {x - v.x, y - v.y, z - v.z, w - v.w}; }
    Vec4 operator*(f32 s) const { return {x * s, y * s, z * s, w * s}; }
    Vec4 operator-() const { return {-x, -y, -z, -w}; }

    Vec4& operator+=(const Vec4& v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
    Vec4& operator-=(const Vec4& v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
    Vec4& operator*=(f32 s) { x *= s; y *= s; z *= s; w *= s; return *this; }

    bool operator==(const Vec4& v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }
    bool operator!=(const Vec4& v) const { return !(*this == v); }

    f32 Dot(const Vec4& v) const { return x * v.x + y * v.y + z * v.z + w * v.w; }
    f32 LengthSq() const { return Dot(*this); }
    f32 Length() const { return std::sqrt(LengthSq()); }
};

inline Vec4 operator*(f32 s, const Vec4& v) { return v * s; }

}
