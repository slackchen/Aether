#pragma once

#include "Core.h"
#include "Math/Color.h"
#include "Math/Vec3.h"
#include "Terrain.h"
#include "PlanetGrid.h"
#include "Container/String.h"

namespace DSP {

using Aether::f32;
using Aether::i32;
using Aether::u8;
using Aether::u32;
using Aether::Math::Color;
using Aether::Math::Vec3;
using Aether::String;

enum class ResourceKind {
    None = 0,
    IronOre,
    CopperOre,
    Coal,
    Stone,
    TitaniumOre,
    SiliconOre,
    CrudeOil,
    Count
};

enum class BiomeType {
    Mediterranean,
    Desert,
    Ice,
    Volcanic,
    GasGiant,
};

// 瓦片坐标 (含垂直层, 预留高空建筑)
struct GridPos {
    i32 Face = 0;
    i32 U = 0;
    i32 V = 0;
    i32 AltitudeLayer = 0;

    u32 Key() const { return PlanetGrid::Key((u8)Face, (u8)U, (u8)V); }
    static GridPos FromKey(u32 k, i32 alt = 0) {
        TileCoord c = PlanetGrid::Coord(k);
        return {c.Face, c.U, c.V, alt};
    }
    bool operator==(const GridPos& o) const {
        return Face == o.Face && U == o.U && V == o.V && AltitudeLayer == o.AltitudeLayer;
    }
};

class Planet {
public:
    Planet(u32 seed, const String& name, BiomeType biome, f32 radius, f32 orbitDist, f32 orbitSpeed);

    void Update(f32 dt);

    // --- 地形 ---
    // 连续高度场 (世界米, 沿径向, Y 为高度轴): 由 TerrainField 球面转换而来
    f32 GetTerrainHeight3D(const Vec3& normal) const { return mField.SphereHeight(normal); }
    const TerrainField& GetTerrainField() const { return mField; }
    const PlanetGrid& Grid() const { return mGrid; }
    PlanetGrid& Grid() { return mGrid; }
    Terrain TerrainAt(u32 tileKey) const { return (Terrain)mGrid.GetTile(tileKey).Terrain; }
    bool IsBuildableTile(u32 tileKey) const;
    bool IsWaterTile(u32 tileKey) const { return TerrainWater(TerrainAt(tileKey)); }

    // 矿脉: 返回矿脉种类, 并按量扣减 (返回 false 表示枯竭/无矿)
    bool MineVein(u32 tileKey, u32 amount, ResourceKind& outKind);

    // --- 大气散射模型 ---
    f32 AtmosphereHeight() const { return mAtmoHeight; }
    f32 AtmosphereDensity(f32 altitude) const;
    Color SkyColor(f32 altitude, const Vec3& sunDir, const Vec3& viewUp) const;
    Color HorizonFogColor(f32 altitude, const Vec3& sunDir) const;
    f32 AtmosphericOpacity(f32 altitude) const;

    // --- 几何 ---
    bool Raycast(const Vec3& rayOrigin, const Vec3& rayDir, Vec3& outHit, f32& outDist) const;
    Vec3 LatLonToCartesian(f32 lat, f32 lon, f32 altOffset = 0.0f) const;

    // --- 天体参数 ---
    const String& Name() const { return mName; }
    BiomeType Biome() const { return mBiome; }
    f32 Radius() const { return mRadius; }
    f32 OrbitDistance() const { return mOrbitDist; }
    f32 OrbitAngle() const { return mOrbitAngle; }
    f32 RotationAngle() const { return mRotationAngle; }
    f32 RotationSpeed() const { return mRotationSpeed; }
    Vec3 OrbitalPosition() const;

    Color GroundColor() const;
    Color AtmosphereColor() const;
    // 星球自旋矩阵作用下的表面法线 (世界系)
    Vec3 WorldNormal(u32 tileKey) const;

private:
    u32 mSeed = 0;
    String mName;
    BiomeType mBiome = BiomeType::Mediterranean;
    f32 mRadius = 200.0f;
    f32 mAtmoHeight = 120.0f;
    f32 mOrbitDist = 1.0f;
    f32 mOrbitSpeed = 0.05f;
    f32 mOrbitAngle = 0.0f;
    f32 mRotationSpeed = 0.02f;
    f32 mRotationAngle = 0.0f;
    f32 mAxialTilt = 0.15f;

    TerrainField mField;
    PlanetGrid mGrid;
};

} // namespace DSP
