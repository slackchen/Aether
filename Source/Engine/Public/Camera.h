#pragma once

#include "Core.h"
#include "Math/Mat4.h"
#include "Math/Vec2.h"
#include "Math/Vec3.h"

namespace Aether::Engine {

struct Camera
{
    Math::Vec3 Position{0.0f, 0.0f, -3.0f};
    Math::Vec3 Target{0.0f, 0.0f, 0.0f};
    Math::Vec3 Up{0.0f, 1.0f, 0.0f};
    f32 FovYDeg = 45.0f;
    f32 NearPlane = 0.1f;
    f32 FarPlane = 100.0f;

    Math::Mat4 View() const
    {
        return Math::Mat4::LookAt(Position, Target, Up);
    }

    Math::Mat4 Projection(f32 aspect) const
    {
        return Math::Mat4::Perspective(FovYDeg * Math::DEG_TO_RAD, aspect, NearPlane, FarPlane);
    }

    Math::Mat4 ViewProjection(f32 aspect) const
    {
        return Projection(aspect) * View();
    }
};

struct Camera2D
{
    Math::Vec2 Position{0.0f, 0.0f};
    f32 LogicalHeight = 720.0f;
    f32 Zoom = 1.0f;

    Math::Mat4 ViewProjection(f32 aspect) const
    {
        f32 halfH = LogicalHeight * 0.5f / Zoom;
        f32 halfW = halfH * aspect;
        f32 left = Position.x - halfW;
        f32 right = Position.x + halfW;
        f32 top = Position.y - halfH;
        f32 bottom = Position.y + halfH;
        return Math::Mat4::Ortho(left, right, top, bottom, -1.0f, 1.0f);
    }
};

}
