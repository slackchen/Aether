#pragma once

#include "core/platform.h"
#include <cmath>

namespace aether {

struct Vec3 {
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 z = 0.0f;
};

struct Vec2 {
    f32 x = 0.0f;
    f32 y = 0.0f;

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(f32 s) const { return {x * s, y * s}; }
    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(f32 s) { x *= s; y *= s; return *this; }
};

inline Vec2 vec2(f32 x, f32 y) { return {x, y}; }

inline Vec2 vec2_normalize(Vec2 v) {
    f32 len = sqrtf(v.x * v.x + v.y * v.y);
    if (len > 1e-6f) {
        v.x /= len;
        v.y /= len;
    }
    return v;
}

inline f32 vec2_length(const Vec2& v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

inline Vec2 vec2_lerp(const Vec2& a, const Vec2& b, f32 t) {
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

struct Color {
    f32 r = 1.0f;
    f32 g = 1.0f;
    f32 b = 1.0f;
    f32 a = 1.0f;

    Color operator*(f32 s) const { return {r * s, g * s, b * s, a * s}; }
};

inline Color color(f32 r, f32 g, f32 b, f32 a = 1.0f) { return {r, g, b, a}; }

struct Mat4 {
    f32 m[16] = {0.0f};
};

inline Mat4 mat4_identity() {
    Mat4 out;
    out.m[0] = 1.0f;
    out.m[5] = 1.0f;
    out.m[10] = 1.0f;
    out.m[15] = 1.0f;
    return out;
}

inline Mat4 mat4_mul(const Mat4& a, const Mat4& b) {
    Mat4 out;
    for (u32 c = 0; c < 4; c++) {
        for (u32 r = 0; r < 4; r++) {
            f32 sum = 0.0f;
            for (u32 k = 0; k < 4; k++) {
                sum += a.m[k * 4 + r] * b.m[c * 4 + k];
            }
            out.m[c * 4 + r] = sum;
        }
    }
    return out;
}

inline Mat4 mat4_translate(f32 x, f32 y, f32 z) {
    Mat4 out = mat4_identity();
    out.m[12] = x;
    out.m[13] = y;
    out.m[14] = z;
    return out;
}

inline Mat4 mat4_scale(f32 x, f32 y, f32 z) {
    Mat4 out;
    out.m[0] = x;
    out.m[5] = y;
    out.m[10] = z;
    out.m[15] = 1.0f;
    return out;
}

inline Mat4 mat4_rotate_x(f32 rad) {
    f32 c = cosf(rad);
    f32 s = sinf(rad);
    Mat4 out = mat4_identity();
    out.m[5] = c;
    out.m[6] = s;
    out.m[9] = -s;
    out.m[10] = c;
    return out;
}

inline Mat4 mat4_rotate_y(f32 rad) {
    f32 c = cosf(rad);
    f32 s = sinf(rad);
    Mat4 out = mat4_identity();
    out.m[0] = c;
    out.m[2] = s;
    out.m[8] = -s;
    out.m[10] = c;
    return out;
}

inline Mat4 mat4_rotate_z(f32 rad) {
    f32 c = cosf(rad);
    f32 s = sinf(rad);
    Mat4 out = mat4_identity();
    out.m[0] = c;
    out.m[1] = s;
    out.m[4] = -s;
    out.m[5] = c;
    return out;
}

inline Mat4 mat4_perspective(f32 fov_y_rad, f32 aspect, f32 zn, f32 zf) {
    f32 f = 1.0f / tanf(fov_y_rad * 0.5f);
    Mat4 out;
    out.m[0] = f / aspect;
    out.m[5] = f;
    out.m[10] = (zf + zn) / (zn - zf);
    out.m[11] = -1.0f;
    out.m[14] = (2.0f * zf * zn) / (zn - zf);
    return out;
}

inline Mat4 mat4_orthographic(f32 left, f32 right, f32 top, f32 bottom, f32 zn, f32 zf) {
    Mat4 out;
    out.m[0] = 2.0f / (right - left);
    out.m[5] = 2.0f / (top - bottom);
    out.m[10] = -2.0f / (zf - zn);
    out.m[12] = -(right + left) / (right - left);
    out.m[13] = -(top + bottom) / (top - bottom);
    out.m[14] = -(zf + zn) / (zf - zn);
    out.m[15] = 1.0f;
    return out;
}

inline Mat4 mat4_look_at(const Vec3& eye, const Vec3& target, const Vec3& up) {
    Vec3 zaxis = {eye.x - target.x, eye.y - target.y, eye.z - target.z};
    f32 zlen = sqrtf(zaxis.x * zaxis.x + zaxis.y * zaxis.y + zaxis.z * zaxis.z);
    if (zlen > 1e-6f) {
        zaxis.x /= zlen;
        zaxis.y /= zlen;
        zaxis.z /= zlen;
    }

    Vec3 xaxis = {
        up.y * zaxis.z - up.z * zaxis.y,
        up.z * zaxis.x - up.x * zaxis.z,
        up.x * zaxis.y - up.y * zaxis.x,
    };
    f32 xlen = sqrtf(xaxis.x * xaxis.x + xaxis.y * xaxis.y + xaxis.z * xaxis.z);
    if (xlen > 1e-6f) {
        xaxis.x /= xlen;
        xaxis.y /= xlen;
        xaxis.z /= xlen;
    }

    Vec3 yaxis = {
        zaxis.y * xaxis.z - zaxis.z * xaxis.y,
        zaxis.z * xaxis.x - zaxis.x * xaxis.z,
        zaxis.x * xaxis.y - zaxis.y * xaxis.x,
    };

    Mat4 out;
    out.m[0] = xaxis.x;
    out.m[1] = yaxis.x;
    out.m[2] = zaxis.x;
    out.m[3] = 0.0f;
    out.m[4] = xaxis.y;
    out.m[5] = yaxis.y;
    out.m[6] = zaxis.y;
    out.m[7] = 0.0f;
    out.m[8] = xaxis.z;
    out.m[9] = yaxis.z;
    out.m[10] = zaxis.z;
    out.m[11] = 0.0f;
    out.m[12] = -(xaxis.x * eye.x + xaxis.y * eye.y + xaxis.z * eye.z);
    out.m[13] = -(yaxis.x * eye.x + yaxis.y * eye.y + yaxis.z * eye.z);
    out.m[14] = -(zaxis.x * eye.x + zaxis.y * eye.y + zaxis.z * eye.z);
    out.m[15] = 1.0f;
    return out;
}

}
