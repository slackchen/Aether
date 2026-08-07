#include "demo/flight_shmup/art.h"
#include "engine/texture.h"
#include <cmath>
#include <vector>

namespace shmup {

namespace {

struct Px {
    u32 w = 0;
    u32 h = 0;
    std::vector<u8> d;

    Px(u32 width, u32 height) : w(width), h(height), d((size_t)width * height * 4, 0) {}

    void blend(u32 x, u32 y, f32 r, f32 g, f32 b, f32 a) {
        if (x >= w || y >= h || a <= 0.0f) return;
        u8* p = &d[((size_t)y * w + x) * 4];
        f32 sa = a > 1.0f ? 1.0f : a;
        f32 da = p[3] / 255.0f;
        f32 oa = sa + da * (1.0f - sa);
        if (oa <= 0.0f) return;
        p[0] = (u8)(((r * sa + (p[0] / 255.0f) * da * (1.0f - sa)) / oa) * 255.0f);
        p[1] = (u8)(((g * sa + (p[1] / 255.0f) * da * (1.0f - sa)) / oa) * 255.0f);
        p[2] = (u8)(((b * sa + (p[2] / 255.0f) * da * (1.0f - sa)) / oa) * 255.0f);
        p[3] = (u8)(oa * 255.0f);
    }

    void fill_circle(f32 cx, f32 cy, f32 rad, f32 r, f32 g, f32 b, f32 a, bool glow = false) {
        i32 x0 = (i32)floorf(cx - rad), x1 = (i32)ceilf(cx + rad);
        i32 y0 = (i32)floorf(cy - rad), y1 = (i32)ceilf(cy + rad);
        for (i32 y = y0; y <= y1; y++) {
            for (i32 x = x0; x <= x1; x++) {
                f32 dx = (f32)x + 0.5f - cx;
                f32 dy = (f32)y + 0.5f - cy;
                f32 dist = sqrtf(dx * dx + dy * dy);
                if (dist <= rad) {
                    f32 alpha = a;
                    if (glow) alpha = a * (1.0f - dist / rad);
                    blend((u32)x, (u32)y, r, g, b, alpha);
                }
            }
        }
    }

    static bool in_poly(f32 px, f32 py, const std::vector<Vec2>& pts) {
        bool inside = false;
        size_t n = pts.size();
        for (size_t i = 0, j = n - 1; i < n; j = i++) {
            if (((pts[i].y > py) != (pts[j].y > py)) &&
                (px < (pts[j].x - pts[i].x) * (py - pts[i].y) / (pts[j].y - pts[i].y) + pts[i].x)) {
                inside = !inside;
            }
        }
        return inside;
    }

    void fill_poly(const std::vector<Vec2>& pts, f32 r, f32 g, f32 b, f32 a) {
        f32 minx = 1e9f, maxx = -1e9f, miny = 1e9f, maxy = -1e9f;
        for (const auto& p : pts) {
            minx = fminf(minx, p.x); maxx = fmaxf(maxx, p.x);
            miny = fminf(miny, p.y); maxy = fmaxf(maxy, p.y);
        }
        for (i32 y = (i32)floorf(miny); y <= (i32)ceilf(maxy); y++) {
            for (i32 x = (i32)floorf(minx); x <= (i32)ceilf(maxx); x++) {
                if (in_poly((f32)x + 0.5f, (f32)y + 0.5f, pts)) {
                    blend((u32)x, (u32)y, r, g, b, a);
                }
            }
        }
    }

    void to_texture(rhi::RHIDevice* device, std::shared_ptr<rhi::RHITexture>& out) {
        out = aether::engine::make_texture(device, w, h, [this](u32 x, u32 y, u8 rgba[4]) {
            const u8* src = &d[((size_t)y * w + x) * 4];
            rgba[0] = src[0];
            rgba[1] = src[1];
            rgba[2] = src[2];
            rgba[3] = src[3];
        });
    }
};

void draw_player(Px& p) {
    p.fill_poly({{14, 46}, {32, 26}, {32, 52}}, 0.05f, 0.4f, 0.9f, 1.0f);
    p.fill_poly({{50, 46}, {32, 26}, {32, 52}}, 0.05f, 0.4f, 0.9f, 1.0f);
    p.fill_poly({{32, 6}, {20, 44}, {44, 44}}, 0.25f, 0.85f, 1.0f, 1.0f);
    p.fill_poly({{32, 6}, {26, 40}, {38, 40}}, 0.9f, 0.97f, 1.0f, 1.0f);
    p.fill_circle(32, 33, 6, 0.08f, 0.18f, 0.5f, 1.0f);
    p.fill_circle(32, 33, 3, 0.85f, 0.95f, 1.0f, 1.0f);
    p.fill_circle(32, 53, 11, 1.0f, 0.55f, 0.2f, 0.85f, true);
}

void draw_grunt(Px& p) {
    p.fill_poly({{10, 12}, {24, 34}, {24, 10}}, 1.0f, 0.3f, 0.25f, 1.0f);
    p.fill_poly({{38, 12}, {24, 34}, {24, 10}}, 1.0f, 0.3f, 0.25f, 1.0f);
    p.fill_poly({{24, 42}, {16, 10}, {32, 10}}, 1.0f, 0.45f, 0.3f, 1.0f);
    p.fill_circle(24, 18, 4, 1.0f, 0.9f, 0.8f, 1.0f);
}

void draw_zigzag(Px& p) {
    p.fill_poly({{24, 4}, {44, 24}, {24, 44}, {4, 24}}, 1.0f, 0.62f, 0.12f, 1.0f);
    p.fill_poly({{24, 12}, {36, 24}, {24, 36}, {12, 24}}, 1.0f, 0.85f, 0.4f, 1.0f);
    p.fill_circle(24, 24, 5, 1.0f, 1.0f, 0.9f, 1.0f);
}

void draw_sniper(Px& p) {
    const std::vector<Vec2> hex = {{28, 6}, {48, 18}, {48, 38}, {28, 50}, {8, 38}, {8, 18}};
    p.fill_poly(hex, 0.55f, 0.2f, 0.9f, 1.0f);
    const std::vector<Vec2> hex2 = {{28, 12}, {42, 21}, {42, 35}, {28, 44}, {14, 35}, {14, 21}};
    p.fill_poly(hex2, 0.75f, 0.45f, 1.0f, 1.0f);
    p.fill_circle(28, 28, 8, 0.1f, 0.1f, 0.4f, 1.0f);
    p.fill_circle(28, 28, 3, 0.8f, 0.95f, 1.0f, 1.0f);
    p.fill_circle(28, 44, 9, 0.9f, 0.4f, 0.9f, 0.7f, true);
}

void draw_tank(Px& p) {
    const std::vector<Vec2> hex = {{36, 8}, {64, 28}, {64, 52}, {36, 72}, {8, 52}, {8, 28}};
    p.fill_poly(hex, 0.25f, 0.4f, 0.35f, 1.0f);
    const std::vector<Vec2> hex2 = {{36, 16}, {58, 32}, {58, 48}, {36, 64}, {14, 48}, {14, 32}};
    p.fill_poly(hex2, 0.45f, 0.6f, 0.5f, 1.0f);
    p.fill_circle(36, 40, 10, 0.9f, 0.25f, 0.2f, 1.0f);
    p.fill_circle(36, 40, 5, 1.0f, 0.9f, 0.7f, 1.0f);
}

void draw_boss(Px& p) {
    f32 cx = (f32)p.w * 0.5f;
    p.fill_poly({{cx, 10}, {cx - 40, 60}, {cx - 34, 110}, {cx + 34, 110}, {cx + 40, 60}}, 0.15f, 0.3f, 0.65f, 1.0f);
    p.fill_poly({{cx, 10}, {cx - 22, 62}, {cx - 20, 104}, {cx + 20, 104}, {cx + 22, 62}}, 0.3f, 0.55f, 0.95f, 1.0f);
    p.fill_poly({{8, 52}, {cx - 20, 60}, {cx - 30, 92}, {4, 92}}, 0.35f, 0.2f, 0.7f, 1.0f);
    p.fill_poly({{(f32)p.w - 8, 52}, {cx + 20, 60}, {cx + 30, 92}, {(f32)p.w - 4, 92}}, 0.35f, 0.2f, 0.7f, 1.0f);
    p.fill_circle(cx, 44, 14, 0.05f, 0.08f, 0.3f, 1.0f);
    p.fill_circle(cx, 44, 6, 1.0f, 0.35f, 0.25f, 1.0f);
    p.fill_circle(cx - 22, 56, 6, 0.9f, 0.2f, 0.2f, 1.0f);
    p.fill_circle(cx + 22, 56, 6, 0.9f, 0.2f, 0.2f, 1.0f);
    p.fill_circle(cx, 96, 26, 0.8f, 0.35f, 0.8f, 0.6f, true);
    for (f32 i = 0; i < 5; i++) {
        f32 px = cx + (i - 2) * 26.0f;
        p.fill_circle(px, 104, 7, 0.2f, 0.7f, 0.9f, 1.0f);
    }
}

void draw_powerup(Px& p) {
    p.fill_poly({{20, 4}, {36, 20}, {20, 36}, {4, 20}}, 0.1f, 0.9f, 0.4f, 1.0f);
    p.fill_poly({{20, 9}, {31, 20}, {20, 31}, {9, 20}}, 0.6f, 1.0f, 0.8f, 1.0f);
    p.fill_circle(20, 20, 4, 1.0f, 1.0f, 1.0f, 1.0f);
}

}

GameTextures make_game_textures(rhi::RHIDevice* device) {
    GameTextures t;
    {
        Px p(64, 64);
        draw_player(p);
        p.to_texture(device, t.player);
    }
    {
        Px p(48, 48);
        draw_grunt(p);
        p.to_texture(device, t.grunt);
    }
    {
        Px p(48, 48);
        draw_zigzag(p);
        p.to_texture(device, t.zigzag);
    }
    {
        Px p(56, 56);
        draw_sniper(p);
        p.to_texture(device, t.sniper);
    }
    {
        Px p(72, 72);
        draw_tank(p);
        p.to_texture(device, t.tank);
    }
    {
        Px p(192, 128);
        draw_boss(p);
        t.boss = aether::engine::make_texture(device, 192, 128, [&p](u32 x, u32 y, u8 rgba[4]) {
            const u8* src = &p.d[((size_t)y * 192 + x) * 4];
            rgba[0] = src[0];
            rgba[1] = src[1];
            rgba[2] = src[2];
            rgba[3] = src[3];
        });
    }
    {
        Px p(40, 40);
        draw_powerup(p);
        p.to_texture(device, t.powerup);
    }
    t.bullet_glow = aether::engine::make_glow_texture(device, 64);
    return t;
}

}
