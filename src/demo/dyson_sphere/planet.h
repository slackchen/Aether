#pragma once

#include "core/platform.h"
#include "core/math.h"
#include "demo/dyson_sphere/terrain.h"
#include "demo/dyson_sphere/planet_grid.h"
#include <string>
#include <memory>

namespace dsp {

using namespace aether;

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
    i32 face = 0;
    i32 u = 0;
    i32 v = 0;
    i32 altitude_layer = 0;

    u32 key() const { return PlanetGrid::key((u8)face, (u8)u, (u8)v); }
    static GridPos from_key(u32 k, i32 alt = 0) {
        TileCoord c = PlanetGrid::coord(k);
        return {c.face, c.u, c.v, alt};
    }
    bool operator==(const GridPos& o) const {
        return face == o.face && u == o.u && v == o.v && altitude_layer == o.altitude_layer;
    }
};

class Planet {
public:
    Planet(u32 seed, const std::string& name, BiomeType biome, f32 radius, f32 orbit_dist, f32 orbit_speed);

    void update(f32 dt);

    // --- 地形 ---
    // 连续高度场 (世界米, 沿径向, Y 为高度轴): 由 TerrainField 球面转换而来
    f32 get_terrain_height_3d(const Vec3& normal) const { return field_.sphere_height(normal); }
    const TerrainField& terrain_field() const { return field_; }
    const PlanetGrid& grid() const { return grid_; }
    PlanetGrid& grid() { return grid_; }
    Terrain terrain_at(u32 tile_key) const { return (Terrain)grid_.tile(tile_key).terrain; }
    bool is_buildable_tile(u32 tile_key) const;
    bool is_water_tile(u32 tile_key) const { return terrain_water(terrain_at(tile_key)); }

    // 矿脉: 返回矿脉种类, 并按量扣减 (返回 false 表示枯竭/无矿)
    bool mine_vein(u32 tile_key, u32 amount, ResourceKind& out_kind);

    // --- 大气散射模型 ---
    f32 atmosphere_height() const { return atmo_height_; }
    f32 atmosphere_density(f32 altitude) const;
    Color sky_color(f32 altitude, const Vec3& sun_dir, const Vec3& view_up) const;
    Color horizon_fog_color(f32 altitude, const Vec3& sun_dir) const;
    f32 atmospheric_opacity(f32 altitude) const;

    // --- 几何 ---
    bool raycast(const Vec3& ray_origin, const Vec3& ray_dir, Vec3& out_hit, f32& out_dist) const;
    Vec3 lat_lon_to_cartesian(f32 lat, f32 lon, f32 alt_offset = 0.0f) const;

    // --- 天体参数 ---
    const std::string& name() const { return name_; }
    BiomeType biome() const { return biome_; }
    f32 radius() const { return radius_; }
    f32 orbit_distance() const { return orbit_dist_; }
    f32 orbit_angle() const { return orbit_angle_; }
    f32 rotation_angle() const { return rotation_angle_; }
    f32 rotation_speed() const { return rotation_speed_; }
    Vec3 orbital_position() const;

    Color ground_color() const;
    Color atmosphere_color() const;
    // 星球自旋矩阵作用下的表面法线 (世界系)
    Vec3 world_normal(u32 tile_key) const;

private:
    u32 seed_ = 0;
    std::string name_;
    BiomeType biome_ = BiomeType::Mediterranean;
    f32 radius_ = 200.0f;
    f32 atmo_height_ = 120.0f;
    f32 orbit_dist_ = 1.0f;
    f32 orbit_speed_ = 0.05f;
    f32 orbit_angle_ = 0.0f;
    f32 rotation_speed_ = 0.02f;
    f32 rotation_angle_ = 0.0f;
    f32 axial_tilt_ = 0.15f;

    TerrainField field_;
    PlanetGrid grid_;
};

} // namespace dsp
