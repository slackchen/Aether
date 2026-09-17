#include "demo/dyson_sphere/terrain.h"
#include <cmath>
#include <algorithm>

namespace dsp {

static constexpr f32 kPi = 3.14159265358979323846f;

// ---------------------------------------------------------------------------
// 噪声基元 (2D 用于平面系统, 3D 用于球面转换)
// ---------------------------------------------------------------------------
static f32 hash2(f32 x, f32 y) {
    f32 n = sinf(x * 127.1f + y * 311.7f) * 43758.5453123f;
    return n - floorf(n);
}

static f32 vnoise2(f32 x, f32 y) {
    f32 ix = floorf(x), fx = x - ix;
    f32 iy = floorf(y), fy = y - iy;
    f32 wx = fx * fx * (3.0f - 2.0f * fx);
    f32 wy = fy * fy * (3.0f - 2.0f * fy);
    f32 c00 = hash2(ix, iy), c10 = hash2(ix + 1, iy);
    f32 c01 = hash2(ix, iy + 1), c11 = hash2(ix + 1, iy + 1);
    f32 x0 = c00 * (1 - wx) + c10 * wx;
    f32 x1 = c01 * (1 - wx) + c11 * wx;
    return x0 * (1 - wy) + x1 * wy;
}

static f32 fbm2(f32 x, f32 y, int octaves) {
    f32 sum = 0.0f, freq = 1.0f, amp = 0.5f, norm = 0.0f;
    for (int i = 0; i < octaves; i++) {
        sum += vnoise2(x * freq, y * freq) * amp;
        norm += amp;
        freq *= 2.03f;
        amp *= 0.5f;
    }
    return sum / norm;
}

// 山脊噪声: |noise*2-1| 反转, 产生尖锐山脊线
static f32 ridged2(f32 x, f32 y, int octaves) {
    f32 sum = 0.0f, freq = 1.0f, amp = 0.55f, norm = 0.0f;
    for (int i = 0; i < octaves; i++) {
        f32 n = 1.0f - fabsf(vnoise2(x * freq, y * freq) * 2.0f - 1.0f);
        sum += n * n * amp;
        norm += amp;
        freq *= 2.11f;
        amp *= 0.5f;
    }
    return sum / norm;
}

static f32 hash3(f32 x, f32 y, f32 z) {
    f32 n = sinf(x * 127.1f + y * 311.7f + z * 74.7f) * 43758.5453123f;
    return n - floorf(n);
}

static f32 vnoise3(f32 x, f32 y, f32 z) {
    f32 ix = floorf(x), fx = x - ix;
    f32 iy = floorf(y), fy = y - iy;
    f32 iz = floorf(z), fz = z - iz;
    f32 wx = fx * fx * (3.0f - 2.0f * fx);
    f32 wy = fy * fy * (3.0f - 2.0f * fy);
    f32 wz = fz * fz * (3.0f - 2.0f * fz);
    f32 c000 = hash3(ix, iy, iz), c100 = hash3(ix + 1, iy, iz);
    f32 c010 = hash3(ix, iy + 1, iz), c110 = hash3(ix + 1, iy + 1, iz);
    f32 c001 = hash3(ix, iy, iz + 1), c101 = hash3(ix + 1, iy, iz + 1);
    f32 c011 = hash3(ix, iy + 1, iz + 1), c111 = hash3(ix + 1, iy + 1, iz + 1);
    f32 x00 = c000 * (1 - wx) + c100 * wx, x10 = c010 * (1 - wx) + c110 * wx;
    f32 x01 = c001 * (1 - wx) + c101 * wx, x11 = c011 * (1 - wx) + c111 * wx;
    f32 y0 = x00 * (1 - wy) + x10 * wy, y1 = x01 * (1 - wy) + x11 * wy;
    return y0 * (1 - wz) + y1 * wz;
}

static f32 fbm3(f32 x, f32 y, f32 z, int octaves) {
    f32 sum = 0.0f, freq = 1.0f, amp = 0.5f, norm = 0.0f;
    for (int i = 0; i < octaves; i++) {
        sum += vnoise3(x * freq, y * freq, z * freq) * amp;
        norm += amp;
        freq *= 2.03f;
        amp *= 0.5f;
    }
    return sum / norm;
}

static f32 ridged3(f32 x, f32 y, f32 z, int octaves) {
    f32 sum = 0.0f, freq = 1.0f, amp = 0.55f, norm = 0.0f;
    for (int i = 0; i < octaves; i++) {
        f32 n = 1.0f - fabsf(vnoise3(x * freq, y * freq, z * freq) * 2.0f - 1.0f);
        sum += n * n * amp;
        norm += amp;
        freq *= 2.11f;
        amp *= 0.5f;
    }
    return sum / norm;
}

static f32 smoothstep_f(f32 a, f32 b, f32 x) {
    f32 t = std::clamp((x - a) / (b - a), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// ---------------------------------------------------------------------------
// 平面地形系统 (米): 高度 = 大陆基面 + 山脉 + 丘陵
// ---------------------------------------------------------------------------
f32 TerrainField::height(f32 x, f32 z) const {
    f32 so = (f32)seed_ * 0.137f;

    // 域扭曲, 让大陆轮廓更自然
    f32 wx = (fbm2(x * 0.00055f + so, z * 0.00055f, 3) - 0.5f) * 2.0f;
    f32 wz = (fbm2(x * 0.00055f + so * 1.7f + 5.0f, z * 0.00055f + 9.0f, 3) - 0.5f) * 2.0f;
    f32 px = x + wx * 260.0f;
    f32 pz = z + wz * 260.0f;

    f32 base = fbm2(px * 0.00105f, pz * 0.00105f, 5);              // 0..1 大陆
    f32 mmask = smoothstep_f(0.50f, 0.62f, base);                  // 山脉集中在内陆
    f32 ridge = ridged2(px * 0.0042f + 3.7f, pz * 0.0042f, 5);     // 0..1 山脊
    f32 hills = fbm2(px * 0.021f, pz * 0.021f, 3);                 // 0..1 丘陵

    f32 h = (base - 0.5f) * 2.0f * 95.0f;                          // 大陆起伏 ±95
    h += mmask * powf(ridge, 1.7f) * 620.0f;                       // 山脉可达 ~700
    h += (hills - 0.5f) * 2.0f * 14.0f;                            // 细节起伏 ±14
    return h;
}

Vec3 TerrainField::planar_normal(f32 x, f32 z, f32 eps) const {
    f32 hl = height(x - eps, z);
    f32 hr = height(x + eps, z);
    f32 hd = height(x, z - eps);
    f32 hu = height(x, z + eps);
    // 法线 = normalize(-dh/dx, 1, -dh/dz) (Y 上, XZ 地面)
    Vec3 n{-(hr - hl) / (2.0f * eps), 1.0f, -(hu - hd) / (2.0f * eps)};
    f32 len = sqrtf(n.x * n.x + n.y * n.y + n.z * n.z);
    return {n.x / len, n.y / len, n.z / len};
}

// 归一化: 海平面以下 → <0.5, 最高山 → ~1
f32 TerrainField::planar_normalized(f32 x, f32 z) const {
    f32 h = height(x, z);
    return std::clamp(0.5f + h / 780.0f, 0.0f, 1.0f);
}

// ---------------------------------------------------------------------------
// 球形转换: 立方体六面映射 (与 PlanetGrid kFaces 一致)
//   face 0: +X  u→+Y  v→+Z      face 1: -X  u→+Z  v→+Y
//   face 2: +Y  u→+Z  v→+X      face 3: -Y  u→+X  v→+Z
//   face 4: +Z  u→+X  v→+Y      face 5: -Z  u→+Y  v→+X
// ---------------------------------------------------------------------------
static const i8 kFaceU[6] = {1, 2, 2, 0, 0, 1};
static const i8 kFaceV[6] = {2, 1, 0, 2, 1, 0};
static const i8 kFaceS[6] = {1, -1, 1, -1, 1, -1};

Vec3 TerrainField::cube_face_point(i32 face, f32 u, f32 v) {
    f32 a = u * 2.0f - 1.0f;
    f32 b = v * 2.0f - 1.0f;
    Vec3 p{0.0f, 0.0f, 0.0f};
    i32 axis = face >> 1;
    if (axis == 0) p.x = (f32)kFaceS[face];
    else if (axis == 1) p.y = (f32)kFaceS[face];
    else p.z = (f32)kFaceS[face];
    if (kFaceU[face] == 0) p.x = a;
    else if (kFaceU[face] == 1) p.y = a;
    else p.z = a;
    if (kFaceV[face] == 0) p.x = b;
    else if (kFaceV[face] == 1) p.y = b;
    else p.z = b;
    f32 len = sqrtf(p.x * p.x + p.y * p.y + p.z * p.z);
    return {p.x / len, p.y / len, p.z / len};
}

void TerrainField::cube_face_basis(i32 face, Vec3& out_u, Vec3& out_v) {
    out_u = {0.0f, 0.0f, 0.0f};
    out_v = {0.0f, 0.0f, 0.0f};
    if (kFaceU[face] == 0) out_u.x = 1.0f;
    else if (kFaceU[face] == 1) out_u.y = 1.0f;
    else out_u.z = 1.0f;
    if (kFaceV[face] == 0) out_v.x = 1.0f;
    else if (kFaceV[face] == 1) out_v.y = 1.0f;
    else out_v.z = 1.0f;
}

void TerrainField::point_to_face(const Vec3& dir, i32& out_face, f32& out_u, f32& out_v) {
    f32 ax = fabsf(dir.x), ay = fabsf(dir.y), az = fabsf(dir.z);
    i32 face;
    if (ax >= ay && ax >= az) face = dir.x >= 0.0f ? 0 : 1;
    else if (ay >= az) face = dir.y >= 0.0f ? 2 : 3;
    else face = dir.z >= 0.0f ? 4 : 5;
    f32 comps[3] = {dir.x, dir.y, dir.z};
    out_face = face;
    out_u = comps[kFaceU[face]] * 0.5f + 0.5f;
    out_v = comps[kFaceV[face]] * 0.5f + 0.5f;
}

f32 TerrainField::sphere_height(const Vec3& unit_dir) const {
    f32 so = (f32)seed_ * 0.137f;
    f32 x = unit_dir.x + so;
    f32 y = unit_dir.y + so * 1.3f;
    f32 z = unit_dir.z + so * 0.7f;

    f32 base = fbm3(x * 2.3f, y * 2.3f, z * 2.3f, 5);              // 0..1 大陆
    f32 mmask = smoothstep_f(0.50f, 0.62f, base);
    f32 ridge = ridged3(x * 5.2f, y * 5.2f, z * 5.2f, 4);
    f32 hills = fbm3(x * 9.5f, y * 9.5f, z * 9.5f, 3);

    f32 h = (base - 0.5f) * 2.0f * kSphereAmp * 0.62f;             // 大陆 ±4
    h += mmask * powf(ridge, 1.7f) * kSphereAmp;                   // 山脉 ~6.5
    h += (hills - 0.5f) * 2.0f * kSphereAmp * 0.22f;               // 细节 ±1.4
    return h;
}

Vec3 TerrainField::sphere_normal(const Vec3& unit_dir, f32 radius, f32 eps) const {
    // 沿切平面两个方向做中心差分
    Vec3 ref = fabsf(unit_dir.y) > 0.92f ? Vec3{1.0f, 0.0f, 0.0f} : Vec3{0.0f, 1.0f, 0.0f};
    Vec3 t1 = {
        ref.y * unit_dir.z - ref.z * unit_dir.y,
        ref.z * unit_dir.x - ref.x * unit_dir.z,
        ref.x * unit_dir.y - ref.y * unit_dir.x
    };
    f32 l1 = sqrtf(t1.x * t1.x + t1.y * t1.y + t1.z * t1.z);
    t1 = {t1.x / l1, t1.y / l1, t1.z / l1};
    Vec3 t2 = {
        t1.y * unit_dir.z - t1.z * unit_dir.y,
        t1.z * unit_dir.x - t1.x * unit_dir.z,
        t1.x * unit_dir.y - t1.y * unit_dir.x
    };

    auto surf = [&](const Vec3& d) {
        f32 h = sphere_height(d);
        return Vec3{d.x * (radius + h), d.y * (radius + h), d.z * (radius + h)};
    };
    Vec3 px = surf({unit_dir.x + t1.x * eps, unit_dir.y + t1.y * eps, unit_dir.z + t1.z * eps});
    Vec3 mx = surf({unit_dir.x - t1.x * eps, unit_dir.y - t1.y * eps, unit_dir.z - t1.z * eps});
    Vec3 pz = surf({unit_dir.x + t2.x * eps, unit_dir.y + t2.y * eps, unit_dir.z + t2.z * eps});
    Vec3 mz = surf({unit_dir.x - t2.x * eps, unit_dir.y - t2.y * eps, unit_dir.z - t2.z * eps});
    Vec3 du = {px.x - mx.x, px.y - mx.y, px.z - mx.z};
    Vec3 dv = {pz.x - mz.x, pz.y - mz.y, pz.z - mz.z};
    Vec3 n = {
        du.y * dv.z - du.z * dv.y,
        du.z * dv.x - du.x * dv.z,
        du.x * dv.y - du.y * dv.x
    };
    // 保证朝外
    if (n.x * unit_dir.x + n.y * unit_dir.y + n.z * unit_dir.z < 0.0f) {
        n = {-n.x, -n.y, -n.z};
    }
    f32 len = sqrtf(n.x * n.x + n.y * n.y + n.z * n.z);
    if (len < 1e-8f) return unit_dir;
    return {n.x / len, n.y / len, n.z / len};
}

f32 TerrainField::sphere_normalized(const Vec3& unit_dir) const {
    return std::clamp(0.5f + sphere_height(unit_dir) / (kSphereAmp * 1.35f), 0.0f, 1.0f);
}

f32 TerrainField::detail3(const Vec3& unit_dir) const {
    f32 so = (f32)seed_ * 0.137f;
    return fbm3((unit_dir.x + so) * 13.0f, (unit_dir.y + so * 1.3f) * 13.0f,
                (unit_dir.z + so * 0.7f) * 13.0f, 3);
}

f32 TerrainField::detail2(f32 x, f32 z) const {
    f32 so = (f32)seed_ * 0.137f;
    return fbm2((x + so * 512.0f) * 0.031f, (z + so * 289.0f) * 0.031f, 3);
}

// ---------------------------------------------------------------------------
// 配色: 高度 + 坡度 → 地貌色 (深水→浅滩→沙滩→草地/森林→岩石→雪)
// ---------------------------------------------------------------------------
Color TerrainField::terrain_color(f32 h_norm, f32 slope, f32 detail) {
    auto lerp3 = [](f32 a, f32 b, f32 t) { return a + (b - a) * t; };
    struct RGB { f32 r, g, b; };
    auto mix = [&lerp3](const RGB& a, const RGB& b, f32 t) -> RGB {
        return {lerp3(a.r, b.r, t), lerp3(a.g, b.g, t), lerp3(a.b, b.b, t)};
    };

    const RGB deep_water{0.055f, 0.13f, 0.29f};
    const RGB shallow{0.11f, 0.32f, 0.45f};
    const RGB sand{0.72f, 0.64f, 0.42f};
    const RGB grass{0.22f, 0.44f, 0.18f};
    const RGB forest{0.10f, 0.28f, 0.12f};
    const RGB rock{0.38f, 0.34f, 0.30f};
    const RGB snow{0.88f, 0.91f, 0.94f};

    RGB c;
    if (h_norm < 0.40f) {
        c = deep_water;
    } else if (h_norm < 0.48f) {
        c = mix(deep_water, shallow, (h_norm - 0.40f) / 0.08f);
    } else if (h_norm < 0.505f) {
        c = mix(shallow, sand, (h_norm - 0.48f) / 0.025f);
    } else if (h_norm < 0.53f) {
        c = sand;
    } else if (h_norm < 0.68f) {
        f32 t = (h_norm - 0.53f) / 0.15f;
        RGB g = detail > 0.55f ? mix(grass, forest, smoothstep_f(0.55f, 0.75f, detail)) : grass;
        c = mix(sand, g, smoothstep_f(0.0f, 0.25f, t));
    } else if (h_norm < 0.80f) {
        f32 t = (h_norm - 0.68f) / 0.12f;
        c = mix(grass, rock, smoothstep_f(0.15f, 0.75f, t));
    } else {
        f32 t = (h_norm - 0.80f) / 0.20f;
        c = mix(rock, snow, smoothstep_f(0.25f, 0.85f, t));
    }

    // 陡坡露岩
    f32 rock_w = smoothstep_f(0.45f, 0.8f, slope);
    if (h_norm > 0.505f) {
        c = mix(c, rock, rock_w * 0.85f);
    }
    // 细节明度扰动
    f32 shade = 0.92f + detail * 0.16f;
    return Color{c.r * shade, c.g * shade, c.b * shade, 1.0f};
}

} // namespace dsp
