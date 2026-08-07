#pragma once

#include "core/platform.h"
#include <cmath>

namespace aether::math {

template<typename T>
struct TVec2 {
    T x, y;

    TVec2() : x(0), y(0) {}
    TVec2(T s) : x(s), y(s) {}
    TVec2(T x_, T y_) : x(x_), y(y_) {}

    T& operator[](size_t i) { return (&x)[i]; }
    const T& operator[](size_t i) const { return (&x)[i]; }

    TVec2 operator+(const TVec2& v) const { return {x + v.x, y + v.y}; }
    TVec2 operator-(const TVec2& v) const { return {x - v.x, y - v.y}; }
    TVec2 operator*(T s) const { return {x * s, y * s}; }
    TVec2 operator/(T s) const { return {x / s, y / s}; }
    TVec2& operator+=(const TVec2& v) { x += v.x; y += v.y; return *this; }
    TVec2& operator-=(const TVec2& v) { x -= v.x; y -= v.y; return *this; }
    TVec2& operator*=(T s) { x *= s; y *= s; return *this; }
    TVec2& operator/=(T s) { x /= s; y /= s; return *this; }
    TVec2 operator-() const { return {-x, -y}; }

    T dot(const TVec2& v) const { return x * v.x + y * v.y; }
    T length_sq() const { return dot(*this); }
    T length() const { return std::sqrt(length_sq()); }
    TVec2 normalized() const { T l = length(); return l > 0 ? *this / l : TVec2(0); }
};

using Vec2 = TVec2<f32>;
using Vec2f = TVec2<f32>;
using Vec2d = TVec2<f64>;
using Vec2i = TVec2<i32>;
using Vec2u = TVec2<u32>;

}
