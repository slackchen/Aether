#pragma once

#include "core/platform.h"
#include <cmath>

namespace aether::math {

template<typename T>
struct TVec3 {
    T x, y, z;

    TVec3() : x(0), y(0), z(0) {}
    TVec3(T s) : x(s), y(s), z(s) {}
    TVec3(T x_, T y_, T z_) : x(x_), y(y_), z(z_) {}

    T& operator[](size_t i) { return (&x)[i]; }
    const T& operator[](size_t i) const { return (&x)[i]; }

    TVec3 operator+(const TVec3& v) const { return {x + v.x, y + v.y, z + v.z}; }
    TVec3 operator-(const TVec3& v) const { return {x - v.x, y - v.y, z - v.z}; }
    TVec3 operator*(T s) const { return {x * s, y * s, z * s}; }
    TVec3 operator/(T s) const { return {x / s, y / s, z / s}; }
    TVec3& operator+=(const TVec3& v) { x += v.x; y += v.y; z += v.z; return *this; }
    TVec3& operator-=(const TVec3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    TVec3& operator*=(T s) { x *= s; y *= s; z *= s; return *this; }
    TVec3& operator/=(T s) { x /= s; y /= s; z /= s; return *this; }
    TVec3 operator-() const { return {-x, -y, -z}; }

    T dot(const TVec3& v) const { return x * v.x + y * v.y + z * v.z; }
    TVec3 cross(const TVec3& v) const {
        return {y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x};
    }
    T length_sq() const { return dot(*this); }
    T length() const { return std::sqrt(length_sq()); }
    TVec3 normalized() const { T l = length(); return l > 0 ? *this / l : TVec3(0); }
};

using Vec3 = TVec3<f32>;
using Vec3f = TVec3<f32>;
using Vec3d = TVec3<f64>;
using Vec3i = TVec3<i32>;
using Color = TVec3<f32>;

struct Color32 {
    u8 r, g, b, a;
    Color32() : r(0), g(0), b(0), a(255) {}
    Color32(u8 r_, u8 g_, u8 b_, u8 a_ = 255) : r(r_), g(g_), b(b_), a(a_) {}
};

}
