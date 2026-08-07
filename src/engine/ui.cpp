#include "engine/ui.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/em_js.h>
#else
#define NOMINMAX
#include <windows.h>
#include "engine/renderer2d.h"
#include "engine/texture.h"
#include "platform/platform.h"
#include <cmath>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#endif

namespace aether::engine::ui {

namespace {

#ifdef __EMSCRIPTEN__

EM_JS(void, js_ui_init, (), {
    if (Module.__aether_ui) return;
    var U = {
        elements: {},
        flashTimer: null
    };
    Module.__aether_ui = U;

    U.el = function(id) {
        var el = U.elements[id];
        if (!el) {
            el = document.getElementById(id);
            U.elements[id] = el;
        }
        return el;
    };

    U.set = function(id, text) {
        var el = U.el(id);
        if (el) el.textContent = String(text);
    };

    U.visible = function(id, show) {
        var el = U.el(id);
        if (!el) return;
        if (el.__aetherDisplay === undefined) {
            el.__aetherDisplay = el.getAttribute('data-display') || 'block';
        }
        el.style.display = show ? el.__aetherDisplay : 'none';
    };

    U.fill = function(id, ratio) {
        var el = U.el(id);
        if (el) el.style.width = (ratio * 100).toFixed(1) + "%";
    };

    U.flash = function(text, dur) {
        var el = U.el("flash");
        if (!el) return;
        el.textContent = text;
        el.style.opacity = "1";
        el.style.display = "block";
        clearTimeout(U.flashTimer);
        U.flashTimer = setTimeout(function() {
            el.style.opacity = "0";
            setTimeout(function() { el.style.display = "none"; }, 500);
        }, (dur || 1.5) * 1000);
    };
});

EM_JS(void, js_ui_set_text, (const char* element, const char* text), {
    Module.__aether_ui.set(UTF8ToString(element), UTF8ToString(text));
});
EM_JS(void, js_ui_set_visible, (const char* element, int visible), {
    Module.__aether_ui.visible(UTF8ToString(element), visible ? true : false);
});
EM_JS(void, js_ui_set_fill, (const char* element, double ratio), {
    Module.__aether_ui.fill(UTF8ToString(element), ratio);
});
EM_JS(void, js_ui_flash, (const char* text, double duration), {
    Module.__aether_ui.flash(UTF8ToString(text), duration);
});

class DomBackend : public Backend {
public:
    void set_text(const char* element, const char* text) override {
        js_ui_set_text(element, text);
    }
    void set_visible(const char* element, bool visible) override {
        js_ui_set_visible(element, visible ? 1 : 0);
    }
    void set_fill(const char* element, f32 ratio) override {
        js_ui_set_fill(element, (double)ratio);
    }
    void flash(const char* text, f32 duration) override {
        js_ui_flash(text, (double)duration);
    }
};

DomBackend s_dom_backend;

#else

constexpr u32 kAtlasCell = 48;
constexpr u32 kAtlasCols = 16;
constexpr u32 kAtlasRows = 6;
constexpr u32 kAtlasWidth = kAtlasCols * kAtlasCell;
constexpr u32 kAtlasHeight = kAtlasRows * kAtlasCell;
constexpr u32 kAtlasFontPx = 36;

constexpr i32 kLayerHud = 20;
constexpr i32 kLayerBoss = 21;
constexpr i32 kLayerPanel = 30;
constexpr i32 kLayerFlash = 35;

struct GlyphInfo {
    u32 uv_x = 0;
    u32 uv_y = 0;
    u32 advance = 0;
};

class NativeBackend : public Backend {
public:
    void set_text(const char* element, const char* text) override {
        text_map_[element] = text;
    }
    void set_visible(const char* element, bool visible) override {
        visible_map_[element] = visible;
    }
    void set_fill(const char* element, f32 ratio) override {
        fill_map_[element] = ratio;
    }
    void flash(const char* text, f32 duration) override {
        flash_text_ = text;
        flash_start_ = aether::platform::now_seconds();
        flash_duration_ = duration > 0.0f ? duration : 1.5f;
    }

    void draw(aether::engine::Renderer2D* renderer) override {
        if (!renderer || !renderer->device()) return;
        ensure_assets(renderer);
        if (!atlas_) return;

        auto& batch = renderer->sprites();
        f32 half_h = 360.0f;
        f32 half_w = half_h * renderer->aspect();
        Vec2 cam = renderer->camera().position;
        auto sx = [&](f32 x) { return (x - half_w) + cam.x; };
        auto sy = [&](f32 y) { return (y - half_h) + cam.y; };

        auto draw_text = [&](const char* text, f32 x, f32 y, f32 px, f32 spacing,
                             const Color& col, i32 layer, bool centered = true, f32 glow = 0.0f) {
            draw_text_impl(batch, text, sx(x), sy(y), px, spacing, col, layer, centered, glow);
        };
        auto rect = [&](f32 x, f32 y, f32 w, f32 h, const Color& col, i32 layer,
                        rhi::BlendMode blend = rhi::BlendMode::Alpha) {
            if (w <= 0.0f || h <= 0.0f) return;
            batch.add(solid_, {sx(x) + w * 0.5f, sy(y) + h * 0.5f}, {w, h}, col, 0.0f, layer, blend);
        };
        auto overlay = [&]() {
            rect(0.0f, 0.0f, half_w * 2.0f, half_h * 2.0f, color(0.0f, 0.0f, 0.0f, 0.8f), kLayerPanel);
            batch.add(glow_, {sx(half_w), sy(half_h)}, {half_w * 2.6f, half_w * 2.6f},
                      color(0.04f, 0.10f, 0.28f, 0.6f), 0.0f, kLayerPanel);
        };

        const char* label = "SCORE";
        const char* key = "hud-score";

        struct HudGroup {
            const char* label;
            const char* key;
        };
        const HudGroup groups[] = {
            {"SCORE", "hud-score"},
            {"HI-SCORE", "hud-hiscore"},
            {"LIVES", "hud-lives"},
            {"WEAPON", "hud-weapon"},
        };
        f32 gx = 20.0f;
        for (const auto& g : groups) {
            const char* value = get_text(g.key, "0");
            f32 gw = measure(g.label, 12.0f, 0.0f);
            f32 vw = measure(value, 26.0f, 0.0f);
            if (vw > gw) gw = vw;
            draw_text(g.label, gx, 14.0f, 12.0f, 0.0f, color(0.50f, 0.72f, 0.91f, 1.0f),
                      kLayerHud, false, 0.25f);
            draw_text(value, gx, 36.0f, 26.0f, 0.0f, color(0.81f, 0.91f, 1.0f, 1.0f),
                      kLayerHud, false, 0.35f);
            gx += gw + 28.0f;
        }

        if (visible("hud-boss", false)) {
            const char* name = get_text("boss-name", "BOSS");
            draw_text(name, half_w, 12.0f, 14.0f, 4.0f, color(1.0f, 0.82f, 0.82f, 1.0f),
                      kLayerBoss, true, 0.6f);
            f32 bar_x = half_w - 280.0f;
            f32 bar_y = 36.0f;
            f32 bar_w = 560.0f;
            f32 bar_h = 14.0f;
            f32 ratio = fill("boss-fill", 1.0f);
            if (ratio < 0.0f) ratio = 0.0f;
            if (ratio > 1.0f) ratio = 1.0f;
            rect(bar_x, bar_y, bar_w, bar_h, color(0.16f, 0.0f, 0.0f, 0.7f), kLayerBoss);
            rect(bar_x, bar_y, bar_w, 2.0f, color(1.0f, 0.31f, 0.31f, 0.8f), kLayerBoss);
            rect(bar_x, bar_y + bar_h - 2.0f, bar_w, 2.0f, color(1.0f, 0.31f, 0.31f, 0.8f), kLayerBoss);
            rect(bar_x, bar_y, 2.0f, bar_h, color(1.0f, 0.31f, 0.31f, 0.8f), kLayerBoss);
            rect(bar_x + bar_w - 2.0f, bar_y, 2.0f, bar_h, color(1.0f, 0.31f, 0.31f, 0.8f), kLayerBoss);
            f32 fill_w = (bar_w - 4.0f) * ratio;
            if (fill_w > 0.0f) {
                rect(bar_x + 2.0f, bar_y + 2.0f, fill_w, bar_h - 4.0f, color(0.95f, 0.28f, 0.16f, 1.0f), kLayerBoss);
                rect(bar_x + 2.0f, bar_y + 2.0f, fill_w, bar_h - 4.0f, color(1.0f, 0.48f, 0.43f, 0.25f), kLayerBoss, rhi::BlendMode::Additive);
            }
        }

        if (visible("panel-title", true)) {
            overlay();
            draw_text("AETHER STRIKE", half_w, 250.0f, 64.0f, 10.0f, color(0.75f, 0.90f, 1.0f, 1.0f),
                      kLayerPanel, true, 0.6f);
            draw_text("VECTOR ASSAULT PROTOCOL", half_w, 330.0f, 18.0f, 3.0f,
                      color(0.62f, 0.83f, 1.0f, 1.0f), kLayerPanel, true, 0.3f);
            draw_text("MOVE: W A S D / ARROWS", half_w, 395.0f, 15.0f, 1.0f,
                      color(0.44f, 0.63f, 0.78f, 1.0f), kLayerPanel);
            draw_text("FIRE: J / SPACE   SLOW: SHIFT   PAUSE: P", half_w, 425.0f, 15.0f, 1.0f,
                      color(0.44f, 0.63f, 0.78f, 1.0f), kLayerPanel);
            draw_text("GAMEPAD: STICK / D-PAD MOVE   RT FIRE   LT SLOW   START PAUSE", half_w, 455.0f,
                      15.0f, 1.0f, color(0.44f, 0.63f, 0.78f, 1.0f), kLayerPanel);
            const char* btn = "PRESS ENTER TO START";
            f32 pulse = 1.0f + 0.06f * sinf((f32)aether::platform::now_seconds() * 3.93f);
            f32 bw = measure(btn, 18.0f, 3.0f) * pulse + 68.0f;
            f32 bh = 40.0f * pulse;
            rect(half_w - bw * 0.5f, 505.0f - bh * 0.5f, bw, bh, color(0.75f, 0.90f, 1.0f, 0.9f), kLayerPanel);
            rect(half_w - bw * 0.5f, 505.0f - bh * 0.5f, bw, bh, color(0.0f, 0.78f, 1.0f, 0.3f), kLayerPanel, rhi::BlendMode::Additive);
            draw_text(btn, half_w, 505.0f, 18.0f, 3.0f, color(0.04f, 0.08f, 0.13f, 1.0f), kLayerPanel, true);
        }
        if (visible("panel-pause", false)) {
            overlay();
            draw_text("PAUSED", half_w, 290.0f, 46.0f, 8.0f, color(1.0f, 0.85f, 0.63f, 1.0f),
                      kLayerPanel, true, 0.5f);
            draw_text("PRESS P TO RESUME", half_w, 370.0f, 18.0f, 3.0f,
                      color(0.62f, 0.83f, 1.0f, 1.0f), kLayerPanel);
        }
        if (visible("panel-gameover", false)) {
            overlay();
            draw_text("GAME OVER", half_w, 290.0f, 46.0f, 8.0f, color(1.0f, 0.85f, 0.63f, 1.0f),
                      kLayerPanel, true, 0.5f);
            draw_text(get_text("gameover-score", "SCORE 0"), half_w, 370.0f, 20.0f, 2.0f,
                      color(0.91f, 0.96f, 1.0f, 1.0f), kLayerPanel, true, 0.3f);
            draw_text("PRESS ENTER TO RETRY", half_w, 430.0f, 15.0f, 1.0f,
                      color(0.44f, 0.63f, 0.78f, 1.0f), kLayerPanel);
        }
        if (visible("panel-win", false)) {
            overlay();
            draw_text("MISSION COMPLETE", half_w, 290.0f, 46.0f, 8.0f, color(1.0f, 0.85f, 0.63f, 1.0f),
                      kLayerPanel, true, 0.5f);
            draw_text(get_text("win-score", "SCORE 0"), half_w, 370.0f, 20.0f, 2.0f,
                      color(0.91f, 0.96f, 1.0f, 1.0f), kLayerPanel, true, 0.3f);
            draw_text("PRESS ENTER TO PLAY AGAIN", half_w, 430.0f, 15.0f, 1.0f,
                      color(0.44f, 0.63f, 0.78f, 1.0f), kLayerPanel);
        }

        if (!flash_text_.empty()) {
            double now = aether::platform::now_seconds();
            double fade = now - flash_start_;
            double total = flash_duration_ + 0.5;
            if (fade < total) {
                f32 alpha = fade < flash_duration_
                                ? 1.0f
                                : (f32)(1.0 - (fade - flash_duration_) / 0.5);
                const char* flash = flash_text_.c_str();
                draw_text(flash, half_w, 274.0f, 42.0f, 8.0f, color(1.0f, 0.86f, 0.47f, alpha * 0.8f),
                          kLayerFlash, true, 0.0f);
                draw_text(flash, half_w, 274.0f, 42.0f, 8.0f, color(1.0f, 1.0f, 1.0f, alpha),
                          kLayerFlash, true, 0.0f);
            } else {
                flash_text_.clear();
            }
        }
    }

private:
    const char* get_text(const char* key, const char* fallback) {
        auto it = text_map_.find(key);
        return it != text_map_.end() ? it->second.c_str() : fallback;
    }
    bool visible(const char* key, bool fallback) {
        auto it = visible_map_.find(key);
        return it != visible_map_.end() ? it->second : fallback;
    }
    f32 fill(const char* key, f32 fallback) {
        auto it = fill_map_.find(key);
        return it != fill_map_.end() ? it->second : fallback;
    }

    void ensure_assets(aether::engine::Renderer2D* renderer) {
        if (atlas_) return;
        build_atlas(renderer->device());
        solid_ = aether::engine::make_solid_texture(renderer->device(), 4, 4, {1.0f, 1.0f, 1.0f, 1.0f});
        glow_ = aether::engine::make_glow_texture(renderer->device(), 256);
    }

    bool build_atlas(rhi::RHIDevice* device) {
        HDC mem = CreateCompatibleDC(nullptr);
        if (!mem) return false;

        BITMAPINFO bi = {};
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = (LONG)kAtlasWidth;
        bi.bmiHeader.biHeight = -(LONG)kAtlasHeight;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;

        void* bits = nullptr;
        HBITMAP dib = CreateDIBSection(mem, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
        if (!dib) {
            DeleteDC(mem);
            return false;
        }
        HGDIOBJ old = SelectObject(mem, dib);

        HBRUSH bg = CreateSolidBrush(RGB(0, 0, 0));
        RECT full = {0, 0, (LONG)kAtlasWidth, (LONG)kAtlasHeight};
        FillRect(mem, &full, bg);
        DeleteObject(bg);

        HFONT font = CreateFontA(-(LONG)kAtlasFontPx, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 ANTIALIASED_QUALITY, DEFAULT_PITCH, "Segoe UI");
        SelectObject(mem, font);

        SetBkMode(mem, TRANSPARENT);
        SetTextAlign(mem, TA_TOP | TA_LEFT);
        LONG cell_pad = (LONG)(kAtlasCell - kAtlasFontPx) / 2;

        for (u32 i = 0; i < 95; i++) {
            u32 c = i + 32;
            u32 col = i % kAtlasCols;
            u32 row = i / kAtlasCols;
            char ch[2] = {(char)c, 0};
            SetTextColor(mem, RGB(255, 255, 255));
            TextOutA(mem, (LONG)(col * kAtlasCell) + 2, (LONG)(row * kAtlasCell) + cell_pad, ch, 1);
            SIZE s = {};
            GetTextExtentPoint32A(mem, ch, 1, &s);
            glyphs_[i].uv_x = col * kAtlasCell;
            glyphs_[i].uv_y = row * kAtlasCell;
            glyphs_[i].advance = (u32)s.cx + 1;
        }

        rgba_.resize(kAtlasWidth * kAtlasHeight * 4);
        const u8* bgra = (const u8*)bits;
        for (u32 y = 0; y < kAtlasHeight; y++) {
            for (u32 x = 0; x < kAtlasWidth; x++) {
                u32 idx = (y * kAtlasWidth + x) * 4;
                u8 lum = bgra[idx + 1];
                rgba_[idx + 0] = 255;
                rgba_[idx + 1] = 255;
                rgba_[idx + 2] = 255;
                rgba_[idx + 3] = lum;
            }
        }

        atlas_ = aether::engine::make_texture(device, kAtlasWidth, kAtlasHeight,
                                              [this](u32 x, u32 y, u8 out[4]) {
                                                  u32 idx = (y * kAtlasWidth + x) * 4;
                                                  out[0] = rgba_[idx + 0];
                                                  out[1] = rgba_[idx + 1];
                                                  out[2] = rgba_[idx + 2];
                                                  out[3] = rgba_[idx + 3];
                                              });

        DeleteObject(font);
        SelectObject(mem, old);
        DeleteObject(dib);
        DeleteDC(mem);
        return atlas_ != nullptr;
    }

    f32 measure(const char* text, f32 px, f32 spacing) {
        if (!atlas_) return 0.0f;
        f32 scale = px / (f32)kAtlasFontPx;
        f32 total = 0.0f;
        u32 n = 0;
        for (const char* p = text; *p; p++) {
            u32 c = (u8)*p;
            if (c < 32 || c > 126) c = '?';
            total += (f32)glyphs_[c - 32].advance;
            n++;
        }
        if (n > 1) total += spacing * (f32)(n - 1);
        return total * scale;
    }

    void draw_text_impl(aether::engine::SpriteBatch& batch, const char* text, f32 world_x, f32 world_y,
                        f32 px, f32 spacing, const Color& col, i32 layer, bool centered, f32 glow) {
        f32 scale = px / (f32)kAtlasFontPx;
        f32 total = measure(text, px, spacing);
        f32 x = centered ? world_x - total * 0.5f : world_x;
        f32 y = world_y;
        f32 pad = (f32)(kAtlasCell - kAtlasFontPx) / 2.0f;

        for (const char* p = text; *p; p++) {
            u32 c = (u8)*p;
            if (c < 32 || c > 126) c = '?';
            const GlyphInfo& g = glyphs_[c - 32];
            Vec2 uv0{(f32)g.uv_x / (f32)kAtlasWidth,
                     ((f32)g.uv_y + pad) / (f32)kAtlasHeight};
            Vec2 uv1{uv0.x + (f32)g.advance / (f32)kAtlasWidth,
                     uv0.y + (f32)kAtlasFontPx / (f32)kAtlasHeight};
            Vec2 center{x + (f32)g.advance * scale * 0.5f, y + (f32)kAtlasFontPx * scale * 0.5f};
            Vec2 size{(f32)g.advance * scale, (f32)kAtlasFontPx * scale};
            if (glow > 0.0f) {
                batch.add_uv(atlas_, uv0, uv1, center, size, color(col.r, col.g, col.b, col.a * glow),
                             0.0f, layer, rhi::BlendMode::Additive);
            }
            batch.add_uv(atlas_, uv0, uv1, center, size, col, 0.0f, layer);
            x += ((f32)g.advance + spacing) * scale;
        }
    }

    std::map<std::string, std::string> text_map_;
    std::map<std::string, bool> visible_map_;
    std::map<std::string, f32> fill_map_;
    std::string flash_text_;
    double flash_start_ = 0.0;
    f32 flash_duration_ = 1.5f;

    std::shared_ptr<rhi::RHITexture> atlas_;
    std::shared_ptr<rhi::RHITexture> solid_;
    std::shared_ptr<rhi::RHITexture> glow_;
    GlyphInfo glyphs_[95] = {};
    std::vector<u8> rgba_;
};

NativeBackend s_native_backend;

#endif

}  // namespace

Backend* g_backend = nullptr;

void init() {
#ifdef __EMSCRIPTEN__
    js_ui_init();
    if (!g_backend) g_backend = &s_dom_backend;
#else
    if (!g_backend) g_backend = &s_native_backend;
#endif
}

void set_backend(Backend* backend) {
    g_backend = backend;
}

void set_text(const char* element, const char* text) {
    if (g_backend) g_backend->set_text(element, text);
}

void set_visible(const char* element, bool visible) {
    if (g_backend) g_backend->set_visible(element, visible);
}

void set_fill(const char* element, f32 ratio) {
    if (g_backend) g_backend->set_fill(element, ratio);
}

void flash(const char* text, f32 duration) {
    if (g_backend) g_backend->flash(text, duration);
}

void draw(Renderer2D* renderer) {
    if (g_backend) g_backend->draw(renderer);
}

}
