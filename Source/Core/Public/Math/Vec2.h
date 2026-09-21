#pragma once

#include "Core.h"
#include "Math.h"

#include <cmath>

namespace Aether::Math {

struct Vec2
{
    f32 x = 0.0f;
    f32 y = 0.0f;

    Vec2() = default;
    Vec2(f32 ix, f32 iy)
        : x(ix)
        , y(iy)
    {
    }

    f32& operator[](u32 i) { return (&x)[i]; }
    const f32& operator[](u32 i) const { return (&x)[i]; }

    Vec2 operator+(const Vec2& v) const { return {x + v.x, y + v.y}; }
    Vec2 operator-(const Vec2& v) const { return {x - v.x, y - v.y}; }
    Vec2 operator*(const Vec2& v) const { return {x * v.x, y * v.y}; }
    Vec2 operator*(f32 s) const { return {x * s, y * s}; }
    Vec2 operator/(f32 s) const { return {x / s, y / s}; }
    Vec2 operator-() const { return {-x, -y}; }

    Vec2& operator+=(const Vec2& v) { x += v.x; y += v.y; return *this; }
    Vec2& operator-=(const Vec2& v) { x -= v.x; y -= v.y; return *this; }
    Vec2& operator*=(f32 s) { x *= s; y *= s; return *this; }
    Vec2& operator/=(f32 s) { x /= s; y /= s; return *this; }

    bool operator==(const Vec2& v) const { return x == v.x && y == v.y; }
    bool operator!=(const Vec2& v) const { return !(*this == v); }

    f32 Dot(const Vec2& v) const { return x * v.x + y * v.y; }
    f32 Cross(const Vec2& v) const { return x * v.y - y * v.x; }
    f32 LengthSq() const { return Dot(*this); }
    f32 Length() const { return std::sqrt(LengthSq()); }

    Vec2 Normalized() const
    {
        f32 len = Length();
        return len > EPSILON ? *this / len : Vec2();
    }

    void Normalize()
    {
        f32 len = Length();
        if (len > EPSILON)
        {
            x /= len;
            y /= len;
        }
    }

    // Rotated by radians around the origin (counter-clockwise in screen
    // coordinates where y points down becomes clockwise).
    Vec2 Rotated(f32 radians) const
    {
        f32 c = std::cos(radians);
        f32 s = std::sin(radians);
        return {x * c - y * s, x * s + y * c};
    }
};

inline Vec2 operator*(f32 s, const Vec2& v) { return v * s; }

inline f32 Dot(const Vec2& a, const Vec2& b) { return a.Dot(b); }

inline Vec2 Lerp(const Vec2& a, const Vec2& b, f32 t)
{
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

inline f32 Distance(const Vec2& a, const Vec2& b) { return (b - a).Length(); }

}
