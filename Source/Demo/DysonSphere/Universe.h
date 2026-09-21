#pragma once

#include "Core.h"
#include "Math/Color.h"
#include "Math/Vec2.h"
#include "Math/Vec3.h"
#include "Math/Mat4.h"
#include "Planet.h"
#include "DysonSphere.h"
#include "Container/Array.h"
#include "Container/String.h"
#include "Container/RefPtr.h"

namespace DSP {

using Aether::f32;
using Aether::u32;
using Aether::Math::Color;
using Aether::Math::Mat4;
using Aether::Math::Vec2;
using Aether::Math::Vec3;
using Aether::Array;
using Aether::String;
using Aether::UniquePtr;

enum class SpectralType {
    ClassO,       // Blue Giant
    ClassB,       // Blue
    ClassA,       // White
    ClassF,       // Yellow-White
    ClassG,       // Sol (Yellow)
    ClassK,       // Orange
    ClassM,       // Red Dwarf
    NeutronStar,  // Pulsar
    BlackHole,    // Gravitational Lensing
};

const char* SpectralName(SpectralType type);
Color StarColor(SpectralType type);

struct Star {
    u32 Id = 0;
    String Name;
    SpectralType Type = SpectralType::ClassG;
    f32 Luminosity = 1.0f;
    f32 Radius = 350.0f;
    Vec3 PosLy{0.0f, 0.0f, 0.0f}; // Position in light years
    Array<UniquePtr<Planet>> Planets;
    DysonSphereManager DysonSphere;
};

enum class ViewScale {
    MechaClose = 0,
    FactoryBirdView,
    PlanetGlobe,
    StarSystem,
    GalaxyMap,
};

class DSPCamera {
public:
    DSPCamera();

    void Update(f32 dt, const Vec3& targetMechaPos, const Vec3& targetPlanetPos, const Vec3& targetStarPos, f32 mechaAltitude);
    void HandleInput(const Vec2& mouseDelta, f32 wheelDelta, bool rightMouseDown, bool middleMouseDown);

    Vec3 Eye() const { return mEye; }
    Vec3 Target() const { return mTarget; }
    Vec3 Up() const { return mUp; }
    f32 FovDeg() const { return mFovDeg; }
    ViewScale CurrentScale() const { return mScale; }
    f32 ZoomDistance() const { return mDistance; }

    Vec3 Forward() const {
        Vec3 f = {mTarget.x - mEye.x, mTarget.y - mEye.y, mTarget.z - mEye.z};
        f32 flen = sqrtf(f.x * f.x + f.y * f.y + f.z * f.z);
        return (flen > 1e-5f) ? Vec3{f.x / flen, f.y / flen, f.z / flen} : Vec3{0.0f, 0.0f, -1.0f};
    }

    Vec3 Right() const {
        Vec3 f = Forward();
        Vec3 r = {
            f.y * mUp.z - f.z * mUp.y,
            f.z * mUp.x - f.x * mUp.z,
            f.x * mUp.y - f.y * mUp.x
        };
        f32 rlen = sqrtf(r.x * r.x + r.y * r.y + r.z * r.z);
        return (rlen > 1e-5f) ? Vec3{r.x / rlen, r.y / rlen, r.z / rlen} : Vec3{1.0f, 0.0f, 0.0f};
    }

    Mat4 ViewMatrix() const;
    Mat4 ProjectionMatrix(f32 aspect) const;
    Mat4 ViewProjection(f32 aspect) const;

    // Ray from screen pos
    void ScreenToRay(const Vec2& screenPos, f32 screenW, f32 screenH, Vec3& outOrigin, Vec3& outDir) const;

private:
    void UpdateScaleTransition();

    Vec3 mEye{0.0f, 150.0f, 250.0f};
    Vec3 mTarget{0.0f, 0.0f, 200.0f};
    Vec3 mUp{0.0f, 1.0f, 0.0f};

    f32 mDistance = 45.0f;
    f32 mTargetDistance = 45.0f;
    f32 mYaw = 0.0f;
    f32 mPitch = 0.55f;
    f32 mFovDeg = 65.0f;

    Vec3 mFwdTangent{0.0f, 0.0f, 1.0f};

    ViewScale mScale = ViewScale::MechaClose;
};

class Universe {
public:
    Universe(u32 seed = 2026);

    void Update(f32 dt);

    const Array<UniquePtr<Star>>& Stars() const { return mStars; }
    Array<UniquePtr<Star>>& Stars() { return mStars; }
    Star* CurrentStar() { return mStars.IsEmpty() ? nullptr : mStars[mCurrentStarIdx].Get(); }
    Planet* CurrentPlanet();

    void SelectStar(u32 idx) {
        if (idx < mStars.Count()) {
            mCurrentStarIdx = idx;
            mCurrentPlanetIdx = 0;
        }
    }
    void SelectPlanet(u32 idx) {
        Star* s = CurrentStar();
        if (s && idx < s->Planets.Count()) mCurrentPlanetIdx = idx;
    }

    DSPCamera& Camera() { return mCamera; }
    const DSPCamera& Camera() const { return mCamera; }

private:
    void GenerateStarCluster(u32 seed);

    Array<UniquePtr<Star>> mStars;
    u32 mCurrentStarIdx = 0;
    u32 mCurrentPlanetIdx = 0;
    DSPCamera mCamera;
};

}
