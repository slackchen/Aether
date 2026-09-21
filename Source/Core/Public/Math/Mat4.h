#pragma once

#include "Core.h"
#include "Math/Vec3.h"
#include "Math/Vec4.h"

#include <cmath>

namespace Aether::Math {

//
// 4x4 column-major matrix, DirectX-style (m[column * 4 + row]).
//
struct Mat4
{
    f32 m[16] = {0.0f};

    static Mat4 Identity()
    {
        Mat4 out;
        out.m[0] = 1.0f;
        out.m[5] = 1.0f;
        out.m[10] = 1.0f;
        out.m[15] = 1.0f;
        return out;
    }

    static Mat4 Translation(f32 x, f32 y, f32 z)
    {
        Mat4 out = Identity();
        out.m[12] = x;
        out.m[13] = y;
        out.m[14] = z;
        return out;
    }

    static Mat4 Scale(f32 x, f32 y, f32 z)
    {
        Mat4 out;
        out.m[0] = x;
        out.m[5] = y;
        out.m[10] = z;
        out.m[15] = 1.0f;
        return out;
    }

    static Mat4 RotateX(f32 radians)
    {
        f32 c = std::cos(radians);
        f32 s = std::sin(radians);
        Mat4 out = Identity();
        out.m[5] = c;
        out.m[6] = s;
        out.m[9] = -s;
        out.m[10] = c;
        return out;
    }

    static Mat4 RotateY(f32 radians)
    {
        f32 c = std::cos(radians);
        f32 s = std::sin(radians);
        Mat4 out = Identity();
        out.m[0] = c;
        out.m[2] = s;
        out.m[8] = -s;
        out.m[10] = c;
        return out;
    }

    static Mat4 RotateZ(f32 radians)
    {
        f32 c = std::cos(radians);
        f32 s = std::sin(radians);
        Mat4 out = Identity();
        out.m[0] = c;
        out.m[1] = s;
        out.m[4] = -s;
        out.m[5] = c;
        return out;
    }

    // DirectX-style perspective, depth range [0, 1].
    static Mat4 Perspective(f32 fovYRadians, f32 aspect, f32 zn, f32 zf)
    {
        f32 f = 1.0f / std::tan(fovYRadians * 0.5f);
        Mat4 out;
        out.m[0] = f / aspect;
        out.m[5] = f;
        out.m[10] = (zf + zn) / (zn - zf);
        out.m[11] = -1.0f;
        out.m[14] = (2.0f * zf * zn) / (zn - zf);
        return out;
    }

    static Mat4 Ortho(f32 left, f32 right, f32 top, f32 bottom, f32 zn, f32 zf)
    {
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

    static Mat4 LookAt(const Vec3& eye, const Vec3& target, const Vec3& up)
    {
        Vec3 zaxis = {eye.x - target.x, eye.y - target.y, eye.z - target.z};
        f32 zlen = std::sqrt(zaxis.x * zaxis.x + zaxis.y * zaxis.y + zaxis.z * zaxis.z);
        if (zlen > 1e-6f)
        {
            zaxis.x /= zlen;
            zaxis.y /= zlen;
            zaxis.z /= zlen;
        }
        else
        {
            zaxis = {0.0f, 0.0f, 1.0f};
        }

        Vec3 curUp = up;
        Vec3 xaxis = {
            curUp.y * zaxis.z - curUp.z * zaxis.y,
            curUp.z * zaxis.x - curUp.x * zaxis.z,
            curUp.x * zaxis.y - curUp.y * zaxis.x,
        };
        f32 xlen = std::sqrt(xaxis.x * xaxis.x + xaxis.y * xaxis.y + xaxis.z * zaxis.z);
        if (xlen <= 1e-5f)
        {
            curUp = (std::fabs(zaxis.y) < 0.9f) ? Vec3{0.0f, 1.0f, 0.0f} : Vec3{0.0f, 0.0f, 1.0f};
            xaxis = {
                curUp.y * zaxis.z - curUp.z * zaxis.y,
                curUp.z * zaxis.x - curUp.x * zaxis.z,
                curUp.x * zaxis.y - curUp.y * zaxis.x,
            };
            xlen = std::sqrt(xaxis.x * xaxis.x + xaxis.y * xaxis.y + xaxis.z * zaxis.z);
        }
        if (xlen > 1e-6f)
        {
            xaxis.x /= xlen;
            xaxis.y /= xlen;
            xaxis.z /= xlen;
        }
        else
        {
            xaxis = {1.0f, 0.0f, 0.0f};
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

    f32& operator()(u32 row, u32 col) { return m[col * 4 + row]; }
    const f32& operator()(u32 row, u32 col) const { return m[col * 4 + row]; }

    Mat4 operator*(const Mat4& other) const
    {
        Mat4 out;
        for (u32 c = 0; c < 4; ++c)
        {
            for (u32 r = 0; r < 4; ++r)
            {
                f32 sum = 0.0f;
                for (u32 k = 0; k < 4; ++k)
                {
                    sum += m[k * 4 + r] * other.m[c * 4 + k];
                }
                out.m[c * 4 + r] = sum;
            }
        }
        return out;
    }

    Mat4& operator*=(const Mat4& other)
    {
        *this = *this * other;
        return *this;
    }

    Vec4 Transform(const Vec4& v) const
    {
        return {
            m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12] * v.w,
            m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13] * v.w,
            m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14] * v.w,
            m[3] * v.x + m[7] * v.y + m[11] * v.z + m[15] * v.w,
        };
    }

    Vec3 TransformPoint(const Vec3& v) const
    {
        Vec4 result = Transform(Vec4(v.x, v.y, v.z, 1.0f));
        return {result.x, result.y, result.z};
    }
};

}
