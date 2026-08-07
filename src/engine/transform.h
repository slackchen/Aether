#pragma once

#include "core/platform.h"
#include "core/math.h"

namespace aether::engine {

struct Transform {
    Vec3 position;
    Vec3 rotation_deg;
    Vec3 scale{1.0f, 1.0f, 1.0f};

    Mat4 model_matrix() const {
        const f32 to_rad = 3.14159265358979323846f / 180.0f;
        Mat4 t = mat4_translate(position.x, position.y, position.z);
        Mat4 rx = mat4_rotate_x(rotation_deg.x * to_rad);
        Mat4 ry = mat4_rotate_y(rotation_deg.y * to_rad);
        Mat4 rz = mat4_rotate_z(rotation_deg.z * to_rad);
        Mat4 s = mat4_scale(scale.x, scale.y, scale.z);
        return mat4_mul(mat4_mul(mat4_mul(t, rx), mat4_mul(ry, rz)), s);
    }
};

}
