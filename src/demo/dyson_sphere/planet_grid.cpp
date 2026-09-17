#include "demo/dyson_sphere/planet_grid.h"
#include <cmath>
#include <cstdio>
#include <random>
#include <algorithm>

namespace dsp {

static constexpr f32 kPi = 3.14159265358979323846f;

bool terrain_buildable(Terrain t) {
    switch (t) {
        case Terrain::DeepOcean:
        case Terrain::ShallowOcean:
        case Terrain::Lava:
            return false;
        default:
            return true;
    }
}

bool terrain_water(Terrain t) {
    return t == Terrain::DeepOcean || t == Terrain::ShallowOcean;
}

// ---------------------------------------------------------------------------
// 立方体面定义: 每个面给出主轴 d (0=x,1=y,2=z)、正负号, 以及 u/v 轴映射,
// 保证 u × v = 面法线 (右手系, 用于一致的四边形绕向)。
//   face 0: +X  u→+Y  v→+Z      face 1: -X  u→+Z  v→+Y
//   face 2: +Y  u→+Z  v→+X      face 3: -Y  u→+X  v→+Z
//   face 4: +Z  u→+X  v→+Y      face 5: -Z  u→+Y  v→+X
// ---------------------------------------------------------------------------
struct FaceDef { i8 axis; i8 sign; i8 u_axis; i8 u_sign; i8 v_axis; i8 v_sign; };
static const FaceDef kFaces[6] = {
    {0, +1, 1, +1, 2, +1},
    {0, -1, 2, +1, 1, +1},
    {1, +1, 2, +1, 0, +1},
    {1, -1, 0, +1, 2, +1},
    {2, +1, 0, +1, 1, +1},
    {2, -1, 1, +1, 0, +1},
};

static Vec3 cube_center_of_tile(u8 face, u8 u, u8 v) {
    const FaceDef& f = kFaces[face];
    f32 a = -1.0f + ((f32)u + 0.5f) * PlanetGrid::kStep;
    f32 b = -1.0f + ((f32)v + 0.5f) * PlanetGrid::kStep;
    Vec3 p{0.0f, 0.0f, 0.0f};
    p.x = (f.axis == 0) ? (f32)f.sign : (f.u_axis == 0) ? a * f.u_sign : b * f.v_sign;
    p.y = (f.axis == 1) ? (f32)f.sign : (f.u_axis == 1) ? a * f.u_sign : b * f.v_sign;
    p.z = (f.axis == 2) ? (f32)f.sign : (f.u_axis == 2) ? a * f.u_sign : b * f.v_sign;
    return p;
}

static u32 face_of_vector(const Vec3& p) {
    f32 ax = fabsf(p.x), ay = fabsf(p.y), az = fabsf(p.z);
    if (ax >= ay && ax >= az) return p.x >= 0.0f ? 0 : 1;
    if (ay >= az) return p.y >= 0.0f ? 2 : 3;
    return p.z >= 0.0f ? 4 : 5;
}

Vec3 PlanetGrid::tile_normal(u32 key) {
    TileCoord c = coord(key);
    Vec3 p = cube_center_of_tile(c.face, c.u, c.v);
    f32 len = sqrtf(p.x * p.x + p.y * p.y + p.z * p.z);
    return {p.x / len, p.y / len, p.z / len};
}

u32 PlanetGrid::normal_to_tile(const Vec3& n) {
    u32 face = face_of_vector(n);
    const FaceDef& f = kFaces[face];
    f32 du = 0.0f, dv = 0.0f;
    if (f.u_axis == 0) du = n.x * f.u_sign;
    else if (f.u_axis == 1) du = n.y * f.u_sign;
    else du = n.z * f.u_sign;
    if (f.v_axis == 0) dv = n.x * f.v_sign;
    else if (f.v_axis == 1) dv = n.y * f.v_sign;
    else dv = n.z * f.v_sign;

    f32 fu = (du + 1.0f) * 0.5f * (f32)kTilesPerFace;
    f32 fv = (dv + 1.0f) * 0.5f * (f32)kTilesPerFace;
    u32 u = (u32)std::clamp((i32)fu, 0, (i32)kTilesPerFace - 1);
    u32 v = (u32)std::clamp((i32)fv, 0, (i32)kTilesPerFace - 1);
    return key((u8)face, (u8)u, (u8)v);
}

u32 PlanetGrid::neighbor_key(u32 key_in, int dir) {
    TileCoord c = coord(key_in);
    Vec3 p = cube_center_of_tile(c.face, c.u, c.v);
    const FaceDef& f = kFaces[c.face];

    // 沿 u/v 轴在立方体空间移动一步
    f32 step = (dir == 0 || dir == 2) ? kStep : -kStep;
    i8 move_axis = (dir == 0 || dir == 1) ? f.u_axis : f.v_axis;
    i8 move_sign = (dir == 0 || dir == 1) ? f.u_sign : f.v_sign;
    if (move_axis == 0) p.x += step * move_sign;
    else if (move_axis == 1) p.y += step * move_sign;
    else p.z += step * move_sign;

    u32 nface = face_of_vector(p);
    const FaceDef& nf = kFaces[nface];
    f32 du = 0.0f, dv = 0.0f;
    if (nf.u_axis == 0) du = p.x * nf.u_sign;
    else if (nf.u_axis == 1) du = p.y * nf.u_sign;
    else du = p.z * nf.u_sign;
    if (nf.v_axis == 0) dv = p.x * nf.v_sign;
    else if (nf.v_axis == 1) dv = p.y * nf.v_sign;
    else dv = p.z * nf.v_sign;

    f32 fu = (du + 1.0f) * 0.5f * (f32)kTilesPerFace;
    f32 fv = (dv + 1.0f) * 0.5f * (f32)kTilesPerFace;
    i32 u = std::clamp((i32)fu, 0, (i32)kTilesPerFace - 1);
    i32 v = std::clamp((i32)fv, 0, (i32)kTilesPerFace - 1);
    return key((u8)nface, (u8)u, (u8)v);
}

Vec3 PlanetGrid::tile_center(f32 radius, u32 key_in) const {
    const Tile& t = tile(key_in);
    Vec3 n = tile_normal(key_in);
    f32 r = radius + (f32)t.height * 0.1f;
    return {n.x * r, n.y * r, n.z * r};
}

void PlanetGrid::tile_basis(u32 key_in, Vec3& out_east, Vec3& out_north) {
    Vec3 n = tile_normal(key_in);
    // 北向取球的局部 "上" 方向投影
    Vec3 world_up = {0.0f, 1.0f, 0.0f};
    f32 d = n.x * world_up.x + n.y * world_up.y + n.z * world_up.z;
    if (fabsf(d) > 0.99f) world_up = {0.0f, 0.0f, 1.0f};
    f32 dot2 = n.x * world_up.x + n.y * world_up.y + n.z * world_up.z;
    out_north = {world_up.x - n.x * dot2, world_up.y - n.y * dot2, world_up.z - n.z * dot2};
    f32 nl = sqrtf(out_north.x * out_north.x + out_north.y * out_north.y + out_north.z * out_north.z);
    if (nl > 1e-5f) { out_north.x /= nl; out_north.y /= nl; out_north.z /= nl; }
    else out_north = {1.0f, 0.0f, 0.0f};
    out_east = {
        out_north.y * n.z - out_north.z * n.y,
        out_north.z * n.x - out_north.x * n.z,
        out_north.x * n.y - out_north.y * n.x
    };
}

bool PlanetGrid::raycast(f32 radius, const Vec3& ray_o, const Vec3& ray_d, Vec3& out_hit, u32& out_key) {
    f32 a = ray_d.x * ray_d.x + ray_d.y * ray_d.y + ray_d.z * ray_d.z;
    f32 b = 2.0f * (ray_o.x * ray_d.x + ray_o.y * ray_d.y + ray_o.z * ray_d.z);
    f32 c = ray_o.x * ray_o.x + ray_o.y * ray_o.y + ray_o.z * ray_o.z - radius * radius;
    f32 disc = b * b - 4.0f * a * c;
    if (disc < 0.0f) return false;
    f32 sq = sqrtf(disc);
    f32 t0 = (-b - sq) / (2.0f * a);
    f32 t1 = (-b + sq) / (2.0f * a);
    f32 t = (t0 > 0.0f) ? t0 : t1;
    if (t <= 0.0f) return false;
    out_hit = {ray_o.x + ray_d.x * t, ray_o.y + ray_d.y * t, ray_o.z + ray_d.z * t};
    f32 len = sqrtf(out_hit.x * out_hit.x + out_hit.y * out_hit.y + out_hit.z * out_hit.z);
    out_key = normal_to_tile({out_hit.x / len, out_hit.y / len, out_hit.z / len});
    return true;
}

// ---------------------------------------------------------------------------
// 程序化噪声 (细节扰动)
// ---------------------------------------------------------------------------
static f32 hash3d(f32 x, f32 y, f32 z) {
    f32 n = sinf(x * 127.1f + y * 311.7f + z * 74.7f) * 43758.5453123f;
    return n - floorf(n);
}

// ---------------------------------------------------------------------------
// 地形生成
// ---------------------------------------------------------------------------
void PlanetGrid::generate(u32 seed, int biome, const TerrainField& field) {
    tiles_.assign((size_t)kFaceCount * kTilesPerFace * kTilesPerFace, Tile{});
    generate_terrain(seed, biome, field);
    generate_veins(seed, biome);
}

// 分类阈值基于 TerrainField::sphere_normalized (0=深海 .. 1=雪峰),
// 与 3D 地形网格的配色渐变保持一致。
void PlanetGrid::generate_terrain(u32 seed, int biome, const TerrainField& field) {
    f32 so = (f32)seed * 0.17f;
    for (u32 face = 0; face < kFaceCount; face++) {
        for (u32 u = 0; u < kTilesPerFace; u++) {
            for (u32 v = 0; v < kTilesPerFace; v++) {
                u32 k = key((u8)face, (u8)u, (u8)v);
                Vec3 n = tile_normal(k);
                f32 hn = field.sphere_normalized(n);
                f32 detail = field.detail3(n);
                Tile& t = tile(k);
                t.tint = (u8)(hash3d(n.x * 91.7f, n.y * 91.7f, n.z * 91.7f) * 255.0f);
                f32 h_m = field.sphere_height(n);
                t.height = (i8)std::clamp(h_m / 0.1f, -127.0f, 127.0f);
                f32 polar = fabsf(n.y);

                auto set = [&](Terrain tr) { t.terrain = (u8)tr; };

                if (biome == 0) { // Mediterranean
                    if (polar > 0.86f) set(Terrain::Ice);
                    else if (hn < 0.42f) set(Terrain::DeepOcean);
                    else if (hn < 0.48f) set(Terrain::ShallowOcean);
                    else if (hn < 0.52f) set(Terrain::Beach);
                    else if (hn < 0.66f) set(detail > 0.52f ? Terrain::Forest : Terrain::Grass);
                    else if (hn < 0.80f) set(Terrain::Rock);
                    else set(Terrain::Snow);
                } else if (biome == 1) { // Desert
                    if (hn < 0.42f) set(Terrain::Beach);
                    else if (hn < 0.68f) set(detail > 0.62f ? Terrain::Rock : Terrain::Desert);
                    else if (hn < 0.80f) set(Terrain::Rock);
                    else set(Terrain::Snow);
                } else if (biome == 2) { // Ice
                    if (hn < 0.46f) set(Terrain::Ice);
                    else if (hn < 0.58f) set(detail > 0.55f ? Terrain::Tundra : Terrain::Ice);
                    else if (hn < 0.72f) set(Terrain::Rock);
                    else set(Terrain::Snow);
                } else if (biome == 3) { // Volcanic
                    if (hn > 0.72f && detail > 0.55f) set(Terrain::Lava);
                    else if (hn > 0.60f) set(Terrain::Basalt);
                    else set(detail > 0.5f ? Terrain::Basalt : Terrain::Rock);
                } else { // GasGiant — 无地表
                    set(Terrain::DeepOcean);
                }
            }
        }
    }
}

void PlanetGrid::generate_veins(u32 seed, int biome) {
    std::mt19937 rng(seed ^ 0x9E3779B9u);
    auto rand_tile = [&]() -> u32 {
        return key((u8)(rng() % kFaceCount),
                   (u8)(rng() % kTilesPerFace),
                   (u8)(rng() % kTilesPerFace));
    };

    struct VeinDef { int resource; u32 clusters; u32 size_min; u32 size_max; };
    std::vector<VeinDef> defs;
    if (biome == 0) {
        defs = {{1, 8, 5, 10}, {2, 7, 4, 9}, {3, 6, 4, 8}, {4, 5, 4, 8}, {5, 3, 4, 7}, {6, 3, 4, 7}, {7, 5, 2, 3}};
    } else if (biome == 1) {
        defs = {{1, 7, 5, 10}, {2, 6, 4, 8}, {3, 5, 4, 8}, {5, 8, 5, 10}, {6, 8, 5, 10}, {7, 4, 2, 3}};
    } else if (biome == 2) {
        defs = {{1, 6, 4, 8}, {2, 5, 4, 8}, {3, 5, 4, 8}, {4, 4, 4, 8}, {5, 4, 4, 7}, {6, 4, 4, 7}};
    } else if (biome == 3) {
        defs = {{1, 6, 5, 9}, {2, 6, 4, 8}, {3, 6, 4, 8}, {5, 7, 5, 9}, {6, 7, 5, 9}, {7, 4, 2, 3}};
    } else {
        defs = {{1, 4, 3, 6}};
    }

    // resource 编号: 1=IronOre 2=CopperOre 3=Coal 4=Stone 5=Titanium 6=Silicon 7=CrudeOil (ResourceKind)
    for (const auto& def : defs) {
        for (u32 c = 0; c < def.clusters; c++) {
            // 找一个可建造的陆地瓦片作为矿脉中心
            u32 center = rand_tile();
            int tries = 0;
            while ((!terrain_buildable((Terrain)tile(center).terrain)) && tries++ < 64) {
                center = rand_tile();
            }
            if (tries >= 64) continue;

            u32 blob = def.size_min + rng() % (def.size_max - def.size_min + 1);
            std::vector<u32> frontier = {center};
            u32 placed = 0;
            u32 richness = 1 + rng() % 3;
            u32 amount_base = 600000u + (u32)(rng() % 900000u);
            while (placed < blob && !frontier.empty()) {
                u32 idx = rng() % frontier.size();
                u32 k = frontier[idx];
                frontier[idx] = frontier.back();
                frontier.pop_back();
                Tile& t = tile(k);
                if (t.resource != 0 || !terrain_buildable((Terrain)t.terrain)) continue;
                if (def.resource == 7 && terrain_water((Terrain)t.terrain)) {
                    // 原油在浅海也可布置, 但深海不行
                    if (t.terrain == (u8)Terrain::DeepOcean) continue;
                }
                t.resource = (u8)def.resource;
                t.richness = (u8)richness;
                t.resource_amount = amount_base * richness;
                placed++;
                for (int d = 0; d < 4; d++) {
                    u32 nb = neighbor_key(k, d);
                    if (nb != k) frontier.push_back(nb);
                }
            }
        }
    }
}

} // namespace dsp
