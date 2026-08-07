#pragma once

#include "core/platform.h"
#include <cmath>

namespace aether::math {

template<typename T>
struct TVec4 {
    T x, y, z, w;

    TVec4() : x(0), y(0), z(0), w(0) {}
    TVec4(T s) : x(s), y(s), z(s), w(s) {}
    TVec4(T x_, T y_, T z_, T w_) : x(x_), y(y_), z(z_), w(w_) {}

    T& operator[](size_t i) { return (&x)[i]; }
    const T& operator[](size_t i) const { return (&x)[i]; }

    TVec4 operator+(const TVec4& v) const { return {x + v.x, y + v.y, z + v.z, w + v.w}; }
    TVec4 operator-(const TVec4& v) const { return {x - v.x, y - v.y, z - v.z, w - v.w}; }
    TVec4 operator*(T s) const { return {x * s, y * s, z * s, w * s}; }
    TVec4 operator/(T s) const { return {x / s, y / s, z / s, w / s}; }
    TVec4& operator+=(const TVec4& v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
    TVec4& operator-=(const TVec4& v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
    TVec4& operator*=(T s) { x *= s; y *= s; z *= s; w *= s; return *this; }
    TVec4& operator/=(T s) { x /= s; y /= s; z /= s; w /= s; return *this; }
    TVec4 operator-() const { return {-x, -y, -z, -w}; }

    T dot(const TVec4& v) const { return x * v.x + y * v.y + z * v.z + w * v.w; }
};

using Vec4 = TVec4<f32>;
using Vec4f = TVec4<f32>;
using Vec4d = TVec4<f64>;

}
