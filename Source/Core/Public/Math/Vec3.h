#pragma once

#include "Core.h"
#include "Math.h"

#include <cmath>

namespace Aether::Math {

struct Vec3
{
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 z = 0.0f;

    Vec3() = default;
    Vec3(f32 ix, f32 iy, f32 iz)
        : x(ix)
        , y(iy)
        , z(iz)
    {
    }

    f32& operator[](u32 i) { return (&x)[i]; }
    const f32& operator[](u32 i) const { return (&x)[i]; }

    Vec3 operator+(const Vec3& v) const { return {x + v.x, y + v.y, z + v.z}; }
    Vec3 operator-(const Vec3& v) const { return {x - v.x, y - v.y, z - v.z}; }
    Vec3 operator*(const Vec3& v) const { return {x * v.x, y * v.y, z * v.z}; }
    Vec3 operator*(f32 s) const { return {x * s, y * s, z * s}; }
    Vec3 operator/(f32 s) const { return {x / s, y / s, z / s}; }
    Vec3 operator-() const { return {-x, -y, -z}; }

    Vec3& operator+=(const Vec3& v) { x += v.x; y += v.y; z += v.z; return *this; }
    Vec3& operator-=(const Vec3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    Vec3& operator*=(f32 s) { x *= s; y *= s; z *= s; return *this; }
    Vec3& operator/=(f32 s) { x /= s; y /= s; z /= s; return *this; }

    bool operator==(const Vec3& v) const { return x == v.x && y == v.y && z == v.z; }
    bool operator!=(const Vec3& v) const { return !(*this == v); }

    f32 Dot(const Vec3& v) const { return x * v.x + y * v.y + z * v.z; }

    Vec3 Cross(const Vec3& v) const
    {
        return {
            y * v.z - z * v.y,
            z * v.x - x * v.z,
            x * v.y - y * v.x,
        };
    }

    f32 LengthSq() const { return Dot(*this); }
    f32 Length() const { return std::sqrt(LengthSq()); }

    Vec3 Normalized() const
    {
        f32 len = Length();
        return len > EPSILON ? *this / len : Vec3();
    }

    void Normalize()
    {
        f32 len = Length();
        if (len > EPSILON)
        {
            x /= len;
            y /= len;
            z /= len;
        }
    }
};

inline Vec3 operator*(f32 s, const Vec3& v) { return v * s; }

inline f32 Dot(const Vec3& a, const Vec3& b) { return a.Dot(b); }

inline Vec3 Cross(const Vec3& a, const Vec3& b) { return a.Cross(b); }

inline Vec3 Lerp(const Vec3& a, const Vec3& b, f32 t)
{
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t};
}

inline f32 Distance(const Vec3& a, const Vec3& b) { return (b - a).Length(); }

}
