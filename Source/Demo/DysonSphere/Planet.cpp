#include "Planet.h"
#include "Math/Math.h"

#include <cmath>

namespace DSP {

using namespace Aether;
using namespace Aether::Math;

Planet::Planet(u32 seed, const String& name, BiomeType biome, f32 radius, f32 orbitDist, f32 orbitSpeed)
    : mSeed(seed), mName(name), mBiome(biome), mRadius(radius), mOrbitDist(orbitDist), mOrbitSpeed(orbitSpeed)
{
    mAtmoHeight = (mBiome == BiomeType::GasGiant) ? 240.0f : 120.0f;
    mRotationSpeed = 0.0f; // 星球固定, 昼夜由恒星公转产生, 保证建筑/角色坐标系稳定
    mField.SetSeed(mSeed);
    mGrid.Generate(mSeed, (int)mBiome, mField);
}

void Planet::Update(f32 dt)
{
    mOrbitAngle += mOrbitSpeed * dt;
    if (mOrbitAngle > Math::TWO_PI) mOrbitAngle -= Math::TWO_PI;
}

Vec3 Planet::OrbitalPosition() const
{
    f32 dist = mOrbitDist * 4000.0f;
    return {
        dist * cosf(mOrbitAngle),
        dist * sinf(mOrbitAngle) * sinf(mAxialTilt),
        dist * sinf(mOrbitAngle) * cosf(mAxialTilt)
    };
}

Vec3 Planet::LatLonToCartesian(f32 lat, f32 lon, f32 altOffset) const
{
    f32 r = mRadius + altOffset;
    return {
        r * cosf(lat) * sinf(lon),
        r * sinf(lat),
        r * cosf(lat) * cosf(lon)
    };
}

bool Planet::IsBuildableTile(u32 tileKey) const
{
    const Tile& t = mGrid.GetTile(tileKey);
    return t.BuildingId == 0 && TerrainBuildable((Terrain)t.Terrain);
}

bool Planet::MineVein(u32 tileKey, u32 amount, ResourceKind& outKind)
{
    Tile& t = mGrid.GetTile(tileKey);
    if (t.Resource == 0 || t.ResourceAmount == 0) return false;
    outKind = (ResourceKind)t.Resource;
    u32 taken = Math::Min(amount, t.ResourceAmount);
    t.ResourceAmount -= taken;
    return true;
}

f32 Planet::AtmosphereDensity(f32 altitude) const
{
    if (altitude < 0.0f) return 1.0f;
    if (altitude >= mAtmoHeight) return 0.0f;
    return expf(-altitude / 35.0f);
}

f32 Planet::AtmosphericOpacity(f32 altitude) const
{
    if (altitude <= 0.0f) return 1.0f;
    if (altitude >= mAtmoHeight) return 0.0f;
    f32 t = 1.0f - (altitude / mAtmoHeight);
    return t * t;
}

Color Planet::SkyColor(f32 altitude, const Vec3& sunDir, const Vec3& viewUp) const
{
    f32 density = AtmosphereDensity(altitude);
    if (density <= 0.001f) return Color{0.0f, 0.0f, 0.0f, 0.0f};

    f32 sunDot = viewUp.x * sunDir.x + viewUp.y * sunDir.y + viewUp.z * sunDir.z;

    Color daySky = (mBiome == BiomeType::Mediterranean) ? Color{0.22f, 0.58f, 0.96f, 1.0f} :
                   (mBiome == BiomeType::Desert) ? Color{0.88f, 0.70f, 0.45f, 1.0f} :
                   (mBiome == BiomeType::Ice) ? Color{0.45f, 0.75f, 0.98f, 1.0f} :
                   (mBiome == BiomeType::Volcanic) ? Color{0.85f, 0.30f, 0.15f, 1.0f} :
                   Color{0.40f, 0.80f, 0.75f, 1.0f};
    Color sunsetSky = Color{0.98f, 0.42f, 0.12f, 1.0f};
    Color nightSky = Color{0.015f, 0.02f, 0.06f, 1.0f};

    Color baseSky;
    if (sunDot > 0.25f)
    {
        baseSky = daySky;
    }
    else if (sunDot > -0.15f)
    {
        f32 factor = (sunDot + 0.15f) / 0.40f;
        baseSky = {
            nightSky.r * (1.0f - factor) + sunsetSky.r * factor,
            nightSky.g * (1.0f - factor) + sunsetSky.g * factor,
            nightSky.b * (1.0f - factor) + sunsetSky.b * factor,
            1.0f
        };
    }
    else
    {
        baseSky = nightSky;
    }
    baseSky.a = density;
    return baseSky;
}

Color Planet::HorizonFogColor(f32 altitude, const Vec3& sunDir) const
{
    f32 density = AtmosphereDensity(altitude);
    Color sky = SkyColor(altitude, sunDir, {0.0f, 1.0f, 0.0f});
    sky.a = density * 0.65f;
    return sky;
}

bool Planet::Raycast(const Vec3& rayOrigin, const Vec3& rayDir, Vec3& outHit, f32& outDist) const
{
    f32 a = rayDir.x * rayDir.x + rayDir.y * rayDir.y + rayDir.z * rayDir.z;
    f32 b = 2.0f * (rayOrigin.x * rayDir.x + rayOrigin.y * rayDir.y + rayOrigin.z * rayDir.z);
    f32 c = rayOrigin.x * rayOrigin.x + rayOrigin.y * rayOrigin.y + rayOrigin.z * rayOrigin.z - mRadius * mRadius;
    f32 discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0.0f) return false;
    f32 sqrtD = sqrtf(discriminant);
    f32 t0 = (-b - sqrtD) / (2.0f * a);
    f32 t1 = (-b + sqrtD) / (2.0f * a);
    f32 t = (t0 > 0.0f) ? t0 : ((t1 > 0.0f) ? t1 : -1.0f);
    if (t < 0.0f) return false;
    outDist = t;
    outHit = {rayOrigin.x + rayDir.x * t, rayOrigin.y + rayDir.y * t, rayOrigin.z + rayDir.z * t};
    return true;
}

Color Planet::GroundColor() const
{
    switch (mBiome)
    {
        case BiomeType::Mediterranean: return Color{0.18f, 0.48f, 0.72f, 1.0f};
        case BiomeType::Desert: return Color{0.82f, 0.65f, 0.38f, 1.0f};
        case BiomeType::Ice: return Color{0.78f, 0.88f, 0.98f, 1.0f};
        case BiomeType::Volcanic: return Color{0.25f, 0.12f, 0.10f, 1.0f};
        case BiomeType::GasGiant: return Color{0.35f, 0.72f, 0.82f, 1.0f};
    }
    return Color{0.5f, 0.5f, 0.5f, 1.0f};
}

Color Planet::AtmosphereColor() const
{
    switch (mBiome)
    {
        case BiomeType::Mediterranean: return Color{0.25f, 0.65f, 0.98f, 0.75f};
        case BiomeType::Desert: return Color{0.92f, 0.72f, 0.45f, 0.60f};
        case BiomeType::Ice: return Color{0.60f, 0.85f, 1.0f, 0.80f};
        case BiomeType::Volcanic: return Color{0.95f, 0.35f, 0.15f, 0.65f};
        case BiomeType::GasGiant: return Color{0.45f, 0.85f, 0.95f, 0.90f};
    }
    return Color{0.3f, 0.6f, 0.9f, 0.5f};
}

Vec3 Planet::WorldNormal(u32 tileKey) const
{
    return PlanetGrid::TileNormal(tileKey);
}

} // namespace DSP
