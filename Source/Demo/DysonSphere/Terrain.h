#pragma once

// ============================================================================
// TerrainField — 地形高度场 (平面系统 → 球形系统的核心)
//
// 坐标系约定 (整个 dsp demo 统一遵守):
//   * 右手系, +Y 是 "高度轴 / 上方向", 地面是 XZ 平面 (X=东, Z=北)。
//   * 平面世界: 高度场 y = Height(x, z), 单位为世界米, 海平面 y = 0。
//   * 星球局部空间: 球心在原点, 高度沿表面法线 (径向) 偏移,
//     即 surface(dir) = dir * (radius + SphereHeight(dir)), dir 为单位向量。
//
// 平面 → 球形的转换分两层:
//   1. 网格层: 球面由 6 个立方体面 (quad sphere) 拼成, 每个面本身就是一块
//      "平面地形网格" (见 BuildPlanar / World3D::BuildSphere),
//      (face, u, v) ∈ [0,1]² 映射到单位方向 (CubeFacePoint)。
//   2. 高度层: 球面高度 SphereHeight(dir) 与平面高度 Height(x,z) 使用
//      同一套噪声管线 / 海平面 / 配色 (TerrainColor), 只是噪声域分别是
//      3D 方向场与 2D 平面场 (保证跨面无缝)。
// ============================================================================

#include "Core.h"
#include "Math/Color.h"
#include "Math/Vec3.h"

namespace DSP {

using Aether::f32;
using Aether::i32;
using Aether::u32;
using Aether::Math::Color;
using Aether::Math::Vec3;

class TerrainField {
public:
    TerrainField() = default;
    explicit TerrainField(u32 seed) : mSeed(seed ? seed : 1) {}

    void SetSeed(u32 seed) { mSeed = seed ? seed : 1; }
    u32 Seed() const { return mSeed; }

    // --- 平面地形系统 (米) ---
    // 山水地貌: 大陆基面 + 山脊 (ridged) + 丘陵细节, 海平面 0。
    f32 Height(f32 x, f32 z) const;
    Vec3 PlanarNormal(f32 x, f32 z, f32 eps = 2.0f) const;
    // 归一化地形值 0(深海)..1(雪峰), 供瓦片分类
    f32 PlanarNormalized(f32 x, f32 z) const;

    // --- 球形转换 (米) ---
    // 立方体面参数 (face ∈ 0..5, u,v ∈ [0,1]) → 单位方向。
    // 面定义与 PlanetGrid 完全一致 (PlanetGrid.cpp FACES)。
    static Vec3 CubeFacePoint(i32 face, f32 u, f32 v);
    static void CubeFaceBasis(i32 face, Vec3& outUAxis, Vec3& outVAxis);
    // 方向 → (face, u, v) ∈ [0,1]²
    static void PointToFace(const Vec3& dir, i32& outFace, f32& outU, f32& outV);

    f32 SphereHeight(const Vec3& unitDir) const;
    Vec3 SphereNormal(const Vec3& unitDir, f32 radius, f32 eps = 0.0015f) const;
    f32 SphereNormalized(const Vec3& unitDir) const;
    // 高频细节噪声 0..1 (森林/草地分布)
    f32 Detail3(const Vec3& unitDir) const;
    f32 Detail2(f32 x, f32 z) const;

    // 地形配色: hNorm = 高度归一值 0..1, slope = 坡度 0(平)..1(峭壁),
    // detail = 细节噪声 0..1 (草地/森林区分)。返回线性 RGBA。
    static Color TerrainColor(f32 hNorm, f32 slope, f32 detail);

    // 球形高度场的振幅 (世界米), 供瓦片高度量化等使用
    static constexpr f32 SPHERE_AMP = 6.5f;
    static constexpr f32 SEA_LEVEL = 0.0f;

private:
    u32 mSeed = 1;
};

} // namespace DSP
