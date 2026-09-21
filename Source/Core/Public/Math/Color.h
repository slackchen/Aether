#pragma once

#include "Core.h"
#include "Math.h"

namespace Aether::Math {

struct Color
{
    f32 r = 1.0f;
    f32 g = 1.0f;
    f32 b = 1.0f;
    f32 a = 1.0f;

    Color operator*(f32 s) const { return {r * s, g * s, b * s, a * s}; }
    Color operator+(const Color& c) const { return {r + c.r, g + c.g, b + c.b, a + c.a}; }
    bool operator==(const Color& c) const { return r == c.r && g == c.g && b == c.b && a == c.a; }
    bool operator!=(const Color& c) const { return !(*this == c); }
};

inline Color Lerp(const Color& a, const Color& b, f32 t)
{
    return {
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a + (b.a - a.a) * t,
    };
}

// Builds a color from 0-255 components.
inline Color ColorRGBA(u8 r, u8 g, u8 b, u8 a = 255)
{
    return {r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f};
}

}
