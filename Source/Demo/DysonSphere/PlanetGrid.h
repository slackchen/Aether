#pragma once

// ============================================================================
// PlanetGrid — 立方球 (Quad Sphere) 地图数据结构
//
// 星球表面被划分为 6 个立方体面 × TILES_PER_FACE × TILES_PER_FACE 个瓦片。
// 每个瓦片记录: 地形类型 / 色调扰动 / 资源矿脉 (种类+储量) / 建筑占用。
// 所有查询通过 u32 packed key (face*65536 + u*256 + v) 进行, 数据存储在
// 扁平数组中, 内存紧凑且缓存友好。
// ============================================================================

#include "Core.h"
#include "Math/Vec3.h"
#include "Container/Array.h"
#include "Terrain.h"

namespace DSP {

using Aether::f32;
using Aether::i8;
using Aether::u8;
using Aether::u16;
using Aether::u32;
using Aether::Math::Vec3;
using Aether::Array;

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
bool TerrainBuildable(Terrain t);
// 地形是否为水域
bool TerrainWater(Terrain t);

struct Tile {
    u8 Terrain = (u8)Terrain::DeepOcean;
    u8 Resource = 0;        // ResourceKind
    u8 Tint = 128;          // 色调扰动 0..255
    u8 Richness = 0;        // 矿脉富集度 1..3
    i8 Height = 0;          // 地形高度, 单位 0.1 世界米 (±12.7m)
    u8 Flags = 0;           // bit0: 已平整
    u16 BuildingId = 0;     // 占用建筑 id, 0 = 空
    u32 ResourceAmount = 0;
};

struct TileCoord {
    u8 Face = 0;
    u8 U = 0;
    u8 V = 0;
};

class PlanetGrid {
public:
    static constexpr u32 TILES_PER_FACE = 48;
    static constexpr u32 FACE_COUNT = 6;

    // 生成地形与矿脉。biome 取值与 Planet::BiomeType 一致 (0..4)。
    // 高度场与分类由 field 驱动 (与地形网格共用同一 TerrainField)。
    void Generate(u32 seed, int biome, const TerrainField& field);

    u32 TileCount() const { return mTiles.Count(); }

    // key 是稀疏编码 (face*65536 + u*256 + v), 存储是紧凑数组 (face*N*N + u*N + v),
    // 所有数据访问必须经过 IndexOf 换算, 否则越界。
    static u32 IndexOf(u32 key) {
        return (((key >> 16) & 0xFFu) * TILES_PER_FACE + ((key >> 8) & 0xFFu)) * TILES_PER_FACE + (key & 0xFFu);
    }

    const Tile& GetTile(u32 key) const { return mTiles[IndexOf(key)]; }
    Tile& GetTile(u32 key) { return mTiles[IndexOf(key)]; }
    const Tile& GetTileAt(u8 face, u8 u, u8 v) const { return mTiles[(face * TILES_PER_FACE + u) * TILES_PER_FACE + v]; }
    Tile& GetTileAt(u8 face, u8 u, u8 v) { return mTiles[(face * TILES_PER_FACE + u) * TILES_PER_FACE + v]; }

    static u32 Key(u8 face, u8 u, u8 v) {
        return (u32)face * 65536u + (u32)u * 256u + (u32)v;
    }
    static TileCoord Coord(u32 key) {
        return {(u8)(key >> 16), (u8)((key >> 8) & 0xFF), (u8)(key & 0xFF)};
    }

    // 四方向邻居: 0=+u 1=-u 2=+v 3=-v。跨面时自动换算到相邻面, 越界返回 key 自身。
    static u32 NeighborKey(u32 key, int dir);

    // 瓦片中心单位法线 (星球局部坐标系, 旋转前)
    static Vec3 TileNormal(u32 key);
    // 瓦片中心在半径 radius 球面上的位置 (附加 height 偏移)
    Vec3 TileCenter(f32 radius, u32 key) const;
    // 瓦片东向/北向切线 (用于建筑朝向)
    static void TileBasis(u32 key, Vec3& outEast, Vec3& outNorth);

    // 单位方向 → 瓦片 key
    static u32 NormalToTile(const Vec3& n);

    // 球体射线检测, 返回命中点与瓦片 key (未命中返回 false)
    static bool Raycast(f32 radius, const Vec3& rayO, const Vec3& rayD, Vec3& outHit, u32& outKey);

    // 矿脉生成结果查询: 该瓦片是否矿脉
    bool HasVein(u32 key) const { return GetTile(key).Resource != 0; }

    static constexpr f32 STEP = 2.0f / (f32)TILES_PER_FACE; // 立方体空间步长

private:
    void GenerateTerrain(u32 seed, int biome, const TerrainField& field);
    void GenerateVeins(u32 seed, int biome);

    Array<Tile> mTiles; // 6 * N * N
};

} // namespace DSP
