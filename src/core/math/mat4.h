#pragma once

#include "core/platform.h"
#include "core/math/vec4.h"
#include "core/math/vec3.h"
#include <cmath>
#include <cstring>

namespace aether::math {

struct Mat4 {
    f32 m[16];

    Mat4() {
        std::memset(m, 0, sizeof(m));
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    static Mat4 identity() {
        return Mat4();
    }

    static Mat4 translation(const Vec3& t) {
        Mat4 r;
        r.m[12] = t.x; r.m[13] = t.y; r.m[14] = t.z;
        return r;
    }

    static Mat4 scale(const Vec3& s) {
        Mat4 r;
        r.m[0] = s.x; r.m[5] = s.y; r.m[10] = s.z;
        return r;
    }

    static Mat4 rotation_x(f32 angle) {
        f32 c = std::cos(angle), s = std::sin(angle);
        Mat4 r;
        r.m[5] = c; r.m[6] = s; r.m[9] = -s; r.m[10] = c;
        return r;
    }

    static Mat4 rotation_y(f32 angle) {
        f32 c = std::cos(angle), s = std::sin(angle);
        Mat4 r;
        r.m[0] = c; r.m[2] = -s; r.m[8] = s; r.m[10] = c;
        return r;
    }

    static Mat4 rotation_z(f32 angle) {
        f32 c = std::cos(angle), s = std::sin(angle);
        Mat4 r;
        r.m[0] = c; r.m[1] = s; r.m[4] = -s; r.m[5] = c;
        return r;
    }

    static Mat4 perspective(f32 fov_y, f32 aspect, f32 near_, f32 far_) {
        Mat4 r;
        f32 f = 1.0f / std::tan(fov_y / 2.0f);
        r.m[0] = f / aspect;
        r.m[5] = f;
        r.m[10] = (far_ + near_) / (near_ - far_);
        r.m[11] = -1.0f;
        r.m[14] = (2.0f * far_ * near_) / (near_ - far_);
        r.m[15] = 0.0f;
        return r;
    }

    static Mat4 look_at(const Vec3& eye, const Vec3& target, const Vec3& up) {
        Vec3 f = (target - eye).normalized();
        Vec3 s = f.cross(up).normalized();
        Vec3 u = s.cross(f);
        Mat4 r;
        r.m[0] = s.x; r.m[4] = s.y; r.m[8] = s.z;
        r.m[1] = u.x; r.m[5] = u.y; r.m[9] = u.z;
        r.m[2] = -f.x; r.m[6] = -f.y; r.m[10] = -f.z;
        r.m[12] = -s.dot(eye); r.m[13] = -u.dot(eye); r.m[14] = f.dot(eye);
        return r;
    }

    f32& operator()(size_t row, size_t col) { return m[row * 4 + col]; }
    const f32& operator()(size_t row, size_t col) const { return m[row * 4 + col]; }

    Vec4 operator*(const Vec4& v) const {
        return {
            m[0]*v.x + m[4]*v.y + m[8]*v.z + m[12]*v.w,
            m[1]*v.x + m[5]*v.y + m[9]*v.z + m[13]*v.w,
            m[2]*v.x + m[6]*v.y + m[10]*v.z + m[14]*v.w,
            m[3]*v.x + m[7]*v.y + m[11]*v.z + m[15]*v.w
        };
    }

    Mat4 operator*(const Mat4& other) const {
        Mat4 r;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                r.m[i*4+j] = 0;
                for (int k = 0; k < 4; k++) {
                    r.m[i*4+j] += m[i*4+k] * other.m[k*4+j];
                }
            }
        }
        return r;
    }

    Mat4& operator*=(const Mat4& other) {
        *this = *this * other;
        return *this;
    }
};

}
