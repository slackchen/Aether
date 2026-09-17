#pragma once

// ============================================================================
// PlanetGrid — 立方球 (Quad Sphere) 地图数据结构
//
// 星球表面被划分为 6 个立方体面 × kTilesPerFace × kTilesPerFace 个瓦片。
// 每个瓦片记录: 地形类型 / 色调扰动 / 资源矿脉 (种类+储量) / 建筑占用。
// 所有查询通过 u32 packed key (face*65536 + u*256 + v) 进行, 数据存储在
// 扁平数组中, 内存紧凑且缓存友好。
// ============================================================================

#include "core/platform.h"
#include "core/math.h"
#include "demo/dyson_sphere/terrain.h"
#include <vector>

namespace dsp {

using namespace aether;

enum class Terrain : u8 {
    DeepOcean = 0,   // 深海
    ShallowOcean,    // 浅海
    Beach,           // 沙滩
    Grass,           // 草原
    Forest,          // 森林
    Rock,            // 山地岩石
    Snow,            // 雪峰
    Desert,          // 沙漠
    Ice,             // 冰原
    Tundra,          // 冻土
    Basalt,          // 玄武岩
    Lava,            // 熔岩
    Count
};

// 地形是否允许建造
bool terrain_buildable(Terrain t);
// 地形是否为水域
bool terrain_water(Terrain t);

struct Tile {
    u8 terrain = (u8)Terrain::DeepOcean;
    u8 resource = 0;        // ResourceKind
    u8 tint = 128;          // 色调扰动 0..255
    u8 richness = 0;        // 矿脉富集度 1..3
    i8 height = 0;          // 地形高度, 单位 0.1 世界米 (±12.7m)
    u8 flags = 0;           // bit0: 已平整
    u16 building_id = 0;    // 占用建筑 id, 0 = 空
    u32 resource_amount = 0;
};

struct TileCoord {
    u8 face = 0;
    u8 u = 0;
    u8 v = 0;
};

class PlanetGrid {
public:
    static constexpr u32 kTilesPerFace = 48;
    static constexpr u32 kFaceCount = 6;

    // 生成地形与矿脉。biome 取值与 Planet::BiomeType 一致 (0..4)。
    // 高度场与分类由 field 驱动 (与地形网格共用同一 TerrainField)。
    void generate(u32 seed, int biome, const TerrainField& field);

    u32 tile_count() const { return (u32)tiles_.size(); }

    // key 是稀疏编码 (face*65536 + u*256 + v), 存储是紧凑数组 (face*N*N + u*N + v),
    // 所有数据访问必须经过 index_of 换算, 否则越界。
    static u32 index_of(u32 key) {
        return (((key >> 16) & 0xFFu) * kTilesPerFace + ((key >> 8) & 0xFFu)) * kTilesPerFace + (key & 0xFFu);
    }

    const Tile& tile(u32 key) const { return tiles_[index_of(key)]; }
    Tile& tile(u32 key) { return tiles_[index_of(key)]; }
    const Tile& tile_at(u8 face, u8 u, u8 v) const { return tiles_[(face * kTilesPerFace + u) * kTilesPerFace + v]; }
    Tile& tile_at(u8 face, u8 u, u8 v) { return tiles_[(face * kTilesPerFace + u) * kTilesPerFace + v]; }

    static u32 key(u8 face, u8 u, u8 v) {
        return (u32)face * 65536u + (u32)u * 256u + (u32)v;
    }
    static TileCoord coord(u32 key) {
        return {(u8)(key >> 16), (u8)((key >> 8) & 0xFF), (u8)(key & 0xFF)};
    }

    // 四方向邻居: 0=+u 1=-u 2=+v 3=-v。跨面时自动换算到相邻面, 越界返回 key 自身。
    static u32 neighbor_key(u32 key, int dir);

    // 瓦片中心单位法线 (星球局部坐标系, 旋转前)
    static Vec3 tile_normal(u32 key);
    // 瓦片中心在半径 radius 球面上的位置 (附加 height 偏移)
    Vec3 tile_center(f32 radius, u32 key) const;
    // 瓦片东向/北向切线 (用于建筑朝向)
    static void tile_basis(u32 key, Vec3& out_east, Vec3& out_north);

    // 单位方向 → 瓦片 key
    static u32 normal_to_tile(const Vec3& n);

    // 球体射线检测, 返回命中点与瓦片 key (未命中返回 false)
    static bool raycast(f32 radius, const Vec3& ray_o, const Vec3& ray_d, Vec3& out_hit, u32& out_key);

    // 矿脉生成结果查询: 该瓦片是否矿脉
    bool has_vein(u32 key) const { return tile(key).resource != 0; }

    static constexpr f32 kStep = 2.0f / (f32)kTilesPerFace; // 立方体空间步长

private:
    void generate_terrain(u32 seed, int biome, const TerrainField& field);
    void generate_veins(u32 seed, int biome);

    std::vector<Tile> tiles_; // 6 * N * N
};

} // namespace dsp
