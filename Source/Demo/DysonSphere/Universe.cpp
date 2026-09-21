#include "Universe.h"
#include "Math/Math.h"

#include <cmath>

namespace DSP {

using namespace Aether;
using namespace Aether::Math;

const char* SpectralName(SpectralType type)
{
    switch (type)
    {
        case SpectralType::ClassO: return "O型 蓝超巨星";
        case SpectralType::ClassB: return "B型 蓝巨星";
        case SpectralType::ClassA: return "A型 白巨星";
        case SpectralType::ClassF: return "F型 黄白星";
        case SpectralType::ClassG: return "G型 黄矮星 (类太阳)";
        case SpectralType::ClassK: return "K型 橙矮星";
        case SpectralType::ClassM: return "M型 红矮星";
        case SpectralType::NeutronStar: return "中子星 (脉冲星)";
        case SpectralType::BlackHole: return "恒星级天体黑洞";
        default: return "未知恒星";
    }
}

Color StarColor(SpectralType type)
{
    switch (type)
    {
        case SpectralType::ClassO: return Color{0.45f, 0.70f, 1.0f, 1.0f};
        case SpectralType::ClassB: return Color{0.65f, 0.85f, 1.0f, 1.0f};
        case SpectralType::ClassA: return Color{0.92f, 0.96f, 1.0f, 1.0f};
        case SpectralType::ClassF: return Color{1.0f, 0.98f, 0.85f, 1.0f};
        case SpectralType::ClassG: return Color{1.0f, 0.88f, 0.35f, 1.0f};
        case SpectralType::ClassK: return Color{1.0f, 0.65f, 0.20f, 1.0f};
        case SpectralType::ClassM: return Color{1.0f, 0.35f, 0.20f, 1.0f};
        case SpectralType::NeutronStar: return Color{0.70f, 0.95f, 1.0f, 1.0f};
        case SpectralType::BlackHole: return Color{0.85f, 0.35f, 0.85f, 1.0f};
        default: return Color{1.0f, 1.0f, 1.0f, 1.0f};
    }
}

DSPCamera::DSPCamera() = default;

void DSPCamera::HandleInput(const Vec2& mouseDelta, f32 wheelDelta, bool rightMouseDown, bool middleMouseDown)
{
    (void)middleMouseDown;
    if (rightMouseDown)
    {
        mYaw += mouseDelta.x * 0.005f;
        mPitch = Math::Clamp(mPitch + mouseDelta.y * 0.005f, 0.10f, Math::HALF_PI * 0.90f);
    }

    if (fabsf(wheelDelta) > 0.001f)
    {
        f32 factor = powf(0.85f, wheelDelta);
        mTargetDistance = Math::Clamp(mTargetDistance * factor, 25.0f, 120000.0f);
    }
}

void DSPCamera::UpdateScaleTransition()
{
    if (mDistance < 60.0f)
    {
        mScale = ViewScale::MechaClose;
    }
    else if (mDistance < 350.0f)
    {
        mScale = ViewScale::FactoryBirdView;
    }
    else if (mDistance < 2500.0f)
    {
        mScale = ViewScale::PlanetGlobe;
    }
    else if (mDistance < 25000.0f)
    {
        mScale = ViewScale::StarSystem;
    }
    else
    {
        mScale = ViewScale::GalaxyMap;
    }
}

void DSPCamera::Update(f32 dt, const Vec3& targetMechaPos, const Vec3& targetPlanetPos, const Vec3& targetStarPos, f32 mechaAltitude)
{
    (void)targetPlanetPos;
    (void)targetStarPos;
    // Dynamic distance tracking based on mecha altitude for seamless one-shot takeoff & landing
    f32 altDistOffset = (mechaAltitude < 15.0f) ? 0.0f :
                        (mechaAltitude < 80.0f) ? (mechaAltitude - 15.0f) * 0.85f :
                        (65.0f * 0.85f + (mechaAltitude - 80.0f) * 1.8f);

    f32 dynamicTargetDist = mTargetDistance + altDistOffset;
    mDistance += (dynamicTargetDist - mDistance) * Math::Min(1.0f, dt * 10.0f);
    UpdateScaleTransition();

    mFovDeg = 65.0f + Math::Clamp(mechaAltitude / 200.0f, 0.0f, 1.0f) * 8.0f;

    // Target always tracks Mecha smoothly
    mTarget.x += (targetMechaPos.x - mTarget.x) * Math::Min(1.0f, dt * 14.0f);
    mTarget.y += (targetMechaPos.y - mTarget.y) * Math::Min(1.0f, dt * 14.0f);
    mTarget.z += (targetMechaPos.z - mTarget.z) * Math::Min(1.0f, dt * 14.0f);

    // Compute surface normal N at mecha pos
    Vec3 normal = {0.0f, 1.0f, 0.0f};
    f32 nlen = sqrtf(targetMechaPos.x * targetMechaPos.x + targetMechaPos.y * targetMechaPos.y + targetMechaPos.z * targetMechaPos.z);
    if (nlen > 1.0f)
    {
        normal = {targetMechaPos.x / nlen, targetMechaPos.y / nlen, targetMechaPos.z / nlen};
    }

    // Continuous Orthonormal Planetary Tangent Frame without 180-degree jumps
    // Smoothly project previous forward tangent onto the new normal plane
    f32 fDotN = mFwdTangent.x * normal.x + mFwdTangent.y * normal.y + mFwdTangent.z * normal.z;
    Vec3 projFwd = {
        mFwdTangent.x - normal.x * fDotN,
        mFwdTangent.y - normal.y * fDotN,
        mFwdTangent.z - normal.z * fDotN
    };
    f32 pfLen = sqrtf(projFwd.x * projFwd.x + projFwd.y * projFwd.y + projFwd.z * projFwd.z);

    if (pfLen > 1e-4f)
    {
        mFwdTangent = {projFwd.x / pfLen, projFwd.y / pfLen, projFwd.z / pfLen};
    }
    else
    {
        // Fallback: If mecha teleports exactly 90 degrees directly onto the forward vector
        mFwdTangent = {1.0f, 0.0f, 0.0f};
    }

    Vec3 north = mFwdTangent;
    Vec3 east = {
        north.y * normal.z - north.z * normal.y,
        north.z * normal.x - north.x * normal.z,
        north.x * normal.y - north.y * normal.x
    };

    f32 cosP = cosf(mPitch);
    f32 sinP = sinf(mPitch);
    f32 cosY = cosf(mYaw);
    f32 sinY = sinf(mYaw);

    // Camera sits behind mecha (-north) elevated (+normal), rotating with yaw
    Vec3 groundBack = {-cosY * north.x - sinY * east.x,
                       -cosY * north.y - sinY * east.y,
                       -cosY * north.z - sinY * east.z};

    Vec3 orbitDir = {
        sinP * normal.x + cosP * groundBack.x,
        sinP * normal.y + cosP * groundBack.y,
        sinP * normal.z + cosP * groundBack.z
    };

    mEye = {
        mTarget.x + mDistance * orbitDir.x,
        mTarget.y + mDistance * orbitDir.y,
        mTarget.z + mDistance * orbitDir.z
    };

    mUp = normal;
}

Mat4 DSPCamera::ViewMatrix() const
{
    return Math::Mat4::LookAt(mEye, mTarget, mUp);
}

Mat4 DSPCamera::ProjectionMatrix(f32 aspect) const
{
    f32 znear = 0.5f;
    f32 zfar = 250000.0f;
    return Math::Mat4::Perspective(mFovDeg * Math::DEG_TO_RAD, aspect, znear, zfar);
}

Mat4 DSPCamera::ViewProjection(f32 aspect) const
{
    return ProjectionMatrix(aspect) * ViewMatrix();
}

void DSPCamera::ScreenToRay(const Vec2& screenPos, f32 screenW, f32 screenH, Vec3& outOrigin, Vec3& outDir) const
{
    outOrigin = mEye;

    f32 ndcX = (2.0f * screenPos.x) / screenW - 1.0f;
    f32 ndcY = 1.0f - (2.0f * screenPos.y) / screenH;

    f32 tanHalfFov = tanf(mFovDeg * 0.5f * Math::DEG_TO_RAD);
    f32 aspect = screenW / (screenH > 0.0f ? screenH : 1.0f);

    Vec3 fwd = Forward();
    Vec3 rgt = Right();
    Vec3 camUp = {
        rgt.y * fwd.z - rgt.z * fwd.y,
        rgt.z * fwd.x - rgt.x * fwd.z,
        rgt.x * fwd.y - rgt.y * fwd.x
    };

    outDir = {
        fwd.x + rgt.x * ndcX * tanHalfFov * aspect + camUp.x * ndcY * tanHalfFov,
        fwd.y + rgt.y * ndcX * tanHalfFov * aspect + camUp.y * ndcY * tanHalfFov,
        fwd.z + rgt.z * ndcX * tanHalfFov * aspect + camUp.z * ndcY * tanHalfFov
    };
    f32 dlen = sqrtf(outDir.x * outDir.x + outDir.y * outDir.y + outDir.z * outDir.z);
    if (dlen > 1e-5f) { outDir.x /= dlen; outDir.y /= dlen; outDir.z /= dlen; }
}

Universe::Universe(u32 seed)
{
    GenerateStarCluster(seed);
}

void Universe::GenerateStarCluster(u32 seed)
{
    mStars.Clear();

    // Star 0: Starter Sol System
    {
        UniquePtr<Star> s = MakeUnique<Star>();
        s->Id = 0;
        s->Name = "伊卡洛斯主恒星 (母星系)";
        s->Type = SpectralType::ClassG;
        s->Luminosity = 1.0f;
        s->Radius = 350.0f;
        s->PosLy = {0.0f, 0.0f, 0.0f};

        // Planet 0: Starter Mediterranean
        s->Planets.Add(MakeUnique<Planet>(seed + 1, "伊卡洛斯 I (地中海母星)", BiomeType::Mediterranean, 500.0f, 1.0f, 0.04f));
        // Planet 1: Desert Titanium Rich
        s->Planets.Add(MakeUnique<Planet>(seed + 2, "伊卡洛斯 II (戈壁荒漠)", BiomeType::Desert, 460.0f, 1.8f, 0.025f));
        // Planet 2: Ice Planet
        s->Planets.Add(MakeUnique<Planet>(seed + 3, "伊卡洛斯 III (远日冰川)", BiomeType::Ice, 420.0f, 2.7f, 0.018f));
        // Planet 3: Gas Giant
        s->Planets.Add(MakeUnique<Planet>(seed + 4, "伊卡洛斯 IV (气态巨行星)", BiomeType::GasGiant, 950.0f, 4.5f, 0.010f));

        mStars.Add(std::move(s));
    }

    // Star 1: Sirius (Class A White)
    {
        UniquePtr<Star> s = MakeUnique<Star>();
        s->Id = 1;
        s->Name = "天狼星 (A型白巨星)";
        s->Type = SpectralType::ClassA;
        s->Luminosity = 1.75f;
        s->Radius = 600.0f;
        s->PosLy = {8.6f, 2.1f, -4.5f};
        s->Planets.Add(MakeUnique<Planet>(seed + 10, "天狼 I (熔岩火山)", BiomeType::Volcanic, 520.0f, 1.2f, 0.05f));
        s->Planets.Add(MakeUnique<Planet>(seed + 11, "天狼 II (干旱沙丘)", BiomeType::Desert, 480.0f, 2.2f, 0.03f));
        mStars.Add(std::move(s));
    }

    // Star 2: Cygnus X-1 (Black Hole)
    {
        UniquePtr<Star> s = MakeUnique<Star>();
        s->Id = 2;
        s->Name = "天鹅座 X-1 (天体黑洞)";
        s->Type = SpectralType::BlackHole;
        s->Luminosity = 0.05f;
        s->Radius = 350.0f;
        s->PosLy = {-12.4f, 6.8f, 11.2f};
        s->Planets.Add(MakeUnique<Planet>(seed + 20, "天鹅座 I (深渊死星)", BiomeType::Volcanic, 440.0f, 2.5f, 0.08f));
        mStars.Add(std::move(s));
    }

    // Star 3: Vega (Class O Blue Giant)
    {
        UniquePtr<Star> s = MakeUnique<Star>();
        s->Id = 3;
        s->Name = "织女星 (O型蓝巨星)";
        s->Type = SpectralType::ClassO;
        s->Luminosity = 2.45f;
        s->Radius = 800.0f;
        s->PosLy = {15.4f, -8.3f, 7.2f};
        s->Planets.Add(MakeUnique<Planet>(seed + 30, "织女 I (极寒冻土)", BiomeType::Ice, 540.0f, 2.0f, 0.03f));
        s->Planets.Add(MakeUnique<Planet>(seed + 31, "织女 II (类地海洋)", BiomeType::Mediterranean, 500.0f, 3.2f, 0.02f));
        mStars.Add(std::move(s));
    }
}

Planet* Universe::CurrentPlanet()
{
    Star* s = CurrentStar();
    if (!s || s->Planets.IsEmpty()) return nullptr;
    if (mCurrentPlanetIdx >= s->Planets.Count()) mCurrentPlanetIdx = 0;
    return s->Planets[mCurrentPlanetIdx].Get();
}

void Universe::Update(f32 dt)
{
    for (UniquePtr<Star>& s : mStars)
    {
        for (UniquePtr<Planet>& p : s->Planets)
        {
            p->Update(dt);
        }
        s->DysonSphere.Update(dt, 0, 0);
    }
}

}
