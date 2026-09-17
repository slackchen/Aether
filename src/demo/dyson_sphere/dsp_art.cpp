#include "demo/dyson_sphere/dsp_art.h"
#include "demo/dyson_sphere/planet_grid.h"
#include "engine/texture.h"
#include <cmath>
#include <vector>
#include <algorithm>

namespace dsp_art {

using namespace aether;

static constexpr f32 kPi = 3.14159265358979323846f;

// ---------------------------------------------------------------------------
// 极简像素画板
// ---------------------------------------------------------------------------
struct Img {
    u32 w = 0, h = 0;
    std::vector<u8> px;

    Img(u32 w_, u32 h_) : w(w_), h(h_), px(w_ * h_ * 4, 0) {}

    void blend(u32 x, u32 y, Color c) {
        if (x >= w || y >= h) return;
        u8* p = &px[(y * w + x) * 4];
        f32 a = std::clamp(c.a, 0.0f, 1.0f);
        p[0] = (u8)std::clamp(p[0] * (1 - a) + c.r * 255.0f * a, 0.0f, 255.0f);
        p[1] = (u8)std::clamp(p[1] * (1 - a) + c.g * 255.0f * a, 0.0f, 255.0f);
        p[2] = (u8)std::clamp(p[2] * (1 - a) + c.b * 255.0f * a, 0.0f, 255.0f);
        p[3] = (u8)std::clamp((f32)p[3] + a * 255.0f * (1.0f - (f32)p[3] / 255.0f), 0.0f, 255.0f);
    }
    void set(u32 x, u32 y, Color c) {
        if (x >= w || y >= h) return;
        u8* p = &px[(y * w + x) * 4];
        p[0] = (u8)std::clamp(c.r * 255.0f, 0.0f, 255.0f);
        p[1] = (u8)std::clamp(c.g * 255.0f, 0.0f, 255.0f);
        p[2] = (u8)std::clamp(c.b * 255.0f, 0.0f, 255.0f);
        p[3] = (u8)std::clamp(c.a * 255.0f, 0.0f, 255.0f);
    }
    void rect(f32 x, f32 y, f32 rw, f32 rh, Color c) {
        for (i32 yy = (i32)floorf(y); yy < (i32)ceilf(y + rh); yy++)
            for (i32 xx = (i32)floorf(x); xx < (i32)ceilf(x + rw); xx++) blend((u32)xx, (u32)yy, c);
    }
    void disc(f32 cx, f32 cy, f32 r, Color c) {
        i32 x0 = (i32)(cx - r - 1), x1 = (i32)(cx + r + 1);
        i32 y0 = (i32)(cy - r - 1), y1 = (i32)(cy + r + 1);
        for (i32 yy = y0; yy <= y1; yy++) {
            for (i32 xx = x0; xx <= x1; xx++) {
                f32 d = sqrtf((xx + 0.5f - cx) * (xx + 0.5f - cx) + (yy + 0.5f - cy) * (yy + 0.5f - cy));
                f32 a = std::clamp(r - d + 0.5f, 0.0f, 1.0f) * c.a;
                if (a > 0.001f) blend((u32)xx, (u32)yy, {c.r, c.g, c.b, a});
            }
        }
    }
    void ring(f32 cx, f32 cy, f32 r, f32 th, Color c) {
        i32 x0 = (i32)(cx - r - th - 1), x1 = (i32)(cx + r + th + 1);
        i32 y0 = (i32)(cy - r - th - 1), y1 = (i32)(cy + r + th + 1);
        for (i32 yy = y0; yy <= y1; yy++) {
            for (i32 xx = x0; xx <= x1; xx++) {
                f32 d = sqrtf((xx + 0.5f - cx) * (xx + 0.5f - cx) + (yy + 0.5f - cy) * (yy + 0.5f - cy));
                f32 a = std::clamp(th * 0.5f - fabsf(d - r) + 0.5f, 0.0f, 1.0f) * c.a;
                if (a > 0.001f) blend((u32)xx, (u32)yy, {c.r, c.g, c.b, a});
            }
        }
    }
    void line(f32 x0, f32 y0, f32 x1, f32 y1, f32 wd, Color c) {
        f32 minx = std::min(x0, x1) - wd - 1, maxx = std::max(x0, x1) + wd + 1;
        f32 miny = std::min(y0, y1) - wd - 1, maxy = std::max(y0, y1) + wd + 1;
        f32 dx = x1 - x0, dy = y1 - y0;
        f32 len2 = dx * dx + dy * dy;
        for (i32 yy = (i32)miny; yy <= (i32)maxy; yy++) {
            for (i32 xx = (i32)minx; xx <= (i32)maxx; xx++) {
                f32 px = xx + 0.5f, py = yy + 0.5f;
                f32 t = len2 > 0.0001f ? std::clamp(((px - x0) * dx + (py - y0) * dy) / len2, 0.0f, 1.0f) : 0.0f;
                f32 qx = x0 + dx * t, qy = y0 + dy * t;
                f32 d = sqrtf((px - qx) * (px - qx) + (py - qy) * (py - qy));
                f32 a = std::clamp(wd * 0.5f - d + 0.5f, 0.0f, 1.0f) * c.a;
                if (a > 0.001f) blend((u32)xx, (u32)yy, {c.r, c.g, c.b, a});
            }
        }
    }
    void rounded(f32 x, f32 y, f32 rw, f32 rh, f32 r, Color c) {
        for (i32 yy = (i32)y; yy < (i32)(y + rh); yy++) {
            for (i32 xx = (i32)x; xx < (i32)(x + rw); xx++) {
                f32 px = xx + 0.5f, py = yy + 0.5f;
                f32 qx = std::clamp(px, x + r, x + rw - r);
                f32 qy = std::clamp(py, y + r, y + rh - r);
                f32 d = sqrtf((px - qx) * (px - qx) + (py - qy) * (py - qy));
                f32 a = std::clamp(r - d + 0.5f, 0.0f, 1.0f) * c.a;
                if (a > 0.001f) blend((u32)xx, (u32)yy, {c.r, c.g, c.b, a});
            }
        }
    }
    void vgrad(f32 x, f32 y, f32 rw, f32 rh, Color top, Color bottom) {
        for (i32 yy = (i32)y; yy < (i32)(y + rh); yy++) {
            f32 t = (rh > 0.0f) ? (yy - y) / rh : 0.0f;
            Color c{top.r * (1 - t) + bottom.r * t, top.g * (1 - t) + bottom.g * t,
                    top.b * (1 - t) + bottom.b * t, top.a * (1 - t) + bottom.a * t};
            for (i32 xx = (i32)x; xx < (i32)(x + rw); xx++) blend((u32)xx, (u32)yy, c);
        }
    }
};

// 可复现噪声
static f32 hash2(f32 x, f32 y, f32 s = 0.0f) {
    f32 n = sinf(x * 127.1f + y * 311.7f + s * 74.7f) * 43758.5453f;
    return n - floorf(n);
}
static f32 vnoise(f32 x, f32 y, f32 s = 0.0f) {
    f32 ix = floorf(x), iy = floorf(y);
    f32 fx = x - ix, fy = y - iy;
    f32 wx = fx * fx * (3 - 2 * fx), wy = fy * fy * (3 - 2 * fy);
    f32 a = hash2(ix, iy, s), b = hash2(ix + 1, iy, s);
    f32 c = hash2(ix, iy + 1, s), d = hash2(ix + 1, iy + 1, s);
    return (a * (1 - wx) + b * wx) * (1 - wy) + (c * (1 - wx) + d * wx) * wy;
}
static f32 fbm2(f32 x, f32 y, f32 s = 0.0f, int oct = 4) {
    f32 sum = 0, amp = 0.5f, fr = 1.0f;
    for (int i = 0; i < oct; i++) {
        sum += vnoise(x * fr, y * fr, s + i * 17.3f) * amp;
        fr *= 2.03f; amp *= 0.5f;
    }
    return sum;
}

static std::shared_ptr<rhi::RHITexture> upload(rhi::RHIDevice* dev, const Img& img) {
    return engine::make_texture(dev, img.w, img.h, [&](u32 x, u32 y, u8 out[4]) {
        const u8* p = &img.px[(y * img.w + x) * 4];
        out[0] = p[0]; out[1] = p[1]; out[2] = p[2]; out[3] = p[3];
    });
}

// ---------------------------------------------------------------------------
// 调色板
// ---------------------------------------------------------------------------
static const Color kHull{0.545f, 0.58f, 0.635f, 1.0f};     // 舰体灰
static const Color kHullDark{0.29f, 0.32f, 0.37f, 1.0f};
static const Color kHullLight{0.78f, 0.82f, 0.87f, 1.0f};
static const Color kCyan{0.20f, 0.88f, 1.0f, 1.0f};
static const Color kOrange{1.0f, 0.54f, 0.24f, 1.0f};
static const Color kGold{1.0f, 0.80f, 0.28f, 1.0f};
static const Color kPurple{0.78f, 0.40f, 1.0f, 1.0f};
static const Color kGreen{0.30f, 0.92f, 0.52f, 1.0f};
static const Color kSteel{0.36f, 0.40f, 0.46f, 1.0f};

static Color shade(Color c, f32 m) { return {c.r * m, c.g * m, c.b * m, c.a}; }

static f32 kHalfPi_f() { return kPi * 0.5f; }
static f32 kTwoPi_f() { return kPi * 2.0f; }

// ---------------------------------------------------------------------------
// 地形图集 4x4 @128
// ---------------------------------------------------------------------------
static void paint_terrain_cell(Img& img, u32 ox, u32 oy, int idx) {
    const u32 S = 128;
    auto cell = [&](f32 u, f32 v) -> f32 { return fbm2(u * 6.0f, v * 6.0f, (f32)idx * 7.7f, 4); };
    Color base, dark, light;
    switch (idx) {
        case 0: base = {0.035f, 0.11f, 0.27f, 1}; dark = {0.02f, 0.07f, 0.19f, 1}; light = {0.07f, 0.19f, 0.40f, 1}; break;   // 深海
        case 1: base = {0.10f, 0.38f, 0.55f, 1}; dark = {0.06f, 0.28f, 0.44f, 1}; light = {0.20f, 0.52f, 0.70f, 1}; break;    // 浅海
        case 2: base = {0.80f, 0.72f, 0.52f, 1}; dark = {0.66f, 0.58f, 0.40f, 1}; light = {0.90f, 0.83f, 0.64f, 1}; break;    // 沙滩
        case 3: base = {0.33f, 0.58f, 0.28f, 1}; dark = {0.24f, 0.46f, 0.20f, 1}; light = {0.44f, 0.70f, 0.36f, 1}; break;    // 草原
        case 4: base = {0.16f, 0.38f, 0.19f, 1}; dark = {0.10f, 0.27f, 0.13f, 1}; light = {0.26f, 0.50f, 0.27f, 1}; break;    // 森林
        case 5: base = {0.47f, 0.44f, 0.40f, 1}; dark = {0.34f, 0.32f, 0.29f, 1}; light = {0.60f, 0.57f, 0.52f, 1}; break;    // 岩石
        case 6: base = {0.90f, 0.93f, 0.96f, 1}; dark = {0.78f, 0.83f, 0.90f, 1}; light = {1.0f, 1.0f, 1.0f, 1}; break;       // 雪
        case 7: base = {0.83f, 0.68f, 0.40f, 1}; dark = {0.70f, 0.55f, 0.30f, 1}; light = {0.92f, 0.79f, 0.52f, 1}; break;    // 沙漠
        case 8: base = {0.78f, 0.88f, 0.95f, 1}; dark = {0.62f, 0.76f, 0.88f, 1}; light = {0.92f, 0.97f, 1.0f, 1}; break;     // 冰原
        case 9: base = {0.55f, 0.58f, 0.50f, 1}; dark = {0.42f, 0.45f, 0.38f, 1}; light = {0.68f, 0.71f, 0.62f, 1}; break;    // 冻土
        case 10: base = {0.20f, 0.18f, 0.22f, 1}; dark = {0.12f, 0.11f, 0.14f, 1}; light = {0.32f, 0.29f, 0.34f, 1}; break;   // 玄武岩
        default: base = {0.22f, 0.13f, 0.10f, 1}; dark = {0.30f, 0.10f, 0.05f, 1}; light = {1.0f, 0.55f, 0.15f, 1}; break;    // 熔岩
    }

    for (u32 y = 0; y < S; y++) {
        for (u32 x = 0; x < S; x++) {
            f32 u = (f32)x / S, v = (f32)y / S;
            f32 n = cell(u, v);
            f32 n2 = fbm2(u * 18.0f, v * 18.0f, (f32)idx * 3.1f, 3);
            Color c = {base.r * (1 - n * 0.5f) + light.r * n * 0.5f,
                       base.g * (1 - n * 0.5f) + light.g * n * 0.5f,
                       base.b * (1 - n * 0.5f) + light.b * n * 0.5f, 1.0f};
            c = shade(c, 0.92f + n2 * 0.16f);

            if (idx == 0 || idx == 1) { // 海浪条纹
                f32 wv = sinf((u * 9.0f + v * 3.0f) * 6.2831f + n * 7.0f);
                if (wv > 0.86f) c = shade(light, 1.15f);
            } else if (idx == 4) { // 森林树冠
                f32 tr = fbm2(u * 26.0f, v * 26.0f, 55.0f, 2);
                if (tr > 0.60f) c = shade(light, 0.85f + tr * 0.3f);
            } else if (idx == 5) { // 岩石裂纹
                f32 cr = fbm2(u * 12.0f, v * 12.0f, 91.0f, 3);
                if (fabsf(cr - 0.5f) < 0.02f) c = shade(dark, 0.8f);
            } else if (idx == 6) { // 雪地亮斑
                if (hash2(x, y, 5.0f) > 0.985f) c = {1, 1, 1, 1};
            } else if (idx == 8) { // 冰面裂纹
                f32 cr = fbm2(u * 10.0f, v * 10.0f, 33.0f, 3);
                if (fabsf(cr - 0.5f) < 0.018f) c = {0.55f, 0.72f, 0.86f, 1};
            } else if (idx == 11) { // 熔岩裂纹
                f32 cr = fbm2(u * 8.0f, v * 8.0f, 77.0f, 3);
                f32 vein = 1.0f - std::clamp(fabsf(cr - 0.5f) * 22.0f, 0.0f, 1.0f);
                c = {c.r + vein * (light.r - c.r), c.g + vein * (light.g - c.g), c.b + vein * (light.b - c.b), 1};
            } else if (idx == 7) { // 沙丘
                f32 dune = sinf(u * 14.0f + fbm2(u * 4, v * 4, 12.0f, 3) * 9.0f);
                c = shade(c, 1.0f + dune * 0.07f);
            }
            img.set(ox + x, oy + y, c);
        }
    }
}

// ---------------------------------------------------------------------------
// 建筑图集 6x6 @128 — 全部俯视图, 朝向 +X
// ---------------------------------------------------------------------------
static void paint_building_cell(Img& img, u32 ox, u32 oy, int kind) {
    const f32 C = 64.0f; // 中心
    auto shadow = [&]() { img.disc(C + 4, C + 6, 44, {0, 0, 0, 0.35f}); };
    auto stripes = [&](f32 x, f32 y, f32 w, f32 h) {
        for (f32 i = -h; i < w; i += 10.0f) {
            Color sc = (fmodf(i, 20.0f) < 10.0f) ? kGold : kHullDark;
            for (f32 t = 0; t < h; t += 0.5f)
                for (f32 s = 0; s < 5.0f; s += 0.5f)
                    img.blend((u32)(x + i + t + s), (u32)(y + t), {sc.r, sc.g, sc.b, sc.a});
        }
    };

    switch (kind) {
        case 1: { // 传送带
            img.rect(ox + 8, oy + 44, 112, 40, {0, 0, 0, 0});
            img.rounded(ox + 6, oy + 44, 116, 40, 6, shade(kHullDark, 0.8f));
            img.rounded(ox + 10, oy + 48, 108, 32, 4, shade(kHull, 0.9f));
            for (int i = 0; i < 9; i++) img.rect(ox + 14 + i * 12, oy + 50, 6, 28, shade(kHullDark, 1.1f));
            img.rect(ox + 10, oy + 44, 108, 5, kGold);   // 危险边条
            img.rect(ox + 10, oy + 79, 108, 5, kGold);
            img.line(ox + 46, oy + 64, ox + 74, oy + 64, 6, kCyan); // 流向箭头
            img.line(ox + 66, oy + 54, ox + 78, oy + 64, 4, kCyan);
            img.line(ox + 66, oy + 74, ox + 78, oy + 64, 4, kCyan);
            break;
        }
        case 2: { // 分拣器
            img.disc(C + 2, C + 2, 22, {0, 0, 0, 0.35f});
            img.disc(C, C, 20, kHullDark);
            img.disc(C, C, 15, kHull);
            img.disc(C, C, 7, shade(kSteel, 1.2f));
            img.line(ox + 70, oy + 64, ox + 112, oy + 64, 10, shade(kHull, 1.05f));
            img.line(ox + 108, oy + 58, ox + 118, oy + 64, 6, kHullDark);
            img.line(ox + 108, oy + 70, ox + 118, oy + 64, 6, kHullDark);
            img.disc(C, C, 4, kCyan);
            break;
        }
        case 3: { // 采矿机
            shadow();
            img.ring(C, C, 40, 10, kHullDark);
            img.ring(C, C, 34, 8, kHull);
            stripes(ox + 18, oy + 100, 92, 8);
            img.disc(C, C, 24, shade(kSteel, 1.1f));
            for (int i = 0; i < 8; i++) {
                f32 a = i * kPi / 4.0f;
                img.line(C + cosf(a) * 8, C + sinf(a) * 8, C + cosf(a) * 22, C + sinf(a) * 22, 5, kHullDark);
            }
            img.disc(C, C, 9, kOrange);
            img.disc(C, C, 5, {1.0f, 0.85f, 0.5f, 1});
            break;
        }
        case 4: { // 抽油机
            shadow();
            img.disc(C, C, 34, kHullDark);
            img.disc(C, C, 30, kHull);
            img.disc(C, C, 18, shade(kSteel, 0.9f));
            img.line(C - 26, C + 14, C + 26, C - 14, 7, kOrange); // 磕头梁
            img.disc(C, C, 6, kHullDark);
            img.disc(C + 26, C - 14, 7, kHullDark);
            img.disc(C + 26, C - 14, 4, {0.15f, 0.1f, 0.2f, 1});
            img.ring(C, C, 12, 3, kCyan);
            break;
        }
        case 5: { // 弧光熔炉
            shadow();
            img.rounded(ox + 18, oy + 22, 92, 84, 10, kHullDark);
            img.rounded(ox + 24, oy + 28, 80, 72, 8, kHull);
            img.rounded(ox + 30, oy + 34, 68, 30, 6, shade(kSteel, 1.15f));
            img.disc(C, oy + 84, 16, {0.9f, 0.35f, 0.08f, 1});   // 出料口熔光
            img.disc(C, oy + 84, 10, {1.0f, 0.7f, 0.2f, 1});
            img.rect(ox + 30, oy + 70, 68, 6, kHullDark);
            img.disc(ox + 34, oy + 40, 5, kCyan);
            img.disc(ox + 94, oy + 40, 5, kCyan);
            break;
        }
        case 6: { // 制造台
            shadow();
            img.rounded(ox + 16, oy + 26, 96, 76, 8, kHullDark);
            img.rounded(ox + 22, oy + 32, 84, 64, 6, kHull);
            img.rounded(ox + 40, oy + 48, 48, 32, 4, shade(kSteel, 1.2f));
            img.line(ox + 46, oy + 40, ox + 40, oy + 70, 8, kHullDark); // 机械臂
            img.disc(ox + 40, oy + 72, 7, kHullDark);
            img.disc(ox + 40, oy + 72, 4, kCyan);
            img.line(ox + 82, oy + 40, ox + 88, oy + 70, 8, kHullDark);
            img.disc(ox + 88, oy + 72, 7, kHullDark);
            img.disc(ox + 88, oy + 72, 4, kOrange);
            img.disc(C, C, 9, shade(kSteel, 1.4f));
            for (int i = 0; i < 6; i++) {
                f32 a = i * kPi / 3.0f;
                img.line(C + cosf(a) * 3, C + sinf(a) * 3, C + cosf(a) * 8, C + sinf(a) * 8, 2, kHullDark);
            }
            break;
        }
        case 7: { // 化工厂
            shadow();
            img.ring(C - 14, C + 8, 20, 8, kHull);
            img.ring(C + 18, C + 8, 20, 8, kHull);
            img.disc(C - 14, C + 8, 14, shade(kSteel, 0.95f));
            img.disc(C + 18, C + 8, 14, shade(kSteel, 0.95f));
            img.rect(ox + 20, oy + 88, 88, 10, kHullDark);
            img.line(ox + 50, oy + 30, ox + 50, oy + 56, 6, kHullDark);
            img.disc(ox + 50, oy + 26, 8, kHullDark);
            img.disc(ox + 50, oy + 26, 5, kGreen);
            img.ring(C - 14, C + 8, 8, 2, kCyan);
            break;
        }
        case 8: { // 矩阵实验室
            shadow();
            img.ring(C, C, 38, 10, kHullDark);
            img.ring(C, C, 30, 8, kHull);
            img.disc(C, C, 22, shade(kSteel, 1.1f));
            img.disc(C, C, 15, {0.1f, 0.5f, 0.9f, 1});
            img.disc(C, C, 9, {0.5f, 0.9f, 1.0f, 1});
            img.ring(C, C, 26, 2.5f, kCyan);
            for (int i = 0; i < 4; i++) {
                f32 a = i * kHalfPi_f();
                img.line(C + cosf(a) * 30, C + sinf(a) * 30, C + cosf(a) * 38, C + sinf(a) * 38, 5, kHullDark);
            }
            break;
        }
        case 9: case 10: { // PLS / ILS
            shadow();
            Color ac = (kind == 9) ? kGold : kPurple;
            img.ring(C, C, 44, 8, kHullDark);
            img.ring(C, C, 36, 6, kHull);
            img.ring(C, C, 24, 4, shade(ac, 0.9f));
            img.disc(C, C, 16, shade(kSteel, 1.1f));
            img.disc(C, C, 10, shade(ac, 0.7f));
            img.disc(C, C, 5, {1, 1, 1, 1});
            for (int i = 0; i < 4; i++) {
                f32 a = i * kHalfPi_f() + kPi * 0.25f;
                f32 px = C + cosf(a) * 34, py = C + sinf(a) * 34;
                img.rounded(px - 8, py - 8, 16, 16, 3, kHullDark);
                img.rounded(px - 5, py - 5, 10, 10, 2, ac);
            }
            break;
        }
        case 11: { // 电磁弹射器
            shadow();
            img.rounded(ox + 20, oy + 30, 88, 68, 8, kHullDark);
            img.rounded(ox + 26, oy + 36, 76, 56, 6, kHull);
            img.line(ox + 30, oy + 88, ox + 104, oy + 40, 10, shade(kSteel, 1.3f)); // 轨道
            for (int i = 0; i < 4; i++) {
                f32 t = i / 3.0f;
                f32 px = ox + 30 + 74 * t, py = oy + 88 - 48 * t;
                img.ring(px, py, 8, 3, kCyan);
            }
            img.disc(ox + 96, oy + 44, 6, {1, 0.9f, 0.5f, 1});
            break;
        }
        case 12: { // 垂直发射井
            shadow();
            img.ring(C, C, 42, 10, kHullDark);
            img.ring(C, C, 34, 7, kHull);
            img.disc(C, C, 24, shade(kSteel, 0.9f));
            img.disc(C, C, 15, {0.05f, 0.05f, 0.08f, 1});
            img.disc(C, C, 9, kOrange);
            img.rect(ox + 90, oy + 40, 14, 48, kHullDark); // 发射塔
            img.rect(ox + 92, oy + 42, 4, 44, kCyan);
            break;
        }
        case 13: { // 射线接收站
            shadow();
            img.ring(C, C + 10, 34, 9, kHullDark);
            img.disc(C, C + 10, 28, shade(kSteel, 1.0f));
            img.disc(C, C + 10, 20, {0.15f, 0.25f, 0.5f, 1});
            img.disc(C, C + 10, 12, kPurple);
            img.line(C, C + 10, C, C - 34, 6, kHullDark);
            img.disc(C, C - 36, 7, kPurple);
            img.disc(C, C - 36, 4, {1, 1, 1, 1});
            break;
        }
        case 14: { // 人造恒星
            img.disc(C, C, 30, {0.5f, 0.1f, 0.9f, 0.35f});
            img.disc(C, C, 20, {0.9f, 0.5f, 1.0f, 1});
            img.disc(C, C, 12, {1.0f, 0.95f, 1.0f, 1});
            img.ring(C, C, 26, 3, kCyan);
            img.ring(C, C, 34, 2, {0.5f, 0.8f, 1.0f, 0.7f});
            for (int i = 0; i < 6; i++) {
                f32 a = i * kPi / 3.0f;
                img.line(C + cosf(a) * 34, C + sinf(a) * 34, C + cosf(a) * 42, C + sinf(a) * 42, 4, kHullDark);
            }
            break;
        }
        case 15: { // 电塔
            img.disc(C, C, 18, kHullDark);
            img.disc(C, C, 13, kHull);
            img.disc(C, C, 6, kSteel);
            img.line(C - 14, C + 14, C + 14, C - 14, 4, kHullLight);
            img.line(C - 14, C - 14, C + 14, C + 14, 4, kHullLight);
            img.disc(C, C, 3, kCyan);
            break;
        }
        case 16: { // 无线充能塔
            img.disc(C, C, 22, kHullDark);
            img.disc(C, C, 17, kHull);
            img.ring(C, C, 11, 3, kCyan);
            img.ring(C, C, 6, 3, kGold);
            img.disc(C, C, 2.5f, {1, 1, 1, 1});
            break;
        }
        case 17: { // 风力涡轮
            img.disc(C, C, 20, kHullDark);
            img.disc(C, C, 15, kHull);
            for (int i = 0; i < 3; i++) {
                f32 a = i * kTwoPi_f() / 3.0f;
                img.line(C, C, C + cosf(a) * 42, C + sinf(a) * 42, 7, kHullLight);
                img.disc(C + cosf(a) * 42, C + sinf(a) * 42, 4, kCyan);
            }
            img.disc(C, C, 7, kHullDark);
            img.disc(C, C, 3, kCyan);
            break;
        }
        case 18: { // 太阳能板
            img.rounded(ox + 12, oy + 30, 104, 68, 4, kHullDark);
            img.rounded(ox + 16, oy + 34, 96, 60, 3, {0.12f, 0.32f, 0.62f, 1});
            for (int i = 1; i < 4; i++) img.rect(ox + 16 + i * 24, oy + 34, 2, 60, kHullDark);
            for (int i = 1; i < 3; i++) img.rect(ox + 16, oy + 34 + i * 20, 96, 2, kHullDark);
            img.line(ox + 20, oy + 40, ox + 40, oy + 40, 2, {0.5f, 0.8f, 1.0f, 0.8f});
            img.rounded(ox + 56, oy + 56, 16, 16, 2, kHull);
            break;
        }
        case 19: { // 火力发电
            shadow();
            img.rounded(ox + 16, oy + 30, 96, 68, 6, kHullDark);
            img.rounded(ox + 22, oy + 36, 84, 56, 5, kHull);
            img.rect(ox + 30, oy + 20, 16, 24, kHullDark);
            img.rect(ox + 58, oy + 20, 16, 24, kHullDark);
            img.rect(ox + 86, oy + 20, 16, 24, kHullDark);
            img.rect(ox + 32, oy + 22, 12, 6, shade(kSteel, 1.3f));
            img.rect(ox + 60, oy + 22, 12, 6, shade(kSteel, 1.3f));
            img.rect(ox + 88, oy + 22, 12, 6, shade(kSteel, 1.3f));
            img.rect(ox + 28, oy + 70, 72, 10, kOrange);
            break;
        }
        default: { // 聚变电站 (20)
            shadow();
            img.ring(C, C, 36, 10, kHullDark);
            img.ring(C, C, 28, 7, kHull);
            img.disc(C, C, 18, {0.1f, 0.15f, 0.25f, 1});
            img.ring(C, C, 12, 5, kCyan);
            img.disc(C, C, 6, {1, 1, 1, 1});
            for (int i = 0; i < 8; i++) {
                f32 a = i * kPi / 4.0f;
                img.line(C + cosf(a) * 36, C + sinf(a) * 36, C + cosf(a) * 44, C + sinf(a) * 44, 4, kHullDark);
            }
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// 物品图标 8x8 @32
// ---------------------------------------------------------------------------
static void paint_item_cell(Img& img, u32 ox, u32 oy, int kind) {
    const f32 C = 16.0f;
    // 常用形状
    auto ore_chunk = [&](Color c) {
        img.disc(C - 4, C + 3, 6, shade(c, 0.7f));
        img.disc(C + 5, C + 4, 5, shade(c, 0.8f));
        img.disc(C, C - 3, 7, c);
        img.disc(C - 2, C - 5, 2.5f, shade(c, 1.35f));
        img.disc(C + 3, C + 1, 1.8f, shade(c, 1.3f));
    };
    auto ingot = [&](Color c) {
        img.rounded(ox + 5, oy + 12, 22, 9, 3, shade(c, 0.75f));
        img.rounded(ox + 7, oy + 10, 18, 8, 2.5f, c);
        img.rounded(ox + 9, oy + 12, 10, 3, 1.5f, shade(c, 1.4f));
    };
    auto cube = [&](Color c) {
        img.rounded(ox + 7, oy + 7, 18, 18, 4, shade(c, 0.65f));
        img.rounded(ox + 9, oy + 9, 14, 14, 3, c);
        img.disc(C - 2, C - 2, 3, shade(c, 1.5f));
    };

    switch (kind) {
        case 1: ore_chunk({0.55f, 0.60f, 0.68f, 1}); break;          // 铁矿
        case 2: ore_chunk({0.88f, 0.55f, 0.30f, 1}); break;          // 铜矿
        case 3: for (int i = 0; i < 4; i++)                           // 煤
                img.disc(C - 6 + i * 4, C + (i % 2) * 5 - 2, 4.5f, {0.13f, 0.13f, 0.15f, 1});
                img.disc(C - 1, C - 4, 2, {0.35f, 0.35f, 0.4f, 1}); break;
        case 4: ore_chunk({0.72f, 0.72f, 0.70f, 1}); break;          // 石材
        case 5: ore_chunk({0.80f, 0.85f, 0.95f, 1}); break;          // 钛矿
        case 6: ore_chunk({0.45f, 0.78f, 0.85f, 1}); break;          // 硅矿
        case 7: img.disc(C, C + 2, 8, {0.10f, 0.08f, 0.13f, 1});     // 原油
                img.disc(C - 2, C, 3, {0.4f, 0.2f, 0.5f, 0.9f}); break;
        case 8: ingot({0.72f, 0.76f, 0.84f, 1}); break;              // 铁块
        case 9: ingot({0.92f, 0.58f, 0.30f, 1}); break;              // 铜块
        case 10: img.rounded(ox + 8, oy + 8, 16, 16, 7, kHull);      // 磁铁 (U形)
                 img.rounded(ox + 11, oy + 11, 10, 10, 4, {0.1f, 0.1f, 0.15f, 1});
                 img.rect(ox + 8, oy + 20, 6, 4, {0.9f, 0.3f, 0.3f, 1});
                 img.rect(ox + 18, oy + 20, 6, 4, {0.3f, 0.4f, 0.95f, 1}); break;
        case 11: for (int i = 0; i < 4; i++)                          // 磁线圈
                 img.ring(C, C, 3 + i * 2.6f, 2, (i % 2) ? kOrange : Color{0.9f, 0.65f, 0.4f, 1.0f});
                 img.disc(C, C, 2, kCyan); break;
        case 12: img.rounded(ox + 6, oy + 8, 20, 16, 2, {0.10f, 0.35f, 0.18f, 1}); // 电路板
                 img.rounded(ox + 8, oy + 10, 16, 12, 1, {0.14f, 0.5f, 0.25f, 1});
                 img.rect(ox + 12, oy + 12, 8, 8, kGold);
                 img.line(ox + 8, oy + 16, ox + 12, oy + 16, 1, kGold);
                 img.line(ox + 20, oy + 12, ox + 24, oy + 10, 1, kGold); break;
        case 13: img.disc(C, C, 9, shade(kSteel, 1.2f));              // 齿轮
                 img.disc(C, C, 4, {0.1f, 0.1f, 0.12f, 1});
                 for (int i = 0; i < 8; i++) {
                     f32 a = i * kPi / 4.0f;
                     img.disc(C + cosf(a) * 10, C + sinf(a) * 10, 2.5f, shade(kSteel, 1.2f));
                 } break;
        case 14: ingot({0.55f, 0.58f, 0.64f, 1}); break;              // 钢材
        case 15: ingot({0.85f, 0.90f, 1.0f, 1}); break;               // 钛块
        case 16: ingot({0.50f, 0.85f, 0.92f, 1}); break;              // 高纯硅
        case 17: img.rounded(ox + 6, oy + 8, 20, 16, 2, {0.08f, 0.08f, 0.12f, 1}); // 处理器
                 img.rounded(ox + 9, oy + 11, 14, 10, 1, kCyan);
                 for (int i = 0; i < 5; i++) {
                     img.rect(ox + 7 + i * 4, oy + 5, 2, 3, kGold);
                     img.rect(ox + 7 + i * 4, oy + 24, 2, 3, kGold);
                 } break;
        case 18: case 19: ingot({0.90f, 0.75f, 0.25f, 1}); break;     // 电机/推进器
        case 20: ingot({0.95f, 0.95f, 0.90f, 1}); break;              // 塑料
        case 21: img.disc(C, C, 9, {0.75f, 0.85f, 0.15f, 1});          // 硫酸
                 img.disc(C, C - 2, 5, {0.9f, 0.95f, 0.3f, 1}); break;
        case 22: img.rounded(ox + 10, oy + 6, 12, 20, 5, {0.85f, 0.35f, 0.45f, 0.9f}); // 氢
                 img.rounded(ox + 12, oy + 9, 8, 12, 3, {1.0f, 0.6f, 0.65f, 0.9f}); break;
        case 23: img.rounded(ox + 10, oy + 6, 12, 20, 5, {0.25f, 0.85f, 0.65f, 0.9f}); // 氘
                 img.rounded(ox + 12, oy + 9, 8, 12, 3, {0.6f, 1.0f, 0.85f, 0.9f}); break;
        case 24: cube({1.0f, 0.4f, 0.9f, 1}); break;                  // 反物质
        case 25: { // 太阳帆
            img.line(ox + 8, oy + 24, ox + 24, oy + 24, 2, kHullDark);
            img.line(ox + 16, oy + 24, ox + 16, oy + 8, 2, kHullDark);
            img.rounded(ox + 10, oy + 6, 18, 14, 2, kGold);
            img.rounded(ox + 12, oy + 8, 14, 10, 1, {1.0f, 0.92f, 0.55f, 1});
        } break;
        case 26: { // 运载火箭
            img.rounded(ox + 13, oy + 8, 6, 14, 3, kHullLight);
            img.disc(ox + 16, oy + 8, 3, kOrange);
            img.line(ox + 13, oy + 22, ox + 10, oy + 27, 2, kHullDark);
            img.line(ox + 19, oy + 22, ox + 22, oy + 27, 2, kHullDark);
            img.disc(ox + 16, oy + 14, 1.5f, kCyan);
        } break;
        case 27: cube({0.65f, 0.30f, 1.0f, 1}); break;                // 空间翘曲器
        case 28: cube({0.15f, 0.60f, 1.0f, 1}); break;                // 蓝矩阵
        case 29: cube({1.0f, 0.25f, 0.20f, 1}); break;                // 红矩阵
        case 30: cube({1.0f, 0.85f, 0.15f, 1}); break;                // 黄矩阵
        case 31: cube({0.85f, 0.25f, 1.0f, 1}); break;                // 紫矩阵
        case 32: cube({0.20f, 0.95f, 0.45f, 1}); break;               // 绿矩阵
        case 33: cube({0.98f, 0.98f, 1.0f, 1}); break;                // 白矩阵
        default: break;
    }
}

// ---------------------------------------------------------------------------
// 矿脉晶体 8x1 @64
// ---------------------------------------------------------------------------
static void paint_vein_cell(Img& img, u32 ox, u32 oy, int res) {
    Color c;
    switch (res) {
        case 1: c = {0.45f, 0.62f, 0.85f, 1}; break;   // 铁
        case 2: c = {0.92f, 0.55f, 0.25f, 1}; break;   // 铜
        case 3: c = {0.22f, 0.22f, 0.25f, 1}; break;   // 煤
        case 4: c = {0.75f, 0.75f, 0.72f, 1}; break;   // 石材
        case 5: c = {0.75f, 0.85f, 1.0f, 1}; break;    // 钛
        case 6: c = {0.40f, 0.85f, 0.90f, 1}; break;   // 硅
        default: c = {0.65f, 0.30f, 0.85f, 1}; break;  // 原油
    }
    // 岩基
    img.disc(32, 44, 20, {0.30f, 0.28f, 0.26f, 1});
    img.disc(24, 46, 14, {0.36f, 0.34f, 0.32f, 1});
    img.disc(42, 45, 13, {0.33f, 0.31f, 0.29f, 1});
    // 晶簇
    auto shard = [&](f32 x, f32 y, f32 hgt, f32 w, f32 tilt) {
        f32 x1 = x + tilt;
        img.line(x, y, x1, y - hgt, w, c);
        img.line(x1, y - hgt, x1, y - hgt, w * 0.4f, shade(c, 1.5f));
    };
    shard(20, 42, 18, 5, -4);
    shard(32, 40, 26, 6, 2);
    shard(44, 43, 15, 5, 6);
    shard(27, 44, 10, 4, 5);
    shard(38, 44, 12, 4, -3);
    img.disc(32, 16, 2.5f, {1, 1, 1, 0.9f});
    if (res == 7) { // 原油: 油池
        img.disc(32, 44, 15, {0.10f, 0.08f, 0.14f, 1});
        img.disc(26, 40, 4, {0.4f, 0.2f, 0.55f, 0.8f});
    }
}

// ---------------------------------------------------------------------------
// 机甲 128x128 (背视)
// ---------------------------------------------------------------------------
static void paint_mecha(Img& img) {
    // 腿
    img.rounded(46, 88, 14, 32, 5, shade(kHullDark, 0.9f));
    img.rounded(68, 88, 14, 32, 5, shade(kHullDark, 0.9f));
    img.rounded(44, 114, 18, 10, 4, kHullDark);
    img.rounded(66, 114, 18, 10, 4, kHullDark);
    // 躯干
    img.rounded(38, 40, 52, 54, 12, kHull);
    img.rounded(44, 46, 40, 30, 8, kHullLight);
    img.rounded(50, 50, 28, 18, 5, shade(kSteel, 1.1f));
    // 肩甲
    img.rounded(24, 42, 18, 26, 6, kHullDark);
    img.rounded(86, 42, 18, 26, 6, kHullDark);
    img.rounded(27, 45, 12, 20, 4, kHull);
    img.rounded(89, 45, 12, 20, 4, kHull);
    img.disc(33, 55, 4, kCyan);
    img.disc(95, 55, 4, kCyan);
    // 头部
    img.rounded(52, 22, 24, 20, 6, kHullLight);
    img.rounded(55, 27, 18, 9, 4, {0.08f, 0.12f, 0.18f, 1});
    img.rounded(57, 29, 14, 4, 2, kCyan);
    // 核心反应堆
    img.disc(64, 84, 10, {0.1f, 0.4f, 0.6f, 1});
    img.disc(64, 84, 6, kCyan);
    img.disc(64, 84, 3, {1, 1, 1, 1});
    // 背包推进器
    img.rounded(46, 30, 12, 14, 4, kHullDark);
    img.rounded(70, 30, 12, 14, 4, kHullDark);
    img.disc(52, 30, 3, kOrange);
    img.disc(76, 30, 3, kOrange);
    // 描边增强
    img.ring(64, 84, 12, 2, {0.2f, 0.9f, 1.0f, 0.55f});
}

// ---------------------------------------------------------------------------
// 特效 4x2 @64: 0太阳帆 1火箭 2无人机 3货船 4能量点 5火花
// ---------------------------------------------------------------------------
static void paint_fx_cell(Img& img, u32 ox, u32 oy, int idx) {
    const f32 C = 32.0f;
    switch (idx) {
        case 0: // 太阳帆
            img.line(C, 52, C, 44, 2.5f, kHullDark);
            img.rounded(ox + 12, oy + 12, 40, 30, 2, kGold);
            img.rounded(ox + 15, oy + 15, 34, 24, 1, {1.0f, 0.92f, 0.55f, 1});
            img.line(ox + 12, oy + 27, ox + 52, oy + 27, 1.5f, shade(kGold, 0.7f));
            break;
        case 1: // 火箭
            img.rounded(C - 6, 14, 12, 34, 5, kHullLight);
            img.disc(C, 14, 6, {0.95f, 0.45f, 0.2f, 1});
            img.line(C - 6, 44, C - 12, 56, 3, kHullDark);
            img.line(C + 6, 44, C + 12, 56, 3, kHullDark);
            img.disc(C, 30, 2.5f, kCyan);
            break;
        case 2: // 无人机
            img.rounded(C - 8, C - 5, 16, 10, 4, kHull);
            img.disc(C, C, 3, kGreen);
            img.disc(C - 10, C - 2, 2, kHullDark);
            img.disc(C + 10, C - 2, 2, kHullDark);
            break;
        case 3: // 货船
            img.rounded(C - 18, C - 6, 36, 12, 5, kHull);
            img.rounded(C + 8, C - 4, 12, 8, 3, kHullLight);
            img.disc(C + 17, C, 2, kGold);
            img.disc(C - 14, C, 2.5f, kCyan);
            break;
        case 4: // 能量点
            img.disc(C, C, 14, {1, 1, 1, 0.25f});
            img.disc(C, C, 8, {1, 1, 1, 0.7f});
            img.disc(C, C, 4, {1, 1, 1, 1});
            break;
        default: // 火花
            img.line(C - 12, C + 12, C + 12, C - 12, 3, {1, 0.9f, 0.6f, 0.9f});
            img.disc(C - 12, C + 12, 4, {1, 0.7f, 0.3f, 0.6f});
            break;
    }
}

// ---------------------------------------------------------------------------
// 星云 / 云层 / 光晕
// ---------------------------------------------------------------------------
static void paint_nebula(Img& img, f32 seed) {
    for (u32 y = 0; y < img.h; y++) {
        for (u32 x = 0; x < img.w; x++) {
            f32 u = (f32)x / img.w, v = (f32)y / img.h;
            // 环形距离衰减, 保证平铺不生硬
            f32 dx = u - 0.5f, dy = v - 0.5f;
            f32 fall = std::clamp(1.0f - sqrtf(dx * dx + dy * dy) * 2.2f, 0.0f, 1.0f);
            f32 n = fbm2(u * 3.5f, v * 3.5f, seed, 5);
            f32 n2 = fbm2(u * 9.0f, v * 9.0f, seed + 31.0f, 4);
            f32 a = std::clamp((n - 0.38f) * 2.2f, 0.0f, 1.0f) * fall * (0.55f + n2 * 0.45f);
            f32 d2 = std::clamp((n2 - 0.5f) * 2.0f, 0.0f, 1.0f);
            img.set(x, y, {0.75f + d2 * 0.25f, 0.7f, 1.0f - d2 * 0.3f, a * 0.85f});
        }
    }
}

static void paint_clouds(Img& img) {
    for (u32 y = 0; y < img.h; y++) {
        for (u32 x = 0; x < img.w; x++) {
            f32 u = (f32)x / img.w * 4.0f, v = (f32)y / img.h * 2.0f;
            f32 n = fbm2(u, v, 7.7f, 5);
            f32 wisp = fbm2(u * 2.5f + 5.0f, v * 1.2f, 19.0f, 4);
            f32 a = std::clamp((n * 0.7f + wisp * 0.3f - 0.52f) * 3.2f, 0.0f, 1.0f);
            f32 sh = 0.8f + (n - 0.5f) * 0.5f;
            img.set(x, y, {sh, sh, sh * 1.05f, a});
        }
    }
}

static void paint_flare(Img& img) {
    f32 cx = img.w * 0.5f, cy = img.h * 0.5f;
    for (u32 y = 0; y < img.h; y++) {
        for (u32 x = 0; x < img.w; x++) {
            f32 dx = fabsf((f32)x - cx) / cx;
            f32 dy = fabsf((f32)y - cy) / cy;
            f32 a = std::clamp(1.0f - dx, 0.0f, 1.0f);
            a *= a;
            a *= std::clamp(1.0f - dy * dy * 1.6f, 0.0f, 1.0f);
            a *= 0.8f + 0.2f * hash2((f32)x, (f32)y, 3.0f);
            img.set(x, y, {1.0f, 0.95f, 0.85f, a * 0.9f});
        }
    }
}

static void paint_star4(Img& img) {
    f32 c = img.w * 0.5f;
    for (u32 y = 0; y < img.h; y++) {
        for (u32 x = 0; x < img.h; x++) {
            f32 dx = fabsf((f32)x - c) / c, dy = fabsf((f32)y - c) / c;
            f32 d = sqrtf(dx * dx + dy * dy);
            f32 cross = std::max(std::clamp(1.0f - dx * dx * 3.0f, 0.0f, 1.0f) * std::clamp(1.0f - dy * 2.5f, 0.0f, 1.0f),
                                 std::clamp(1.0f - dy * dy * 3.0f, 0.0f, 1.0f) * std::clamp(1.0f - dx * 2.5f, 0.0f, 1.0f));
            f32 core = std::clamp(1.0f - d * 2.4f, 0.0f, 1.0f);
            f32 a = std::clamp(cross * 0.65f + core, 0.0f, 1.0f);
            img.set(x, y, {1, 1, 1, a});
        }
    }
}

// ---------------------------------------------------------------------------
// 图集生成与入口
// ---------------------------------------------------------------------------
static std::shared_ptr<rhi::RHITexture> s_terrain, s_buildings, s_items, s_veins;
static std::shared_ptr<rhi::RHITexture> s_mecha, s_fx, s_nebula, s_clouds, s_flare, s_star4;

bool init(rhi::RHIDevice* device) {
    if (!device || s_terrain) return true;

    { // 地形 4x4 @128
        Img img(512, 512);
        for (int i = 0; i < 12; i++) paint_terrain_cell(img, (i % 4) * 128, (i / 4) * 128, i);
        s_terrain = upload(device, img);
    }
    { // 建筑 6x6 @128 (kind 0..20, 0=None 跳过)
        Img img(768, 768);
        for (int i = 1; i <= 20; i++) paint_building_cell(img, ((i - 1) % 6) * 128, ((i - 1) / 6) * 128, i);
        s_buildings = upload(device, img);
    }
    { // 物品 8x8 @32
        Img img(256, 256);
        for (int i = 1; i <= 33; i++) paint_item_cell(img, (i % 8) * 32, (i / 8) * 32, i);
        s_items = upload(device, img);
    }
    { // 矿脉 8x1 @64
        Img img(512, 64);
        for (int i = 0; i < 7; i++) paint_vein_cell(img, i * 64, 0, i + 1);
        s_veins = upload(device, img);
    }
    { // 机甲
        Img img(128, 128);
        paint_mecha(img);
        s_mecha = upload(device, img);
    }
    { // 特效 4x2 @64
        Img img(256, 128);
        for (int i = 0; i < 6; i++) paint_fx_cell(img, (i % 4) * 64, (i / 4) * 64, i);
        s_fx = upload(device, img);
    }
    {
        Img img(512, 512); paint_nebula(img, 3.3f);
        s_nebula = upload(device, img);
    }
    {
        Img img(512, 256); paint_clouds(img);
        s_clouds = upload(device, img);
    }
    {
        Img img(256, 64); paint_flare(img);
        s_flare = upload(device, img);
    }
    {
        Img img(128, 128); paint_star4(img);
        s_star4 = upload(device, img);
    }
    return true;
}

void shutdown() {
    s_terrain.reset(); s_buildings.reset(); s_items.reset(); s_veins.reset();
    s_mecha.reset(); s_fx.reset(); s_nebula.reset(); s_clouds.reset();
    s_flare.reset(); s_star4.reset();
}

const std::shared_ptr<rhi::RHITexture>& tex_terrain() { return s_terrain; }
const std::shared_ptr<rhi::RHITexture>& tex_buildings() { return s_buildings; }
const std::shared_ptr<rhi::RHITexture>& tex_items() { return s_items; }
const std::shared_ptr<rhi::RHITexture>& tex_veins() { return s_veins; }
const std::shared_ptr<rhi::RHITexture>& tex_mecha() { return s_mecha; }
const std::shared_ptr<rhi::RHITexture>& tex_fx() { return s_fx; }
const std::shared_ptr<rhi::RHITexture>& tex_nebula() { return s_nebula; }
const std::shared_ptr<rhi::RHITexture>& tex_clouds() { return s_clouds; }
const std::shared_ptr<rhi::RHITexture>& tex_flare() { return s_flare; }
const std::shared_ptr<rhi::RHITexture>& tex_star4() { return s_star4; }

void cell_uv(int cols, int rows, int idx, Vec2& uv0, Vec2& uv1) {
    int cx = idx % cols, cy = idx / cols;
    uv0 = {(f32)cx / cols, (f32)cy / rows};
    uv1 = {(f32)(cx + 1) / cols, (f32)(cy + 1) / rows};
}

} // namespace dsp_art
