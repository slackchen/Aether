#include "Art.h"

#include "Container/Array.h"
#include "Math/Math.h"
#include "Math/Vec2.h"
#include "Texture.h"

#include <cmath>

using namespace Aether;

namespace Shmup
{

namespace
{

//
// Tiny software rasterizer used to bake the demo's pixel-art sprites into
// RGBA8 buffers before uploading them as textures.
//
struct Px
{
    u32 W = 0;
    u32 H = 0;
    Array<u8> Pixels;

    Px(u32 width, u32 height)
        : W(width)
        , H(height)
    {
        Pixels.Resize(width * height * 4);
    }

    void Blend(u32 x, u32 y, f32 r, f32 g, f32 b, f32 a)
    {
        if (x >= W || y >= H || a <= 0.0f)
        {
            return;
        }
        u8* p = &Pixels[(y * W + x) * 4];
        f32 sa = a > 1.0f ? 1.0f : a;
        f32 da = p[3] / 255.0f;
        f32 oa = sa + da * (1.0f - sa);
        if (oa <= 0.0f)
        {
            return;
        }
        p[0] = (u8)(((r * sa + (p[0] / 255.0f) * da * (1.0f - sa)) / oa) * 255.0f);
        p[1] = (u8)(((g * sa + (p[1] / 255.0f) * da * (1.0f - sa)) / oa) * 255.0f);
        p[2] = (u8)(((b * sa + (p[2] / 255.0f) * da * (1.0f - sa)) / oa) * 255.0f);
        p[3] = (u8)(oa * 255.0f);
    }

    void FillCircle(f32 cx, f32 cy, f32 rad, f32 r, f32 g, f32 b, f32 a, bool glow = false)
    {
        i32 x0 = (i32)floorf(cx - rad), x1 = (i32)ceilf(cx + rad);
        i32 y0 = (i32)floorf(cy - rad), y1 = (i32)ceilf(cy + rad);
        for (i32 y = y0; y <= y1; y++)
        {
            for (i32 x = x0; x <= x1; x++)
            {
                f32 dx = (f32)x + 0.5f - cx;
                f32 dy = (f32)y + 0.5f - cy;
                f32 dist = sqrtf(dx * dx + dy * dy);
                if (dist <= rad)
                {
                    f32 alpha = a;
                    if (glow)
                    {
                        alpha = a * (1.0f - dist / rad);
                    }
                    Blend((u32)x, (u32)y, r, g, b, alpha);
                }
            }
        }
    }

    static bool InPoly(f32 px, f32 py, const Array<Math::Vec2>& pts)
    {
        bool inside = false;
        u32 n = pts.Count();
        for (u32 i = 0, j = n - 1; i < n; j = i++)
        {
            if (((pts[i].y > py) != (pts[j].y > py)) &&
                (px < (pts[j].x - pts[i].x) * (py - pts[i].y) / (pts[j].y - pts[i].y) + pts[i].x))
            {
                inside = !inside;
            }
        }
        return inside;
    }

    void FillPoly(const Array<Math::Vec2>& pts, f32 r, f32 g, f32 b, f32 a)
    {
        f32 minx = 1e9f, maxx = -1e9f, miny = 1e9f, maxy = -1e9f;
        for (const Math::Vec2& p : pts)
        {
            minx = Math::Min(minx, p.x);
            maxx = Math::Max(maxx, p.x);
            miny = Math::Min(miny, p.y);
            maxy = Math::Max(maxy, p.y);
        }
        for (i32 y = (i32)floorf(miny); y <= (i32)ceilf(maxy); y++)
        {
            for (i32 x = (i32)floorf(minx); x <= (i32)ceilf(maxx); x++)
            {
                if (InPoly((f32)x + 0.5f, (f32)y + 0.5f, pts))
                {
                    Blend((u32)x, (u32)y, r, g, b, a);
                }
            }
        }
    }

    void ToTexture(RHI::RHIDevice* device, RefPtr<RHI::RHITexture>& out)
    {
        out = Engine::MakeTexture(device, W, H, [this](u32 x, u32 y, u8* rgba) {
            const u8* src = &Pixels[(y * W + x) * 4];
            rgba[0] = src[0];
            rgba[1] = src[1];
            rgba[2] = src[2];
            rgba[3] = src[3];
        });
    }
};

void DrawPlayer(Px& p)
{
    p.FillPoly({{14, 46}, {32, 26}, {32, 52}}, 0.05f, 0.4f, 0.9f, 1.0f);
    p.FillPoly({{50, 46}, {32, 26}, {32, 52}}, 0.05f, 0.4f, 0.9f, 1.0f);
    p.FillPoly({{32, 6}, {20, 44}, {44, 44}}, 0.25f, 0.85f, 1.0f, 1.0f);
    p.FillPoly({{32, 6}, {26, 40}, {38, 40}}, 0.9f, 0.97f, 1.0f, 1.0f);
    p.FillCircle(32, 33, 6, 0.08f, 0.18f, 0.5f, 1.0f);
    p.FillCircle(32, 33, 3, 0.85f, 0.95f, 1.0f, 1.0f);
    p.FillCircle(32, 53, 11, 1.0f, 0.55f, 0.2f, 0.85f, true);
}

void DrawGrunt(Px& p)
{
    p.FillPoly({{10, 12}, {24, 34}, {24, 10}}, 1.0f, 0.3f, 0.25f, 1.0f);
    p.FillPoly({{38, 12}, {24, 34}, {24, 10}}, 1.0f, 0.3f, 0.25f, 1.0f);
    p.FillPoly({{24, 42}, {16, 10}, {32, 10}}, 1.0f, 0.45f, 0.3f, 1.0f);
    p.FillCircle(24, 18, 4, 1.0f, 0.9f, 0.8f, 1.0f);
}

void DrawZigzag(Px& p)
{
    p.FillPoly({{24, 4}, {44, 24}, {24, 44}, {4, 24}}, 1.0f, 0.62f, 0.12f, 1.0f);
    p.FillPoly({{24, 12}, {36, 24}, {24, 36}, {12, 24}}, 1.0f, 0.85f, 0.4f, 1.0f);
    p.FillCircle(24, 24, 5, 1.0f, 1.0f, 0.9f, 1.0f);
}

void DrawSniper(Px& p)
{
    const Array<Math::Vec2> hex = {{28, 6}, {48, 18}, {48, 38}, {28, 50}, {8, 38}, {8, 18}};
    p.FillPoly(hex, 0.55f, 0.2f, 0.9f, 1.0f);
    const Array<Math::Vec2> hex2 = {{28, 12}, {42, 21}, {42, 35}, {28, 44}, {14, 35}, {14, 21}};
    p.FillPoly(hex2, 0.75f, 0.45f, 1.0f, 1.0f);
    p.FillCircle(28, 28, 8, 0.1f, 0.1f, 0.4f, 1.0f);
    p.FillCircle(28, 28, 3, 0.8f, 0.95f, 1.0f, 1.0f);
    p.FillCircle(28, 44, 9, 0.9f, 0.4f, 0.9f, 0.7f, true);
}

void DrawTank(Px& p)
{
    const Array<Math::Vec2> hex = {{36, 8}, {64, 28}, {64, 52}, {36, 72}, {8, 52}, {8, 28}};
    p.FillPoly(hex, 0.25f, 0.4f, 0.35f, 1.0f);
    const Array<Math::Vec2> hex2 = {{36, 16}, {58, 32}, {58, 48}, {36, 64}, {14, 48}, {14, 32}};
    p.FillPoly(hex2, 0.45f, 0.6f, 0.5f, 1.0f);
    p.FillCircle(36, 40, 10, 0.9f, 0.25f, 0.2f, 1.0f);
    p.FillCircle(36, 40, 5, 1.0f, 0.9f, 0.7f, 1.0f);
}

void DrawBoss(Px& p)
{
    f32 cx = (f32)p.W * 0.5f;
    p.FillPoly({{cx, 10}, {cx - 40, 60}, {cx - 34, 110}, {cx + 34, 110}, {cx + 40, 60}}, 0.15f, 0.3f, 0.65f, 1.0f);
    p.FillPoly({{cx, 10}, {cx - 22, 62}, {cx - 20, 104}, {cx + 20, 104}, {cx + 22, 62}}, 0.3f, 0.55f, 0.95f, 1.0f);
    p.FillPoly({{8, 52}, {cx - 20, 60}, {cx - 30, 92}, {4, 92}}, 0.35f, 0.2f, 0.7f, 1.0f);
    p.FillPoly({{(f32)p.W - 8, 52}, {cx + 20, 60}, {cx + 30, 92}, {(f32)p.W - 4, 92}}, 0.35f, 0.2f, 0.7f, 1.0f);
    p.FillCircle(cx, 44, 14, 0.05f, 0.08f, 0.3f, 1.0f);
    p.FillCircle(cx, 44, 6, 1.0f, 0.35f, 0.25f, 1.0f);
    p.FillCircle(cx - 22, 56, 6, 0.9f, 0.2f, 0.2f, 1.0f);
    p.FillCircle(cx + 22, 56, 6, 0.9f, 0.2f, 0.2f, 1.0f);
    p.FillCircle(cx, 96, 26, 0.8f, 0.35f, 0.8f, 0.6f, true);
    for (f32 i = 0; i < 5; i++)
    {
        f32 px = cx + (i - 2) * 26.0f;
        p.FillCircle(px, 104, 7, 0.2f, 0.7f, 0.9f, 1.0f);
    }
}

void DrawPowerup(Px& p)
{
    p.FillPoly({{20, 4}, {36, 20}, {20, 36}, {4, 20}}, 0.1f, 0.9f, 0.4f, 1.0f);
    p.FillPoly({{20, 9}, {31, 20}, {20, 31}, {9, 20}}, 0.6f, 1.0f, 0.8f, 1.0f);
    p.FillCircle(20, 20, 4, 1.0f, 1.0f, 1.0f, 1.0f);
}

}

GameTextures MakeGameTextures(RHI::RHIDevice* device)
{
    GameTextures t;
    {
        Px p(64, 64);
        DrawPlayer(p);
        p.ToTexture(device, t.Player);
    }
    {
        Px p(48, 48);
        DrawGrunt(p);
        p.ToTexture(device, t.Grunt);
    }
    {
        Px p(48, 48);
        DrawZigzag(p);
        p.ToTexture(device, t.Zigzag);
    }
    {
        Px p(56, 56);
        DrawSniper(p);
        p.ToTexture(device, t.Sniper);
    }
    {
        Px p(72, 72);
        DrawTank(p);
        p.ToTexture(device, t.Tank);
    }
    {
        Px p(192, 128);
        DrawBoss(p);
        t.Boss = Engine::MakeTexture(device, 192, 128, [&p](u32 x, u32 y, u8* rgba) {
            const u8* src = &p.Pixels[(y * 192 + x) * 4];
            rgba[0] = src[0];
            rgba[1] = src[1];
            rgba[2] = src[2];
            rgba[3] = src[3];
        });
    }
    {
        Px p(40, 40);
        DrawPowerup(p);
        p.ToTexture(device, t.Powerup);
    }
    t.BulletGlow = Engine::MakeGlowTexture(device, 64);
    return t;
}

}
