#pragma once

#include "core/platform.h"
#include "core/math.h"

namespace aether::engine {

struct Camera {
    Vec3 position{0.0f, 0.0f, -3.0f};
    Vec3 target{0.0f, 0.0f, 0.0f};
    Vec3 up{0.0f, 1.0f, 0.0f};
    f32 fov_y_deg = 45.0f;
    f32 near_plane = 0.1f;
    f32 far_plane = 100.0f;

    Mat4 view() const {
        return mat4_look_at(position, target, up);
    }

    Mat4 projection(f32 aspect) const {
        const f32 to_rad = 3.14159265358979323846f / 180.0f;
        return mat4_perspective(fov_y_deg * to_rad, aspect, near_plane, far_plane);
    }

    Mat4 view_projection(f32 aspect) const {
        return mat4_mul(projection(aspect), view());
    }
};

struct Camera2D {
    Vec2 position{0.0f, 0.0f};
    f32 logical_height = 720.0f;
    f32 zoom = 1.0f;

    Mat4 view_projection(f32 aspect) const {
        f32 half_h = logical_height * 0.5f / zoom;
        f32 half_w = half_h * aspect;
        f32 left = position.x - half_w;
        f32 right = position.x + half_w;
        f32 top = position.y - half_h;
        f32 bottom = position.y + half_h;
        return mat4_orthographic(left, right, top, bottom, -1.0f, 1.0f);
    }
};

}
