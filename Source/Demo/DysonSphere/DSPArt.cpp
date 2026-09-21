#include "DSPArt.h"
#include "PlanetGrid.h"
#include "Texture.h"
#include "Math/Math.h"
#include <cmath>

namespace DSPArt
{

using namespace Aether;
using namespace Aether::Math;

// ---------------------------------------------------------------------------
// 极简像素画板
// ---------------------------------------------------------------------------
struct Img
{
    u32 W = 0, H = 0;
    Array<u8> Px;

    Img(u32 imgW, u32 imgH) : W(imgW), H(imgH) { Px.Resize(W * H * 4, 0); }

    void Blend(u32 x, u32 y, Color c)
    {
        if (x >= W || y >= H) return;
        u8* p = &Px[(y * W + x) * 4];
        f32 a = Math::Clamp(c.a, 0.0f, 1.0f);
        p[0] = (u8)Math::Clamp(p[0] * (1 - a) + c.r * 255.0f * a, 0.0f, 255.0f);
        p[1] = (u8)Math::Clamp(p[1] * (1 - a) + c.g * 255.0f * a, 0.0f, 255.0f);
        p[2] = (u8)Math::Clamp(p[2] * (1 - a) + c.b * 255.0f * a, 0.0f, 255.0f);
        p[3] = (u8)Math::Clamp((f32)p[3] + a * 255.0f * (1.0f - (f32)p[3] / 255.0f), 0.0f, 255.0f);
    }
    void Set(u32 x, u32 y, Color c)
    {
        if (x >= W || y >= H) return;
        u8* p = &Px[(y * W + x) * 4];
        p[0] = (u8)Math::Clamp(c.r * 255.0f, 0.0f, 255.0f);
        p[1] = (u8)Math::Clamp(c.g * 255.0f, 0.0f, 255.0f);
        p[2] = (u8)Math::Clamp(c.b * 255.0f, 0.0f, 255.0f);
        p[3] = (u8)Math::Clamp(c.a * 255.0f, 0.0f, 255.0f);
    }
    void Rect(f32 x, f32 y, f32 rw, f32 rh, Color c)
    {
        for (i32 yy = (i32)floorf(y); yy < (i32)ceilf(y + rh); yy++)
            for (i32 xx = (i32)floorf(x); xx < (i32)ceilf(x + rw); xx++) Blend((u32)xx, (u32)yy, c);
    }
    void Disc(f32 cx, f32 cy, f32 r, Color c)
    {
        i32 x0 = (i32)(cx - r - 1), x1 = (i32)(cx + r + 1);
        i32 y0 = (i32)(cy - r - 1), y1 = (i32)(cy + r + 1);
        for (i32 yy = y0; yy <= y1; yy++)
        {
            for (i32 xx = x0; xx <= x1; xx++)
            {
                f32 d = sqrtf((xx + 0.5f - cx) * (xx + 0.5f - cx) + (yy + 0.5f - cy) * (yy + 0.5f - cy));
                f32 a = Math::Clamp(r - d + 0.5f, 0.0f, 1.0f) * c.a;
                if (a > 0.001f) Blend((u32)xx, (u32)yy, {c.r, c.g, c.b, a});
            }
        }
    }
    void Ring(f32 cx, f32 cy, f32 r, f32 th, Color c)
    {
        i32 x0 = (i32)(cx - r - th - 1), x1 = (i32)(cx + r + th + 1);
        i32 y0 = (i32)(cy - r - th - 1), y1 = (i32)(cy + r + th + 1);
        for (i32 yy = y0; yy <= y1; yy++)
        {
            for (i32 xx = x0; xx <= x1; xx++)
            {
                f32 d = sqrtf((xx + 0.5f - cx) * (xx + 0.5f - cx) + (yy + 0.5f - cy) * (yy + 0.5f - cy));
                f32 a = Math::Clamp(th * 0.5f - fabsf(d - r) + 0.5f, 0.0f, 1.0f) * c.a;
                if (a > 0.001f) Blend((u32)xx, (u32)yy, {c.r, c.g, c.b, a});
            }
        }
    }
    void Line(f32 x0, f32 y0, f32 x1, f32 y1, f32 wd, Color c)
    {
        f32 minx = Math::Min(x0, x1) - wd - 1, maxx = Math::Max(x0, x1) + wd + 1;
        f32 miny = Math::Min(y0, y1) - wd - 1, maxy = Math::Max(y0, y1) + wd + 1;
        f32 dx = x1 - x0, dy = y1 - y0;
        f32 len2 = dx * dx + dy * dy;
        for (i32 yy = (i32)miny; yy <= (i32)maxy; yy++)
        {
            for (i32 xx = (i32)minx; xx <= (i32)maxx; xx++)
            {
                f32 px = xx + 0.5f, py = yy + 0.5f;
                f32 t = len2 > 0.0001f ? Math::Clamp(((px - x0) * dx + (py - y0) * dy) / len2, 0.0f, 1.0f) : 0.0f;
                f32 qx = x0 + dx * t, qy = y0 + dy * t;
                f32 d = sqrtf((px - qx) * (px - qx) + (py - qy) * (py - qy));
                f32 a = Math::Clamp(wd * 0.5f - d + 0.5f, 0.0f, 1.0f) * c.a;
                if (a > 0.001f) Blend((u32)xx, (u32)yy, {c.r, c.g, c.b, a});
            }
        }
    }
    void Rounded(f32 x, f32 y, f32 rw, f32 rh, f32 r, Color c)
    {
        for (i32 yy = (i32)y; yy < (i32)(y + rh); yy++)
        {
            for (i32 xx = (i32)x; xx < (i32)(x + rw); xx++)
            {
                f32 px = xx + 0.5f, py = yy + 0.5f;
                f32 qx = Math::Clamp(px, x + r, x + rw - r);
                f32 qy = Math::Clamp(py, y + r, y + rh - r);
                f32 d = sqrtf((px - qx) * (px - qx) + (py - qy) * (py - qy));
                f32 a = Math::Clamp(r - d + 0.5f, 0.0f, 1.0f) * c.a;
                if (a > 0.001f) Blend((u32)xx, (u32)yy, {c.r, c.g, c.b, a});
            }
        }
    }
    void VGrad(f32 x, f32 y, f32 rw, f32 rh, Color top, Color bottom)
    {
        for (i32 yy = (i32)y; yy < (i32)(y + rh); yy++)
        {
            f32 t = (rh > 0.0f) ? (yy - y) / rh : 0.0f;
            Color c{top.r * (1 - t) + bottom.r * t, top.g * (1 - t) + bottom.g * t,
                    top.b * (1 - t) + bottom.b * t, top.a * (1 - t) + bottom.a * t};
            for (i32 xx = (i32)x; xx < (i32)(x + rw); xx++) Blend((u32)xx, (u32)yy, c);
        }
    }
};

// 可复现噪声
static f32 Hash2(f32 x, f32 y, f32 s = 0.0f)
{
    f32 n = sinf(x * 127.1f + y * 311.7f + s * 74.7f) * 43758.5453f;
    return n - floorf(n);
}
static f32 VNoise(f32 x, f32 y, f32 s = 0.0f)
{
    f32 ix = floorf(x), iy = floorf(y);
    f32 fx = x - ix, fy = y - iy;
    f32 wx = fx * fx * (3 - 2 * fx), wy = fy * fy * (3 - 2 * fy);
    f32 a = Hash2(ix, iy, s), b = Hash2(ix + 1, iy, s);
    f32 c = Hash2(ix, iy + 1, s), d = Hash2(ix + 1, iy + 1, s);
    return (a * (1 - wx) + b * wx) * (1 - wy) + (c * (1 - wx) + d * wx) * wy;
}
static f32 Fbm2(f32 x, f32 y, f32 s = 0.0f, int oct = 4)
{
    f32 sum = 0, amp = 0.5f, fr = 1.0f;
    for (int i = 0; i < oct; i++)
    {
        sum += VNoise(x * fr, y * fr, s + i * 17.3f) * amp;
        fr *= 2.03f; amp *= 0.5f;
    }
    return sum;
}

static RefPtr<RHI::RHITexture> Upload(RHI::RHIDevice* dev, const Img& img)
{
    return Engine::MakeTexture(dev, img.W, img.H, [&](u32 x, u32 y, u8* rgba) {
        const u8* p = &img.Px[(y * img.W + x) * 4];
        rgba[0] = p[0]; rgba[1] = p[1]; rgba[2] = p[2]; rgba[3] = p[3];
    });
}

// ---------------------------------------------------------------------------
// 调色板
// ---------------------------------------------------------------------------
static const Color HULL{0.545f, 0.58f, 0.635f, 1.0f};     // 舰体灰
static const Color HULL_DARK{0.29f, 0.32f, 0.37f, 1.0f};
static const Color HULL_LIGHT{0.78f, 0.82f, 0.87f, 1.0f};
static const Color CYAN{0.20f, 0.88f, 1.0f, 1.0f};
static const Color ORANGE{1.0f, 0.54f, 0.24f, 1.0f};
static const Color GOLD{1.0f, 0.80f, 0.28f, 1.0f};
static const Color PURPLE{0.78f, 0.40f, 1.0f, 1.0f};
static const Color GREEN{0.30f, 0.92f, 0.52f, 1.0f};
static const Color STEEL{0.36f, 0.40f, 0.46f, 1.0f};

static Color Shade(Color c, f32 m) { return {c.r * m, c.g * m, c.b * m, c.a}; }

// ---------------------------------------------------------------------------
// 地形图集 4x4 @128
// ---------------------------------------------------------------------------
static void PaintTerrainCell(Img& img, u32 ox, u32 oy, int idx)
{
    const u32 S = 128;
    auto cell = [&](f32 u, f32 v) -> f32 { return Fbm2(u * 6.0f, v * 6.0f, (f32)idx * 7.7f, 4); };
    Color base, dark, light;
    switch (idx)
    {
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

    for (u32 y = 0; y < S; y++)
    {
        for (u32 x = 0; x < S; x++)
        {
            f32 u = (f32)x / S, v = (f32)y / S;
            f32 n = cell(u, v);
            f32 n2 = Fbm2(u * 18.0f, v * 18.0f, (f32)idx * 3.1f, 3);
            Color c = {base.r * (1 - n * 0.5f) + light.r * n * 0.5f,
                       base.g * (1 - n * 0.5f) + light.g * n * 0.5f,
                       base.b * (1 - n * 0.5f) + light.b * n * 0.5f, 1.0f};
            c = Shade(c, 0.92f + n2 * 0.16f);

            if (idx == 0 || idx == 1) // 海浪条纹
            {
                f32 wv = sinf((u * 9.0f + v * 3.0f) * 6.2831f + n * 7.0f);
                if (wv > 0.86f) c = Shade(light, 1.15f);
            }
            else if (idx == 4) // 森林树冠
            {
                f32 tr = Fbm2(u * 26.0f, v * 26.0f, 55.0f, 2);
                if (tr > 0.60f) c = Shade(light, 0.85f + tr * 0.3f);
            }
            else if (idx == 5) // 岩石裂纹
            {
                f32 cr = Fbm2(u * 12.0f, v * 12.0f, 91.0f, 3);
                if (fabsf(cr - 0.5f) < 0.02f) c = Shade(dark, 0.8f);
            }
            else if (idx == 6) // 雪地亮斑
            {
                if (Hash2((f32)x, (f32)y, 5.0f) > 0.985f) c = {1, 1, 1, 1};
            }
            else if (idx == 8) // 冰面裂纹
            {
                f32 cr = Fbm2(u * 10.0f, v * 10.0f, 33.0f, 3);
                if (fabsf(cr - 0.5f) < 0.018f) c = {0.55f, 0.72f, 0.86f, 1};
            }
            else if (idx == 11) // 熔岩裂纹
            {
                f32 cr = Fbm2(u * 8.0f, v * 8.0f, 77.0f, 3);
                f32 vein = 1.0f - Math::Clamp(fabsf(cr - 0.5f) * 22.0f, 0.0f, 1.0f);
                c = {c.r + vein * (light.r - c.r), c.g + vein * (light.g - c.g), c.b + vein * (light.b - c.b), 1};
            }
            else if (idx == 7) // 沙丘
            {
                f32 dune = sinf(u * 14.0f + Fbm2(u * 4, v * 4, 12.0f, 3) * 9.0f);
                c = Shade(c, 1.0f + dune * 0.07f);
            }
            img.Set(ox + x, oy + y, c);
        }
    }
}

// ---------------------------------------------------------------------------
// 建筑图集 6x6 @128 — 全部俯视图, 朝向 +X
// ---------------------------------------------------------------------------
static void PaintBuildingCell(Img& img, u32 ox, u32 oy, int kind)
{
    const f32 C = 64.0f; // 中心
    auto shadow = [&]() { img.Disc(C + 4, C + 6, 44, {0, 0, 0, 0.35f}); };
    auto stripes = [&](f32 x, f32 y, f32 w, f32 h) {
        for (f32 i = -h; i < w; i += 10.0f)
        {
            Color sc = (fmodf(i, 20.0f) < 10.0f) ? GOLD : HULL_DARK;
            for (f32 t = 0; t < h; t += 0.5f)
                for (f32 s = 0; s < 5.0f; s += 0.5f)
                    img.Blend((u32)(x + i + t + s), (u32)(y + t), {sc.r, sc.g, sc.b, sc.a});
        }
    };

    switch (kind)
    {
        case 1: // 传送带
        {
            img.Rect(ox + 8, oy + 44, 112, 40, {0, 0, 0, 0});
            img.Rounded(ox + 6, oy + 44, 116, 40, 6, Shade(HULL_DARK, 0.8f));
            img.Rounded(ox + 10, oy + 48, 108, 32, 4, Shade(HULL, 0.9f));
            for (int i = 0; i < 9; i++) img.Rect(ox + 14 + i * 12, oy + 50, 6, 28, Shade(HULL_DARK, 1.1f));
            img.Rect(ox + 10, oy + 44, 108, 5, GOLD);   // 危险边条
            img.Rect(ox + 10, oy + 79, 108, 5, GOLD);
            img.Line(ox + 46, oy + 64, ox + 74, oy + 64, 6, CYAN); // 流向箭头
            img.Line(ox + 66, oy + 54, ox + 78, oy + 64, 4, CYAN);
            img.Line(ox + 66, oy + 74, ox + 78, oy + 64, 4, CYAN);
            break;
        }
        case 2: // 分拣器
        {
            img.Disc(C + 2, C + 2, 22, {0, 0, 0, 0.35f});
            img.Disc(C, C, 20, HULL_DARK);
            img.Disc(C, C, 15, HULL);
            img.Disc(C, C, 7, Shade(STEEL, 1.2f));
            img.Line(ox + 70, oy + 64, ox + 112, oy + 64, 10, Shade(HULL, 1.05f));
            img.Line(ox + 108, oy + 58, ox + 118, oy + 64, 6, HULL_DARK);
            img.Line(ox + 108, oy + 70, ox + 118, oy + 64, 6, HULL_DARK);
            img.Disc(C, C, 4, CYAN);
            break;
        }
        case 3: // 采矿机
        {
            shadow();
            img.Ring(C, C, 40, 10, HULL_DARK);
            img.Ring(C, C, 34, 8, HULL);
            stripes(ox + 18, oy + 100, 92, 8);
            img.Disc(C, C, 24, Shade(STEEL, 1.1f));
            for (int i = 0; i < 8; i++)
            {
                f32 a = i * Math::PI / 4.0f;
                img.Line(C + cosf(a) * 8, C + sinf(a) * 8, C + cosf(a) * 22, C + sinf(a) * 22, 5, HULL_DARK);
            }
            img.Disc(C, C, 9, ORANGE);
            img.Disc(C, C, 5, {1.0f, 0.85f, 0.5f, 1});
            break;
        }
        case 4: // 抽油机
        {
            shadow();
            img.Disc(C, C, 34, HULL_DARK);
            img.Disc(C, C, 30, HULL);
            img.Disc(C, C, 18, Shade(STEEL, 0.9f));
            img.Line(C - 26, C + 14, C + 26, C - 14, 7, ORANGE); // 磕头梁
            img.Disc(C, C, 6, HULL_DARK);
            img.Disc(C + 26, C - 14, 7, HULL_DARK);
            img.Disc(C + 26, C - 14, 4, {0.15f, 0.1f, 0.2f, 1});
            img.Ring(C, C, 12, 3, CYAN);
            break;
        }
        case 5: // 弧光熔炉
        {
            shadow();
            img.Rounded(ox + 18, oy + 22, 92, 84, 10, HULL_DARK);
            img.Rounded(ox + 24, oy + 28, 80, 72, 8, HULL);
            img.Rounded(ox + 30, oy + 34, 68, 30, 6, Shade(STEEL, 1.15f));
            img.Disc(C, oy + 84, 16, {0.9f, 0.35f, 0.08f, 1});   // 出料口熔光
            img.Disc(C, oy + 84, 10, {1.0f, 0.7f, 0.2f, 1});
            img.Rect(ox + 30, oy + 70, 68, 6, HULL_DARK);
            img.Disc(ox + 34, oy + 40, 5, CYAN);
            img.Disc(ox + 94, oy + 40, 5, CYAN);
            break;
        }
        case 6: // 制造台
        {
            shadow();
            img.Rounded(ox + 16, oy + 26, 96, 76, 8, HULL_DARK);
            img.Rounded(ox + 22, oy + 32, 84, 64, 6, HULL);
            img.Rounded(ox + 40, oy + 48, 48, 32, 4, Shade(STEEL, 1.2f));
            img.Line(ox + 46, oy + 40, ox + 40, oy + 70, 8, HULL_DARK); // 机械臂
            img.Disc(ox + 40, oy + 72, 7, HULL_DARK);
            img.Disc(ox + 40, oy + 72, 4, CYAN);
            img.Line(ox + 82, oy + 40, ox + 88, oy + 70, 8, HULL_DARK);
            img.Disc(ox + 88, oy + 72, 7, HULL_DARK);
            img.Disc(ox + 88, oy + 72, 4, ORANGE);
            img.Disc(C, C, 9, Shade(STEEL, 1.4f));
            for (int i = 0; i < 6; i++)
            {
                f32 a = i * Math::PI / 3.0f;
                img.Line(C + cosf(a) * 3, C + sinf(a) * 3, C + cosf(a) * 8, C + sinf(a) * 8, 2, HULL_DARK);
            }
            break;
        }
        case 7: // 化工厂
        {
            shadow();
            img.Ring(C - 14, C + 8, 20, 8, HULL);
            img.Ring(C + 18, C + 8, 20, 8, HULL);
            img.Disc(C - 14, C + 8, 14, Shade(STEEL, 0.95f));
            img.Disc(C + 18, C + 8, 14, Shade(STEEL, 0.95f));
            img.Rect(ox + 20, oy + 88, 88, 10, HULL_DARK);
            img.Line(ox + 50, oy + 30, ox + 50, oy + 56, 6, HULL_DARK);
            img.Disc(ox + 50, oy + 26, 8, HULL_DARK);
            img.Disc(ox + 50, oy + 26, 5, GREEN);
            img.Ring(C - 14, C + 8, 8, 2, CYAN);
            break;
        }
        case 8: // 矩阵实验室
        {
            shadow();
            img.Ring(C, C, 38, 10, HULL_DARK);
            img.Ring(C, C, 30, 8, HULL);
            img.Disc(C, C, 22, Shade(STEEL, 1.1f));
            img.Disc(C, C, 15, {0.1f, 0.5f, 0.9f, 1});
            img.Disc(C, C, 9, {0.5f, 0.9f, 1.0f, 1});
            img.Ring(C, C, 26, 2.5f, CYAN);
            for (int i = 0; i < 4; i++)
            {
                f32 a = i * Math::HALF_PI;
                img.Line(C + cosf(a) * 30, C + sinf(a) * 30, C + cosf(a) * 38, C + sinf(a) * 38, 5, HULL_DARK);
            }
            break;
        }
        case 9: case 10: // PLS / ILS
        {
            shadow();
            Color ac = (kind == 9) ? GOLD : PURPLE;
            img.Ring(C, C, 44, 8, HULL_DARK);
            img.Ring(C, C, 36, 6, HULL);
            img.Ring(C, C, 24, 4, Shade(ac, 0.9f));
            img.Disc(C, C, 16, Shade(STEEL, 1.1f));
            img.Disc(C, C, 10, Shade(ac, 0.7f));
            img.Disc(C, C, 5, {1, 1, 1, 1});
            for (int i = 0; i < 4; i++)
            {
                f32 a = i * Math::HALF_PI + Math::PI * 0.25f;
                f32 px = C + cosf(a) * 34, py = C + sinf(a) * 34;
                img.Rounded(px - 8, py - 8, 16, 16, 3, HULL_DARK);
                img.Rounded(px - 5, py - 5, 10, 10, 2, ac);
            }
            break;
        }
        case 11: // 电磁弹射器
        {
            shadow();
            img.Rounded(ox + 20, oy + 30, 88, 68, 8, HULL_DARK);
            img.Rounded(ox + 26, oy + 36, 76, 56, 6, HULL);
            img.Line(ox + 30, oy + 88, ox + 104, oy + 40, 10, Shade(STEEL, 1.3f)); // 轨道
            for (int i = 0; i < 4; i++)
            {
                f32 t = i / 3.0f;
                f32 px = ox + 30 + 74 * t, py = oy + 88 - 48 * t;
                img.Ring(px, py, 8, 3, CYAN);
            }
            img.Disc(ox + 96, oy + 44, 6, {1, 0.9f, 0.5f, 1});
            break;
        }
        case 12: // 垂直发射井
        {
            shadow();
            img.Ring(C, C, 42, 10, HULL_DARK);
            img.Ring(C, C, 34, 7, HULL);
            img.Disc(C, C, 24, Shade(STEEL, 0.9f));
            img.Disc(C, C, 15, {0.05f, 0.05f, 0.08f, 1});
            img.Disc(C, C, 9, ORANGE);
            img.Rect(ox + 90, oy + 40, 14, 48, HULL_DARK); // 发射塔
            img.Rect(ox + 92, oy + 42, 4, 44, CYAN);
            break;
        }
        case 13: // 射线接收站
        {
            shadow();
            img.Ring(C, C + 10, 34, 9, HULL_DARK);
            img.Disc(C, C + 10, 28, Shade(STEEL, 1.0f));
            img.Disc(C, C + 10, 20, {0.15f, 0.25f, 0.5f, 1});
            img.Disc(C, C + 10, 12, PURPLE);
            img.Line(C, C + 10, C, C - 34, 6, HULL_DARK);
            img.Disc(C, C - 36, 7, PURPLE);
            img.Disc(C, C - 36, 4, {1, 1, 1, 1});
            break;
        }
        case 14: // 人造恒星
        {
            img.Disc(C, C, 30, {0.5f, 0.1f, 0.9f, 0.35f});
            img.Disc(C, C, 20, {0.9f, 0.5f, 1.0f, 1});
            img.Disc(C, C, 12, {1.0f, 0.95f, 1.0f, 1});
            img.Ring(C, C, 26, 3, CYAN);
            img.Ring(C, C, 34, 2, {0.5f, 0.8f, 1.0f, 0.7f});
            for (int i = 0; i < 6; i++)
            {
                f32 a = i * Math::PI / 3.0f;
                img.Line(C + cosf(a) * 34, C + sinf(a) * 34, C + cosf(a) * 42, C + sinf(a) * 42, 4, HULL_DARK);
            }
            break;
        }
        case 15: // 电塔
        {
            img.Disc(C, C, 18, HULL_DARK);
            img.Disc(C, C, 13, HULL);
            img.Disc(C, C, 6, STEEL);
            img.Line(C - 14, C + 14, C + 14, C - 14, 4, HULL_LIGHT);
            img.Line(C - 14, C - 14, C + 14, C + 14, 4, HULL_LIGHT);
            img.Disc(C, C, 3, CYAN);
            break;
        }
        case 16: // 无线充能塔
        {
            img.Disc(C, C, 22, HULL_DARK);
            img.Disc(C, C, 17, HULL);
            img.Ring(C, C, 11, 3, CYAN);
            img.Ring(C, C, 6, 3, GOLD);
            img.Disc(C, C, 2.5f, {1, 1, 1, 1});
            break;
        }
        case 17: // 风力涡轮
        {
            img.Disc(C, C, 20, HULL_DARK);
            img.Disc(C, C, 15, HULL);
            for (int i = 0; i < 3; i++)
            {
                f32 a = i * Math::TWO_PI / 3.0f;
                img.Line(C, C, C + cosf(a) * 42, C + sinf(a) * 42, 7, HULL_LIGHT);
                img.Disc(C + cosf(a) * 42, C + sinf(a) * 42, 4, CYAN);
            }
            img.Disc(C, C, 7, HULL_DARK);
            img.Disc(C, C, 3, CYAN);
            break;
        }
        case 18: // 太阳能板
        {
            img.Rounded(ox + 12, oy + 30, 104, 68, 4, HULL_DARK);
            img.Rounded(ox + 16, oy + 34, 96, 60, 3, {0.12f, 0.32f, 0.62f, 1});
            for (int i = 1; i < 4; i++) img.Rect(ox + 16 + i * 24, oy + 34, 2, 60, HULL_DARK);
            for (int i = 1; i < 3; i++) img.Rect(ox + 16, oy + 34 + i * 20, 96, 2, HULL_DARK);
            img.Line(ox + 20, oy + 40, ox + 40, oy + 40, 2, {0.5f, 0.8f, 1.0f, 0.8f});
            img.Rounded(ox + 56, oy + 56, 16, 16, 2, HULL);
            break;
        }
        case 19: // 火力发电
        {
            shadow();
            img.Rounded(ox + 16, oy + 30, 96, 68, 6, HULL_DARK);
            img.Rounded(ox + 22, oy + 36, 84, 56, 5, HULL);
            img.Rect(ox + 30, oy + 20, 16, 24, HULL_DARK);
            img.Rect(ox + 58, oy + 20, 16, 24, HULL_DARK);
            img.Rect(ox + 86, oy + 20, 16, 24, HULL_DARK);
            img.Rect(ox + 32, oy + 22, 12, 6, Shade(STEEL, 1.3f));
            img.Rect(ox + 60, oy + 22, 12, 6, Shade(STEEL, 1.3f));
            img.Rect(ox + 88, oy + 22, 12, 6, Shade(STEEL, 1.3f));
            img.Rect(ox + 28, oy + 70, 72, 10, ORANGE);
            break;
        }
        default: // 聚变电站 (20)
        {
            shadow();
            img.Ring(C, C, 36, 10, HULL_DARK);
            img.Ring(C, C, 28, 7, HULL);
            img.Disc(C, C, 18, {0.1f, 0.15f, 0.25f, 1});
            img.Ring(C, C, 12, 5, CYAN);
            img.Disc(C, C, 6, {1, 1, 1, 1});
            for (int i = 0; i < 8; i++)
            {
                f32 a = i * Math::PI / 4.0f;
                img.Line(C + cosf(a) * 36, C + sinf(a) * 36, C + cosf(a) * 44, C + sinf(a) * 44, 4, HULL_DARK);
            }
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// 物品图标 8x8 @32
// ---------------------------------------------------------------------------
static void PaintItemCell(Img& img, u32 ox, u32 oy, int kind)
{
    const f32 C = 16.0f;
    // 常用形状
    auto oreChunk = [&](Color c) {
        img.Disc(C - 4, C + 3, 6, Shade(c, 0.7f));
        img.Disc(C + 5, C + 4, 5, Shade(c, 0.8f));
        img.Disc(C, C - 3, 7, c);
        img.Disc(C - 2, C - 5, 2.5f, Shade(c, 1.35f));
        img.Disc(C + 3, C + 1, 1.8f, Shade(c, 1.3f));
    };
    auto ingot = [&](Color c) {
        img.Rounded(ox + 5, oy + 12, 22, 9, 3, Shade(c, 0.75f));
        img.Rounded(ox + 7, oy + 10, 18, 8, 2.5f, c);
        img.Rounded(ox + 9, oy + 12, 10, 3, 1.5f, Shade(c, 1.4f));
    };
    auto cube = [&](Color c) {
        img.Rounded(ox + 7, oy + 7, 18, 18, 4, Shade(c, 0.65f));
        img.Rounded(ox + 9, oy + 9, 14, 14, 3, c);
        img.Disc(C - 2, C - 2, 3, Shade(c, 1.5f));
    };

    switch (kind)
    {
        case 1: oreChunk({0.55f, 0.60f, 0.68f, 1}); break;          // 铁矿
        case 2: oreChunk({0.88f, 0.55f, 0.30f, 1}); break;          // 铜矿
        case 3: for (int i = 0; i < 4; i++)                          // 煤
                img.Disc(C - 6 + i * 4, C + (i % 2) * 5 - 2, 4.5f, {0.13f, 0.13f, 0.15f, 1});
                img.Disc(C - 1, C - 4, 2, {0.35f, 0.35f, 0.4f, 1}); break;
        case 4: oreChunk({0.72f, 0.72f, 0.70f, 1}); break;          // 石材
        case 5: oreChunk({0.80f, 0.85f, 0.95f, 1}); break;          // 钛矿
        case 6: oreChunk({0.45f, 0.78f, 0.85f, 1}); break;          // 硅矿
        case 7: img.Disc(C, C + 2, 8, {0.10f, 0.08f, 0.13f, 1});     // 原油
                img.Disc(C - 2, C, 3, {0.4f, 0.2f, 0.5f, 0.9f}); break;
        case 8: ingot({0.72f, 0.76f, 0.84f, 1}); break;              // 铁块
        case 9: ingot({0.92f, 0.58f, 0.30f, 1}); break;              // 铜块
        case 10: img.Rounded(ox + 8, oy + 8, 16, 16, 7, HULL);       // 磁铁 (U形)
                 img.Rounded(ox + 11, oy + 11, 10, 10, 4, {0.1f, 0.1f, 0.15f, 1});
                 img.Rect(ox + 8, oy + 20, 6, 4, {0.9f, 0.3f, 0.3f, 1});
                 img.Rect(ox + 18, oy + 20, 6, 4, {0.3f, 0.4f, 0.95f, 1}); break;
        case 11: for (int i = 0; i < 4; i++)                          // 磁线圈
                 img.Ring(C, C, 3 + i * 2.6f, 2, (i % 2) ? ORANGE : Color{0.9f, 0.65f, 0.4f, 1.0f});
                 img.Disc(C, C, 2, CYAN); break;
        case 12: img.Rounded(ox + 6, oy + 8, 20, 16, 2, {0.10f, 0.35f, 0.18f, 1}); // 电路板
                 img.Rounded(ox + 8, oy + 10, 16, 12, 1, {0.14f, 0.5f, 0.25f, 1});
                 img.Rect(ox + 12, oy + 12, 8, 8, GOLD);
                 img.Line(ox + 8, oy + 16, ox + 12, oy + 16, 1, GOLD);
                 img.Line(ox + 20, oy + 12, ox + 24, oy + 10, 1, GOLD); break;
        case 13: img.Disc(C, C, 9, Shade(STEEL, 1.2f));              // 齿轮
                 img.Disc(C, C, 4, {0.1f, 0.1f, 0.12f, 1});
                 for (int i = 0; i < 8; i++)
                 {
                     f32 a = i * Math::PI / 4.0f;
                     img.Disc(C + cosf(a) * 10, C + sinf(a) * 10, 2.5f, Shade(STEEL, 1.2f));
                 } break;
        case 14: ingot({0.55f, 0.58f, 0.64f, 1}); break;              // 钢材
        case 15: ingot({0.85f, 0.90f, 1.0f, 1}); break;               // 钛块
        case 16: ingot({0.50f, 0.85f, 0.92f, 1}); break;              // 高纯硅
        case 17: img.Rounded(ox + 6, oy + 8, 20, 16, 2, {0.08f, 0.08f, 0.12f, 1}); // 处理器
                 img.Rounded(ox + 9, oy + 11, 14, 10, 1, CYAN);
                 for (int i = 0; i < 5; i++)
                 {
                     img.Rect(ox + 7 + i * 4, oy + 5, 2, 3, GOLD);
                     img.Rect(ox + 7 + i * 4, oy + 24, 2, 3, GOLD);
                 } break;
        case 18: case 19: ingot({0.90f, 0.75f, 0.25f, 1}); break;     // 电机/推进器
        case 20: ingot({0.95f, 0.95f, 0.90f, 1}); break;              // 塑料
        case 21: img.Disc(C, C, 9, {0.75f, 0.85f, 0.15f, 1});          // 硫酸
                 img.Disc(C, C - 2, 5, {0.9f, 0.95f, 0.3f, 1}); break;
        case 22: img.Rounded(ox + 10, oy + 6, 12, 20, 5, {0.85f, 0.35f, 0.45f, 0.9f}); // 氢
                 img.Rounded(ox + 12, oy + 9, 8, 12, 3, {1.0f, 0.6f, 0.65f, 0.9f}); break;
        case 23: img.Rounded(ox + 10, oy + 6, 12, 20, 5, {0.25f, 0.85f, 0.65f, 0.9f}); // 氘
                 img.Rounded(ox + 12, oy + 9, 8, 12, 3, {0.6f, 1.0f, 0.85f, 0.9f}); break;
        case 24: cube({1.0f, 0.4f, 0.9f, 1}); break;                  // 反物质
        case 25: // 太阳帆
        {
            img.Line(ox + 8, oy + 24, ox + 24, oy + 24, 2, HULL_DARK);
            img.Line(ox + 16, oy + 24, ox + 16, oy + 8, 2, HULL_DARK);
            img.Rounded(ox + 10, oy + 6, 18, 14, 2, GOLD);
            img.Rounded(ox + 12, oy + 8, 14, 10, 1, {1.0f, 0.92f, 0.55f, 1});
        } break;
        case 26: // 运载火箭
        {
            img.Rounded(ox + 13, oy + 8, 6, 14, 3, HULL_LIGHT);
            img.Disc(ox + 16, oy + 8, 3, ORANGE);
            img.Line(ox + 13, oy + 22, ox + 10, oy + 27, 2, HULL_DARK);
            img.Line(ox + 19, oy + 22, ox + 22, oy + 27, 2, HULL_DARK);
            img.Disc(ox + 16, oy + 14, 1.5f, CYAN);
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
static void PaintVeinCell(Img& img, u32 ox, u32 oy, int res)
{
    (void)ox; (void)oy;
    Color c;
    switch (res)
    {
        case 1: c = {0.45f, 0.62f, 0.85f, 1}; break;   // 铁
        case 2: c = {0.92f, 0.55f, 0.25f, 1}; break;   // 铜
        case 3: c = {0.22f, 0.22f, 0.25f, 1}; break;   // 煤
        case 4: c = {0.75f, 0.75f, 0.72f, 1}; break;   // 石材
        case 5: c = {0.75f, 0.85f, 1.0f, 1}; break;    // 钛
        case 6: c = {0.40f, 0.85f, 0.90f, 1}; break;   // 硅
        default: c = {0.65f, 0.30f, 0.85f, 1}; break;  // 原油
    }
    // 岩基
    img.Disc(32, 44, 20, {0.30f, 0.28f, 0.26f, 1});
    img.Disc(24, 46, 14, {0.36f, 0.34f, 0.32f, 1});
    img.Disc(42, 45, 13, {0.33f, 0.31f, 0.29f, 1});
    // 晶簇
    auto shard = [&](f32 x, f32 y, f32 hgt, f32 w, f32 tilt) {
        f32 x1 = x + tilt;
        img.Line(x, y, x1, y - hgt, w, c);
        img.Line(x1, y - hgt, x1, y - hgt, w * 0.4f, Shade(c, 1.5f));
    };
    shard(20, 42, 18, 5, -4);
    shard(32, 40, 26, 6, 2);
    shard(44, 43, 15, 5, 6);
    shard(27, 44, 10, 4, 5);
    shard(38, 44, 12, 4, -3);
    img.Disc(32, 16, 2.5f, {1, 1, 1, 0.9f});
    if (res == 7) // 原油: 油池
    {
        img.Disc(32, 44, 15, {0.10f, 0.08f, 0.14f, 1});
        img.Disc(26, 40, 4, {0.4f, 0.2f, 0.55f, 0.8f});
    }
}

// ---------------------------------------------------------------------------
// 机甲 128x128 (背视)
// ---------------------------------------------------------------------------
static void PaintMecha(Img& img)
{
    // 腿
    img.Rounded(46, 88, 14, 32, 5, Shade(HULL_DARK, 0.9f));
    img.Rounded(68, 88, 14, 32, 5, Shade(HULL_DARK, 0.9f));
    img.Rounded(44, 114, 18, 10, 4, HULL_DARK);
    img.Rounded(66, 114, 18, 10, 4, HULL_DARK);
    // 躯干
    img.Rounded(38, 40, 52, 54, 12, HULL);
    img.Rounded(44, 46, 40, 30, 8, HULL_LIGHT);
    img.Rounded(50, 50, 28, 18, 5, Shade(STEEL, 1.1f));
    // 肩甲
    img.Rounded(24, 42, 18, 26, 6, HULL_DARK);
    img.Rounded(86, 42, 18, 26, 6, HULL_DARK);
    img.Rounded(27, 45, 12, 20, 4, HULL);
    img.Rounded(89, 45, 12, 20, 4, HULL);
    img.Disc(33, 55, 4, CYAN);
    img.Disc(95, 55, 4, CYAN);
    // 头部
    img.Rounded(52, 22, 24, 20, 6, HULL_LIGHT);
    img.Rounded(55, 27, 18, 9, 4, {0.08f, 0.12f, 0.18f, 1});
    img.Rounded(57, 29, 14, 4, 2, CYAN);
    // 核心反应堆
    img.Disc(64, 84, 10, {0.1f, 0.4f, 0.6f, 1});
    img.Disc(64, 84, 6, CYAN);
    img.Disc(64, 84, 3, {1, 1, 1, 1});
    // 背包推进器
    img.Rounded(46, 30, 12, 14, 4, HULL_DARK);
    img.Rounded(70, 30, 12, 14, 4, HULL_DARK);
    img.Disc(52, 30, 3, ORANGE);
    img.Disc(76, 30, 3, ORANGE);
    // 描边增强
    img.Ring(64, 84, 12, 2, {0.2f, 0.9f, 1.0f, 0.55f});
}

// ---------------------------------------------------------------------------
// 特效 4x2 @64: 0太阳帆 1火箭 2无人机 3货船 4能量点 5火花
// ---------------------------------------------------------------------------
static void PaintFxCell(Img& img, u32 ox, u32 oy, int idx)
{
    const f32 C = 32.0f;
    switch (idx)
    {
        case 0: // 太阳帆
            img.Line(C, 52, C, 44, 2.5f, HULL_DARK);
            img.Rounded(ox + 12, oy + 12, 40, 30, 2, GOLD);
            img.Rounded(ox + 15, oy + 15, 34, 24, 1, {1.0f, 0.92f, 0.55f, 1});
            img.Line(ox + 12, oy + 27, ox + 52, oy + 27, 1.5f, Shade(GOLD, 0.7f));
            break;
        case 1: // 火箭
            img.Rounded(C - 6, 14, 12, 34, 5, HULL_LIGHT);
            img.Disc(C, 14, 6, {0.95f, 0.45f, 0.2f, 1});
            img.Line(C - 6, 44, C - 12, 56, 3, HULL_DARK);
            img.Line(C + 6, 44, C + 12, 56, 3, HULL_DARK);
            img.Disc(C, 30, 2.5f, CYAN);
            break;
        case 2: // 无人机
            img.Rounded(C - 8, C - 5, 16, 10, 4, HULL);
            img.Disc(C, C, 3, GREEN);
            img.Disc(C - 10, C - 2, 2, HULL_DARK);
            img.Disc(C + 10, C - 2, 2, HULL_DARK);
            break;
        case 3: // 货船
            img.Rounded(C - 18, C - 6, 36, 12, 5, HULL);
            img.Rounded(C + 8, C - 4, 12, 8, 3, HULL_LIGHT);
            img.Disc(C + 17, C, 2, GOLD);
            img.Disc(C - 14, C, 2.5f, CYAN);
            break;
        case 4: // 能量点
            img.Disc(C, C, 14, {1, 1, 1, 0.25f});
            img.Disc(C, C, 8, {1, 1, 1, 0.7f});
            img.Disc(C, C, 4, {1, 1, 1, 1});
            break;
        default: // 火花
            img.Line(C - 12, C + 12, C + 12, C - 12, 3, {1, 0.9f, 0.6f, 0.9f});
            img.Disc(C - 12, C + 12, 4, {1, 0.7f, 0.3f, 0.6f});
            break;
    }
}

// ---------------------------------------------------------------------------
// 星云 / 云层 / 光晕
// ---------------------------------------------------------------------------
static void PaintNebula(Img& img, f32 seed)
{
    for (u32 y = 0; y < img.H; y++)
    {
        for (u32 x = 0; x < img.W; x++)
        {
            f32 u = (f32)x / img.W, v = (f32)y / img.H;
            // 环形距离衰减, 保证平铺不生硬
            f32 dx = u - 0.5f, dy = v - 0.5f;
            f32 fall = Math::Clamp(1.0f - sqrtf(dx * dx + dy * dy) * 2.2f, 0.0f, 1.0f);
            f32 n = Fbm2(u * 3.5f, v * 3.5f, seed, 5);
            f32 n2 = Fbm2(u * 9.0f, v * 9.0f, seed + 31.0f, 4);
            f32 a = Math::Clamp((n - 0.38f) * 2.2f, 0.0f, 1.0f) * fall * (0.55f + n2 * 0.45f);
            f32 d2 = Math::Clamp((n2 - 0.5f) * 2.0f, 0.0f, 1.0f);
            img.Set(x, y, {0.75f + d2 * 0.25f, 0.7f, 1.0f - d2 * 0.3f, a * 0.85f});
        }
    }
}

static void PaintClouds(Img& img)
{
    for (u32 y = 0; y < img.H; y++)
    {
        for (u32 x = 0; x < img.W; x++)
        {
            f32 u = (f32)x / img.W * 4.0f, v = (f32)y / img.H * 2.0f;
            f32 n = Fbm2(u, v, 7.7f, 5);
            f32 wisp = Fbm2(u * 2.5f + 5.0f, v * 1.2f, 19.0f, 4);
            f32 a = Math::Clamp((n * 0.7f + wisp * 0.3f - 0.52f) * 3.2f, 0.0f, 1.0f);
            f32 sh = 0.8f + (n - 0.5f) * 0.5f;
            img.Set(x, y, {sh, sh, sh * 1.05f, a});
        }
    }
}

static void PaintFlare(Img& img)
{
    f32 cx = img.W * 0.5f, cy = img.H * 0.5f;
    for (u32 y = 0; y < img.H; y++)
    {
        for (u32 x = 0; x < img.W; x++)
        {
            f32 dx = fabsf((f32)x - cx) / cx;
            f32 dy = fabsf((f32)y - cy) / cy;
            f32 a = Math::Clamp(1.0f - dx, 0.0f, 1.0f);
            a *= a;
            a *= Math::Clamp(1.0f - dy * dy * 1.6f, 0.0f, 1.0f);
            a *= 0.8f + 0.2f * Hash2((f32)x, (f32)y, 3.0f);
            img.Set(x, y, {1.0f, 0.95f, 0.85f, a * 0.9f});
        }
    }
}

static void PaintStar4(Img& img)
{
    f32 c = img.W * 0.5f;
    for (u32 y = 0; y < img.H; y++)
    {
        for (u32 x = 0; x < img.H; x++)
        {
            f32 dx = fabsf((f32)x - c) / c, dy = fabsf((f32)y - c) / c;
            f32 d = sqrtf(dx * dx + dy * dy);
            f32 cross = Math::Max(Math::Clamp(1.0f - dx * dx * 3.0f, 0.0f, 1.0f) * Math::Clamp(1.0f - dy * 2.5f, 0.0f, 1.0f),
                                  Math::Clamp(1.0f - dy * dy * 3.0f, 0.0f, 1.0f) * Math::Clamp(1.0f - dx * 2.5f, 0.0f, 1.0f));
            f32 core = Math::Clamp(1.0f - d * 2.4f, 0.0f, 1.0f);
            f32 a = Math::Clamp(cross * 0.65f + core, 0.0f, 1.0f);
            img.Set(x, y, {1, 1, 1, a});
        }
    }
}

// ---------------------------------------------------------------------------
// 图集生成与入口
// ---------------------------------------------------------------------------
static RefPtr<RHI::RHITexture> sTerrain, sBuildings, sItems, sVeins;
static RefPtr<RHI::RHITexture> sMecha, sFx, sNebula, sClouds, sFlare, sStar4;

bool Init(RHI::RHIDevice* device)
{
    if (!device || sTerrain) return true;

    { // 地形 4x4 @128
        Img img(512, 512);
        for (int i = 0; i < 12; i++) PaintTerrainCell(img, (i % 4) * 128, (i / 4) * 128, i);
        sTerrain = Upload(device, img);
    }
    { // 建筑 6x6 @128 (kind 0..20, 0=None 跳过)
        Img img(768, 768);
        for (int i = 1; i <= 20; i++) PaintBuildingCell(img, ((i - 1) % 6) * 128, ((i - 1) / 6) * 128, i);
        sBuildings = Upload(device, img);
    }
    { // 物品 8x8 @32
        Img img(256, 256);
        for (int i = 1; i <= 33; i++) PaintItemCell(img, (i % 8) * 32, (i / 8) * 32, i);
        sItems = Upload(device, img);
    }
    { // 矿脉 8x1 @64
        Img img(512, 64);
        for (int i = 0; i < 7; i++) PaintVeinCell(img, i * 64, 0, i + 1);
        sVeins = Upload(device, img);
    }
    { // 机甲
        Img img(128, 128);
        PaintMecha(img);
        sMecha = Upload(device, img);
    }
    { // 特效 4x2 @64
        Img img(256, 128);
        for (int i = 0; i < 6; i++) PaintFxCell(img, (i % 4) * 64, (i / 4) * 64, i);
        sFx = Upload(device, img);
    }
    {
        Img img(512, 512); PaintNebula(img, 3.3f);
        sNebula = Upload(device, img);
    }
    {
        Img img(512, 256); PaintClouds(img);
        sClouds = Upload(device, img);
    }
    {
        Img img(256, 64); PaintFlare(img);
        sFlare = Upload(device, img);
    }
    {
        Img img(128, 128); PaintStar4(img);
        sStar4 = Upload(device, img);
    }
    return true;
}

void Shutdown()
{
    sTerrain.Reset(); sBuildings.Reset(); sItems.Reset(); sVeins.Reset();
    sMecha.Reset(); sFx.Reset(); sNebula.Reset(); sClouds.Reset();
    sFlare.Reset(); sStar4.Reset();
}

const RefPtr<RHI::RHITexture>& TexTerrain() { return sTerrain; }
const RefPtr<RHI::RHITexture>& TexBuildings() { return sBuildings; }
const RefPtr<RHI::RHITexture>& TexItems() { return sItems; }
const RefPtr<RHI::RHITexture>& TexVeins() { return sVeins; }
const RefPtr<RHI::RHITexture>& TexMecha() { return sMecha; }
const RefPtr<RHI::RHITexture>& TexFx() { return sFx; }
const RefPtr<RHI::RHITexture>& TexNebula() { return sNebula; }
const RefPtr<RHI::RHITexture>& TexClouds() { return sClouds; }
const RefPtr<RHI::RHITexture>& TexFlare() { return sFlare; }
const RefPtr<RHI::RHITexture>& TexStar4() { return sStar4; }

void CellUv(int cols, int rows, int idx, Vec2& uv0, Vec2& uv1)
{
    int cx = idx % cols, cy = idx / cols;
    uv0 = {(f32)cx / cols, (f32)cy / rows};
    uv1 = {(f32)(cx + 1) / cols, (f32)(cy + 1) / rows};
}

} // namespace DSPArt
