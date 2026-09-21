#pragma once

#include "Core.h"
#include "Math/Mat4.h"
#include "Math/Vec3.h"

namespace Aether::Engine {

struct Transform
{
    Math::Vec3 Position;
    Math::Vec3 RotationDeg;
    Math::Vec3 Scale{1.0f, 1.0f, 1.0f};

    Math::Mat4 ModelMatrix() const
    {
        Math::Mat4 t = Math::Mat4::Translation(Position.x, Position.y, Position.z);
        Math::Mat4 rx = Math::Mat4::RotateX(RotationDeg.x * Math::DEG_TO_RAD);
        Math::Mat4 ry = Math::Mat4::RotateY(RotationDeg.y * Math::DEG_TO_RAD);
        Math::Mat4 rz = Math::Mat4::RotateZ(RotationDeg.z * Math::DEG_TO_RAD);
        Math::Mat4 s = Math::Mat4::Scale(Scale.x, Scale.y, Scale.z);
        return (((t * rx) * ry) * rz) * s;
    }
};

}
