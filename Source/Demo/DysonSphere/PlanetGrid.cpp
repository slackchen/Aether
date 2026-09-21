#include "PlanetGrid.h"
#include "Random.h"
#include "Math/Math.h"

#include <cmath>

namespace DSP {

using namespace Aether;
using namespace Aether::Math;

bool TerrainBuildable(Terrain t)
{
    switch (t)
    {
        case Terrain::DeepOcean:
        case Terrain::ShallowOcean:
        case Terrain::Lava:
            return false;
        default:
            return true;
    }
}

bool TerrainWater(Terrain t)
{
    return t == Terrain::DeepOcean || t == Terrain::ShallowOcean;
}

// ---------------------------------------------------------------------------
// 立方体面定义: 每个面给出主轴 d (0=x,1=y,2=z)、正负号, 以及 u/v 轴映射,
// 保证 u × v = 面法线 (右手系, 用于一致的四边形绕向)。
//   face 0: +X  u→+Y  v→+Z      face 1: -X  u→+Z  v→+Y
//   face 2: +Y  u→+Z  v→+X      face 3: -Y  u→+X  v→+Z
//   face 4: +Z  u→+X  v→+Y      face 5: -Z  u→+Y  v→+X
// ---------------------------------------------------------------------------
struct FaceDef { i8 Axis; i8 Sign; i8 UAxis; i8 USign; i8 VAxis; i8 VSign; };
static const FaceDef FACES[6] = {
    {0, +1, 1, +1, 2, +1},
    {0, -1, 2, +1, 1, +1},
    {1, +1, 2, +1, 0, +1},
    {1, -1, 0, +1, 2, +1},
    {2, +1, 0, +1, 1, +1},
    {2, -1, 1, +1, 0, +1},
};

static Vec3 CubeCenterOfTile(u8 face, u8 u, u8 v)
{
    const FaceDef& f = FACES[face];
    f32 a = -1.0f + ((f32)u + 0.5f) * PlanetGrid::STEP;
    f32 b = -1.0f + ((f32)v + 0.5f) * PlanetGrid::STEP;
    Vec3 p{0.0f, 0.0f, 0.0f};
    p.x = (f.Axis == 0) ? (f32)f.Sign : (f.UAxis == 0) ? a * f.USign : b * f.VSign;
    p.y = (f.Axis == 1) ? (f32)f.Sign : (f.UAxis == 1) ? a * f.USign : b * f.VSign;
    p.z = (f.Axis == 2) ? (f32)f.Sign : (f.UAxis == 2) ? a * f.USign : b * f.VSign;
    return p;
}

static u32 FaceOfVector(const Vec3& p)
{
    f32 ax = fabsf(p.x), ay = fabsf(p.y), az = fabsf(p.z);
    if (ax >= ay && ax >= az) return p.x >= 0.0f ? 0 : 1;
    if (ay >= az) return p.y >= 0.0f ? 2 : 3;
    return p.z >= 0.0f ? 4 : 5;
}

Vec3 PlanetGrid::TileNormal(u32 key)
{
    TileCoord c = Coord(key);
    Vec3 p = CubeCenterOfTile(c.Face, c.U, c.V);
    f32 len = sqrtf(p.x * p.x + p.y * p.y + p.z * p.z);
    return {p.x / len, p.y / len, p.z / len};
}

u32 PlanetGrid::NormalToTile(const Vec3& n)
{
    u32 face = FaceOfVector(n);
    const FaceDef& f = FACES[face];
    f32 du = 0.0f, dv = 0.0f;
    if (f.UAxis == 0) du = n.x * f.USign;
    else if (f.UAxis == 1) du = n.y * f.USign;
    else du = n.z * f.USign;
    if (f.VAxis == 0) dv = n.x * f.VSign;
    else if (f.VAxis == 1) dv = n.y * f.VSign;
    else dv = n.z * f.VSign;

    f32 fu = (du + 1.0f) * 0.5f * (f32)TILES_PER_FACE;
    f32 fv = (dv + 1.0f) * 0.5f * (f32)TILES_PER_FACE;
    u32 u = (u32)Math::Clamp((i32)fu, 0, (i32)TILES_PER_FACE - 1);
    u32 v = (u32)Math::Clamp((i32)fv, 0, (i32)TILES_PER_FACE - 1);
    return Key((u8)face, (u8)u, (u8)v);
}

u32 PlanetGrid::NeighborKey(u32 keyIn, int dir)
{
    TileCoord c = Coord(keyIn);
    Vec3 p = CubeCenterOfTile(c.Face, c.U, c.V);
    const FaceDef& f = FACES[c.Face];

    // 沿 u/v 轴在立方体空间移动一步
    f32 step = (dir == 0 || dir == 2) ? STEP : -STEP;
    i8 moveAxis = (dir == 0 || dir == 1) ? f.UAxis : f.VAxis;
    i8 moveSign = (dir == 0 || dir == 1) ? f.USign : f.VSign;
    if (moveAxis == 0) p.x += step * moveSign;
    else if (moveAxis == 1) p.y += step * moveSign;
    else p.z += step * moveSign;

    u32 nface = FaceOfVector(p);
    const FaceDef& nf = FACES[nface];
    f32 du = 0.0f, dv = 0.0f;
    if (nf.UAxis == 0) du = p.x * nf.USign;
    else if (nf.UAxis == 1) du = p.y * nf.USign;
    else du = p.z * nf.USign;
    if (nf.VAxis == 0) dv = p.x * nf.VSign;
    else if (nf.VAxis == 1) dv = p.y * nf.VSign;
    else dv = p.z * nf.VSign;

    f32 fu = (du + 1.0f) * 0.5f * (f32)TILES_PER_FACE;
    f32 fv = (dv + 1.0f) * 0.5f * (f32)TILES_PER_FACE;
    i32 u = Math::Clamp((i32)fu, 0, (i32)TILES_PER_FACE - 1);
    i32 v = Math::Clamp((i32)fv, 0, (i32)TILES_PER_FACE - 1);
    return Key((u8)nface, (u8)u, (u8)v);
}

Vec3 PlanetGrid::TileCenter(f32 radius, u32 keyIn) const
{
    const Tile& t = GetTile(keyIn);
    Vec3 n = TileNormal(keyIn);
    f32 r = radius + (f32)t.Height * 0.1f;
    return {n.x * r, n.y * r, n.z * r};
}

void PlanetGrid::TileBasis(u32 keyIn, Vec3& outEast, Vec3& outNorth)
{
    Vec3 n = TileNormal(keyIn);
    // 北向取球的局部 "上" 方向投影
    Vec3 worldUp = {0.0f, 1.0f, 0.0f};
    f32 d = n.x * worldUp.x + n.y * worldUp.y + n.z * worldUp.z;
    if (fabsf(d) > 0.99f) worldUp = {0.0f, 0.0f, 1.0f};
    f32 dot2 = n.x * worldUp.x + n.y * worldUp.y + n.z * worldUp.z;
    outNorth = {worldUp.x - n.x * dot2, worldUp.y - n.y * dot2, worldUp.z - n.z * dot2};
    f32 nl = sqrtf(outNorth.x * outNorth.x + outNorth.y * outNorth.y + outNorth.z * outNorth.z);
    if (nl > 1e-5f) { outNorth.x /= nl; outNorth.y /= nl; outNorth.z /= nl; }
    else outNorth = {1.0f, 0.0f, 0.0f};
    outEast = {
        outNorth.y * n.z - outNorth.z * n.y,
        outNorth.z * n.x - outNorth.x * n.z,
        outNorth.x * n.y - outNorth.y * n.x
    };
}

bool PlanetGrid::Raycast(f32 radius, const Vec3& rayO, const Vec3& rayD, Vec3& outHit, u32& outKey)
{
    f32 a = rayD.x * rayD.x + rayD.y * rayD.y + rayD.z * rayD.z;
    f32 b = 2.0f * (rayO.x * rayD.x + rayO.y * rayD.y + rayO.z * rayD.z);
    f32 c = rayO.x * rayO.x + rayO.y * rayO.y + rayO.z * rayO.z - radius * radius;
    f32 disc = b * b - 4.0f * a * c;
    if (disc < 0.0f) return false;
    f32 sq = sqrtf(disc);
    f32 t0 = (-b - sq) / (2.0f * a);
    f32 t1 = (-b + sq) / (2.0f * a);
    f32 t = (t0 > 0.0f) ? t0 : t1;
    if (t <= 0.0f) return false;
    outHit = {rayO.x + rayD.x * t, rayO.y + rayD.y * t, rayO.z + rayD.z * t};
    f32 len = sqrtf(outHit.x * outHit.x + outHit.y * outHit.y + outHit.z * outHit.z);
    outKey = NormalToTile({outHit.x / len, outHit.y / len, outHit.z / len});
    return true;
}

// ---------------------------------------------------------------------------
// 程序化噪声 (细节扰动)
// ---------------------------------------------------------------------------
static f32 Hash3D(f32 x, f32 y, f32 z)
{
    f32 n = sinf(x * 127.1f + y * 311.7f + z * 74.7f) * 43758.5453123f;
    return n - floorf(n);
}

// ---------------------------------------------------------------------------
// 地形生成
// ---------------------------------------------------------------------------
void PlanetGrid::Generate(u32 seed, int biome, const TerrainField& field)
{
    mTiles.Resize((u64)FACE_COUNT * TILES_PER_FACE * TILES_PER_FACE);
    GenerateTerrain(seed, biome, field);
    GenerateVeins(seed, biome);
}

// 分类阈值基于 TerrainField::SphereNormalized (0=深海 .. 1=雪峰),
// 与 3D 地形网格的配色渐变保持一致。
void PlanetGrid::GenerateTerrain(u32 seed, int biome, const TerrainField& field)
{
    (void)seed;
    for (u32 face = 0; face < FACE_COUNT; face++)
    {
        for (u32 u = 0; u < TILES_PER_FACE; u++)
        {
            for (u32 v = 0; v < TILES_PER_FACE; v++)
            {
                u32 k = Key((u8)face, (u8)u, (u8)v);
                Vec3 n = TileNormal(k);
                f32 hn = field.SphereNormalized(n);
                f32 detail = field.Detail3(n);
                Tile& t = GetTile(k);
                t.Tint = (u8)(Hash3D(n.x * 91.7f, n.y * 91.7f, n.z * 91.7f) * 255.0f);
                f32 hM = field.SphereHeight(n);
                t.Height = (i8)Math::Clamp(hM / 0.1f, -127.0f, 127.0f);
                f32 polar = fabsf(n.y);

                auto set = [&](Terrain tr) { t.Terrain = (u8)tr; };

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

void PlanetGrid::GenerateVeins(u32 seed, int biome)
{
    Random rng(seed ^ 0x9E3779B9u);
    auto RandTile = [&]() -> u32 {
        return Key((u8)(rng.NextU64() % FACE_COUNT),
                   (u8)(rng.NextU64() % TILES_PER_FACE),
                   (u8)(rng.NextU64() % TILES_PER_FACE));
    };

    struct VeinDef { int Resource; u32 Clusters; u32 SizeMin; u32 SizeMax; };
    Array<VeinDef> defs;
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

    // Resource 编号: 1=IronOre 2=CopperOre 3=Coal 4=Stone 5=Titanium 6=Silicon 7=CrudeOil (ResourceKind)
    for (const VeinDef& def : defs)
    {
        for (u32 c = 0; c < def.Clusters; c++)
        {
            // 找一个可建造的陆地瓦片作为矿脉中心
            u32 center = RandTile();
            int tries = 0;
            while ((!TerrainBuildable((Terrain)GetTile(center).Terrain)) && tries++ < 64)
            {
                center = RandTile();
            }
            if (tries >= 64) continue;

            u32 blob = def.SizeMin + (u32)(rng.NextU64() % (def.SizeMax - def.SizeMin + 1));
            Array<u32> frontier = {center};
            u32 placed = 0;
            u32 richness = 1 + (u32)(rng.NextU64() % 3);
            u32 amountBase = 600000u + (u32)(rng.NextU64() % 900000u);
            while (placed < blob && !frontier.IsEmpty())
            {
                u32 idx = (u32)(rng.NextU64() % frontier.Count());
                u32 k = frontier[idx];
                frontier[idx] = frontier.Last();
                frontier.RemoveLast();
                Tile& t = GetTile(k);
                if (t.Resource != 0 || !TerrainBuildable((Terrain)t.Terrain)) continue;
                if (def.Resource == 7 && TerrainWater((Terrain)t.Terrain))
                {
                    // 原油在浅海也可布置, 但深海不行
                    if (t.Terrain == (u8)Terrain::DeepOcean) continue;
                }
                t.Resource = (u8)def.Resource;
                t.Richness = (u8)richness;
                t.ResourceAmount = amountBase * richness;
                placed++;
                for (int d = 0; d < 4; d++)
                {
                    u32 nb = NeighborKey(k, d);
                    if (nb != k) frontier.Add(nb);
                }
            }
        }
    }
}

} // namespace DSP
