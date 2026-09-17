#pragma once

// ============================================================================
// TerrainField — 地形高度场 (平面系统 → 球形系统的核心)
//
// 坐标系约定 (整个 dsp demo 统一遵守):
//   * 右手系, +Y 是 "高度轴 / 上方向", 地面是 XZ 平面 (X=东, Z=北)。
//   * 平面世界: 高度场 y = height(x, z), 单位为世界米, 海平面 y = 0。
//   * 星球局部空间: 球心在原点, 高度沿表面法线 (径向) 偏移,
//     即 surface(dir) = dir * (radius + sphere_height(dir)), dir 为单位向量。
//
// 平面 → 球形的转换分两层:
//   1. 网格层: 球面由 6 个立方体面 (quad sphere) 拼成, 每个面本身就是一块
//      "平面地形网格" (见 build_planar / World3D::build_sphere),
//      (face, u, v) ∈ [0,1]² 映射到单位方向 (cube_face_point)。
//   2. 高度层: 球面高度 sphere_height(dir) 与平面高度 height(x,z) 使用
//      同一套噪声管线 / 海平面 / 配色 (terrain_color), 只是噪声域分别是
//      3D 方向场与 2D 平面场 (保证跨面无缝)。
// ============================================================================

#include "core/platform.h"
#include "core/math.h"

namespace dsp {

using namespace aether;

class TerrainField {
public:
    TerrainField() = default;
    explicit TerrainField(u32 seed) : seed_(seed ? seed : 1) {}

    void set_seed(u32 seed) { seed_ = seed ? seed : 1; }
    u32 seed() const { return seed_; }

    // --- 平面地形系统 (米) ---
    // 山水地貌: 大陆基面 + 山脊 (ridged) + 丘陵细节, 海平面 0。
    f32 height(f32 x, f32 z) const;
    Vec3 planar_normal(f32 x, f32 z, f32 eps = 2.0f) const;
    // 归一化地形值 0(深海)..1(雪峰), 供瓦片分类
    f32 planar_normalized(f32 x, f32 z) const;

    // --- 球形转换 (米) ---
    // 立方体面参数 (face ∈ 0..5, u,v ∈ [0,1]) → 单位方向。
    // 面定义与 PlanetGrid 完全一致 (planet_grid.cpp kFaces)。
    static Vec3 cube_face_point(i32 face, f32 u, f32 v);
    static void cube_face_basis(i32 face, Vec3& out_u_axis, Vec3& out_v_axis);
    // 方向 → (face, u, v) ∈ [0,1]²
    static void point_to_face(const Vec3& dir, i32& out_face, f32& out_u, f32& out_v);

    f32 sphere_height(const Vec3& unit_dir) const;
    Vec3 sphere_normal(const Vec3& unit_dir, f32 radius, f32 eps = 0.0015f) const;
    f32 sphere_normalized(const Vec3& unit_dir) const;
    // 高频细节噪声 0..1 (森林/草地分布)
    f32 detail3(const Vec3& unit_dir) const;
    f32 detail2(f32 x, f32 z) const;

    // 地形配色: h_norm = 高度归一值 0..1, slope = 坡度 0(平)..1(峭壁),
    // detail = 细节噪声 0..1 (草地/森林区分)。返回线性 RGBA。
    static Color terrain_color(f32 h_norm, f32 slope, f32 detail);

    // 球形高度场的振幅 (世界米), 供瓦片高度量化等使用
    static constexpr f32 kSphereAmp = 6.5f;
    static constexpr f32 kSeaLevel = 0.0f;

private:
    u32 seed_ = 1;
};

} // namespace dsp
