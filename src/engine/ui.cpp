#include "engine/ui.h"
#include "engine/input.h"
#include "engine/renderer2d.h"
#include "engine/texture.h"
#include "platform/platform.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/em_js.h>
#else
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#endif

namespace aether::engine::ui {

namespace {

Theme g_theme;
Renderer2D* g_current_renderer = nullptr;

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

constexpr u32 kAtlasCell = 44;
constexpr u32 kAtlasCols = 32;
constexpr u32 kAtlasRows = 32;
constexpr u32 kAtlasWidth = kAtlasCols * kAtlasCell;
constexpr u32 kAtlasHeight = kAtlasRows * kAtlasCell;
constexpr u32 kAtlasFontPx = 30;
constexpr u32 kAtlasPad = (kAtlasCell - kAtlasFontPx) / 2;

constexpr i32 kLayerFlash = 38;

struct GlyphInfo {
    u32 uv_x = 0;
    u32 uv_y = 0;
    u32 bb_x = 0;
    u32 bb_y = 0;
    u32 bb_w = 0;
    u32 bb_h = 0;
    u32 advance = 0;
};

class NativeBackend : public Backend {
public:
    NativeBackend() = default;
    ~NativeBackend() {
        if (font_) DeleteObject(font_);
        if (dib_) DeleteObject(dib_);
        if (mem_dc_) DeleteDC(mem_dc_);
    }

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

    const char* get_text(const char* key, const char* fallback) override {
        auto it = text_map_.find(key);
        return it != text_map_.end() ? it->second.c_str() : fallback;
    }
    bool is_visible(const char* key, bool fallback) override {
        auto it = visible_map_.find(key);
        return it != visible_map_.end() ? it->second : fallback;
    }
    f32 get_fill(const char* key, f32 fallback) override {
        auto it = fill_map_.find(key);
        return it != fill_map_.end() ? it->second : fallback;
    }

    void draw(Renderer2D* renderer) override {
        if (!renderer || !renderer->device()) return;
        ensure_assets(renderer);
        if (!atlas_) return;

        if (!flash_text_.empty()) {
            double now = aether::platform::now_seconds();
            double fade = now - flash_start_;
            double total = flash_duration_ + 0.5;
            if (fade < total) {
                f32 alpha = fade < flash_duration_
                                ? 1.0f
                                : (f32)(1.0 - (fade - flash_duration_) / 0.5);
                const char* flash = flash_text_.c_str();
                f32 half_w = 360.0f * renderer->aspect();
                draw_text_screen(renderer, flash, half_w, 274.0f, 38.0f, 6.0f, color(1.0f, 0.86f, 0.47f, alpha * 0.8f),
                                 kLayerFlash, true, 0.4f);
                draw_text_screen(renderer, flash, half_w, 274.0f, 38.0f, 6.0f, color(1.0f, 1.0f, 1.0f, alpha),
                                 kLayerFlash, true, 0.0f);
            } else {
                flash_text_.clear();
            }
        }
    }

    void draw_rect_screen(Renderer2D* renderer, const Rect& r, const Color& col, i32 layer,
                          rhi::BlendMode blend = rhi::BlendMode::Alpha) {
        if (!renderer || !renderer->device() || r.w <= 0.0f || r.h <= 0.0f) return;
        ensure_assets(renderer);
        if (!solid_) return;

        f32 half_h = 360.0f;
        f32 half_w = half_h * renderer->aspect();
        Vec2 cam = renderer->camera().position;
        f32 sx = (r.x - half_w) + cam.x + r.w * 0.5f;
        f32 sy = (r.y - half_h) + cam.y + r.h * 0.5f;

        renderer->sprites().add(solid_, {sx, sy}, {r.w, r.h}, col, 0.0f, layer, blend);
    }

    void draw_glow_screen(Renderer2D* renderer, const Rect& r, const Color& col, i32 layer) {
        if (!renderer || !renderer->device() || r.w <= 0.0f || r.h <= 0.0f) return;
        ensure_assets(renderer);
        if (!glow_) return;

        f32 half_h = 360.0f;
        f32 half_w = half_h * renderer->aspect();
        Vec2 cam = renderer->camera().position;
        f32 sx = (r.x - half_w) + cam.x + r.w * 0.5f;
        f32 sy = (r.y - half_h) + cam.y + r.h * 0.5f;

        renderer->sprites().add(glow_, {sx, sy}, {r.w * 1.5f, r.h * 1.5f}, col, 0.0f, layer, rhi::BlendMode::Additive);
    }

    void ensure_glyph(rhi::RHIDevice* device, wchar_t wc) {
        if (glyph_map_.find(wc) != glyph_map_.end()) return;
        if (!mem_dc_ || next_slot_ >= kAtlasCols * kAtlasRows) return;

        u32 slot = next_slot_++;
        u32 col = slot % kAtlasCols;
        u32 row = slot / kAtlasCols;
        u32 x0 = col * kAtlasCell;
        u32 y0 = row * kAtlasCell;

        wchar_t str[2] = {wc, 0};
        SetTextColor(mem_dc_, RGB(255, 255, 255));
        TextOutW(mem_dc_, (LONG)x0 + 2, (LONG)y0 + kAtlasPad, str, 1);

        SIZE s = {};
        GetTextExtentPoint32W(mem_dc_, str, 1, &s);

        const u8* bgra = (const u8*)dib_bits_;
        u32 min_x = kAtlasCell, max_x = 0, min_y = kAtlasCell, max_y = 0;
        for (u32 yy = 0; yy < kAtlasCell; yy++) {
            for (u32 xx = 0; xx < kAtlasCell; xx++) {
                u32 idx = ((y0 + yy) * kAtlasWidth + (x0 + xx)) * 4;
                u8 lum = bgra[idx + 1];
                rgba_[idx + 0] = 255;
                rgba_[idx + 1] = 255;
                rgba_[idx + 2] = 255;
                rgba_[idx + 3] = lum;
                if (lum > 0) {
                    if (xx < min_x) min_x = xx;
                    if (xx > max_x) max_x = xx;
                    if (yy < min_y) min_y = yy;
                    if (yy > max_y) max_y = yy;
                }
            }
        }

        GlyphInfo g;
        g.uv_x = x0;
        g.uv_y = y0;
        g.advance = (u32)s.cx + 1;
        if (max_x >= min_x && max_y >= min_y) {
            g.bb_x = min_x;
            g.bb_y = min_y;
            g.bb_w = max_x - min_x + 1;
            g.bb_h = max_y - min_y + 1;
        }
        glyph_map_[wc] = g;
        atlas_dirty_ = true;
    }

    std::wstring to_wstring(const char* text) {
        if (!text || !*text) return L"";
        int len = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
        if (len <= 1) return L"";
        std::wstring wstr(len - 1, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, text, -1, &wstr[0], len);
        return wstr;
    }

    void draw_text_screen(Renderer2D* renderer, const char* text, f32 screen_x, f32 screen_y,
                          f32 px, f32 spacing, const Color& col, i32 layer, bool centered, f32 glow) {
        if (!renderer || !renderer->device() || !text || !*text) return;
        ensure_assets(renderer);

        std::wstring wtext = to_wstring(text);
        if (wtext.empty()) return;

        for (wchar_t wc : wtext) {
            ensure_glyph(renderer->device(), wc);
        }

        if (atlas_dirty_) {
            atlas_ = make_texture(renderer->device(), kAtlasWidth, kAtlasHeight,
                                  [this](u32 x, u32 y, u8 out[4]) {
                                      u32 idx = (y * kAtlasWidth + x) * 4;
                                      out[0] = rgba_[idx + 0];
                                      out[1] = rgba_[idx + 1];
                                      out[2] = rgba_[idx + 2];
                                      out[3] = rgba_[idx + 3];
                                  });
            atlas_dirty_ = false;
        }

        if (!atlas_) return;

        f32 half_h = 360.0f;
        f32 half_w = half_h * renderer->aspect();
        Vec2 cam = renderer->camera().position;
        f32 world_x = (screen_x - half_w) + cam.x;
        f32 world_y = (screen_y - half_h) + cam.y;

        f32 scale = px / (f32)kAtlasFontPx;
        f32 total = measure(text, px, spacing);
        f32 x = centered ? world_x - total * 0.5f : world_x;
        f32 y = world_y;

        f32 ink_top = 1e30f, ink_bottom = -1e30f;
        for (wchar_t wc : wtext) {
            auto it = glyph_map_.find(wc);
            if (it == glyph_map_.end()) continue;
            const GlyphInfo& g = it->second;
            if (g.bb_w == 0 || g.bb_h == 0) continue;
            if ((f32)g.bb_y < ink_top) ink_top = (f32)g.bb_y;
            if ((f32)(g.bb_y + g.bb_h) > ink_bottom) ink_bottom = (f32)(g.bb_y + g.bb_h);
        }
        f32 ink_center = (ink_top + ink_bottom) * 0.5f;

        for (wchar_t wc : wtext) {
            auto it = glyph_map_.find(wc);
            if (it != glyph_map_.end()) {
                const GlyphInfo& g = it->second;
                if (g.bb_w > 0 && g.bb_h > 0) {
                    f32 x_mid_orig = (f32)g.bb_x + (f32)g.bb_w * 0.5f;
                    f32 y_mid_orig = (f32)g.bb_y + (f32)g.bb_h * 0.5f;
                    f32 cx = x + x_mid_orig * scale;
                    f32 cy = y + (y_mid_orig - ink_center) * scale;
                    f32 w = (f32)g.bb_w * scale;
                    f32 h = (f32)g.bb_h * scale;

                    f32 u0 = (f32)(g.uv_x + g.bb_x) / (f32)kAtlasWidth;
                    f32 v0 = (f32)(g.uv_y + g.bb_y) / (f32)kAtlasHeight;
                    f32 u1 = u0 + (f32)g.bb_w / (f32)kAtlasWidth;
                    f32 v1 = v0 + (f32)g.bb_h / (f32)kAtlasHeight;

                    if (glow > 0.0f && glow_) {
                        Color glow_col = col;
                        glow_col.a = glow * col.a * 0.4f;
                        renderer->sprites().add_uv(atlas_, {u0, v0}, {u1, v1}, {cx, cy}, {w * 1.3f, h * 1.3f},
                                                   glow_col, 0.0f, layer - 1, rhi::BlendMode::Additive);
                    }

                    renderer->sprites().add_uv(atlas_, {u0, v0}, {u1, v1}, {cx, cy}, {w, h},
                                               col, 0.0f, layer, rhi::BlendMode::Alpha);
                }
                x += (f32)g.advance * scale + spacing;
            } else {
                x += px * 0.6f + spacing;
            }
        }
    }

    f32 measure(const char* text, f32 px, f32 spacing) {
        std::wstring wtext = to_wstring(text);
        if (wtext.empty()) return 0.0f;

        f32 scale = px / (f32)kAtlasFontPx;
        f32 total = 0.0f;
        u32 n = 0;
        for (wchar_t wc : wtext) {
            auto it = glyph_map_.find(wc);
            if (it != glyph_map_.end()) {
                total += (f32)it->second.advance * scale;
            } else {
                total += (wc > 128 ? px * 0.95f : px * 0.55f);
            }
            n++;
        }
        if (n > 1) total += spacing * (f32)(n - 1);
        return total;
    }

private:
    void ensure_assets(Renderer2D* renderer) {
        if (mem_dc_) return;
        build_atlas(renderer->device());
        solid_ = make_solid_texture(renderer->device(), 4, 4, {1.0f, 1.0f, 1.0f, 1.0f});
        glow_ = make_glow_texture(renderer->device(), 256);
    }

    bool build_atlas(rhi::RHIDevice* device) {
        build_in_progress_ = true;
        mem_dc_ = CreateCompatibleDC(nullptr);
        if (!mem_dc_) {
            build_in_progress_ = false;
            return false;
        }

        BITMAPINFO bi = {};
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = (LONG)kAtlasWidth;
        bi.bmiHeader.biHeight = -(LONG)kAtlasHeight;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;

        dib_ = CreateDIBSection(mem_dc_, &bi, DIB_RGB_COLORS, &dib_bits_, nullptr, 0);
        if (!dib_) {
            DeleteDC(mem_dc_);
            mem_dc_ = nullptr;
            build_in_progress_ = false;
            return false;
        }
        SelectObject(mem_dc_, dib_);

        HBRUSH bg = CreateSolidBrush(RGB(0, 0, 0));
        RECT full = {0, 0, (LONG)kAtlasWidth, (LONG)kAtlasHeight};
        FillRect(mem_dc_, &full, bg);
        DeleteObject(bg);

        font_ = CreateFontW(-(LONG)kAtlasFontPx, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei");
        SelectObject(mem_dc_, font_);

        SetBkMode(mem_dc_, TRANSPARENT);
        SetTextAlign(mem_dc_, TA_TOP | TA_LEFT);

        rgba_.assign(kAtlasWidth * kAtlasHeight * 4, 0);

        // Pre-populate ASCII printable range (32..126)
        for (u32 i = 0; i < 95; i++) {
            ensure_glyph(device, (wchar_t)(i + 32));
        }

        atlas_ = make_texture(device, kAtlasWidth, kAtlasHeight,
                              [this](u32 x, u32 y, u8 out[4]) {
                                  u32 idx = (y * kAtlasWidth + x) * 4;
                                  out[0] = rgba_[idx + 0];
                                  out[1] = rgba_[idx + 1];
                                  out[2] = rgba_[idx + 2];
                                  out[3] = rgba_[idx + 3];
                              });

        atlas_dirty_ = false;
        build_in_progress_ = false;
        return atlas_ != nullptr;
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
    std::vector<u8> rgba_;
    std::map<wchar_t, GlyphInfo> glyph_map_;
    u32 next_slot_ = 0;
    bool atlas_dirty_ = false;
    bool build_in_progress_ = false;

    HDC mem_dc_ = nullptr;
    HBITMAP dib_ = nullptr;
    HFONT font_ = nullptr;
    void* dib_bits_ = nullptr;
};

NativeBackend s_native_backend;

#endif

Backend* s_backend = nullptr;

Backend* active_backend() {
    if (s_backend) return s_backend;
#ifdef __EMSCRIPTEN__
    return &s_dom_backend;
#else
    return &s_native_backend;
#endif
}

} // namespace

void init() {
#ifdef __EMSCRIPTEN__
    js_ui_init();
#endif
}

void set_backend(Backend* backend) {
    s_backend = backend;
}

Theme& get_theme() {
    return g_theme;
}

void begin_frame(Renderer2D* renderer) {
    g_current_renderer = renderer;
}

void end_frame() {
    g_current_renderer = nullptr;
}

void draw_rect(const Rect& r, const Color& col, i32 layer, rhi::BlendMode blend) {
#ifndef __EMSCRIPTEN__
    s_native_backend.draw_rect_screen(g_current_renderer, r, col, layer, blend);
#endif
}

void draw_rect_outline(const Rect& r, f32 thickness, const Color& col, i32 layer) {
#ifndef __EMSCRIPTEN__
    if (r.w <= 0.0f || r.h <= 0.0f || thickness <= 0.0f) return;
    // Top, bottom, left, right borders
    draw_rect({r.x, r.y, r.w, thickness}, col, layer);
    draw_rect({r.x, r.y + r.h - thickness, r.w, thickness}, col, layer);
    draw_rect({r.x, r.y + thickness, thickness, r.h - thickness * 2.0f}, col, layer);
    draw_rect({r.x + r.w - thickness, r.y + thickness, thickness, r.h - thickness * 2.0f}, col, layer);
#endif
}

void draw_panel(const Rect& r, const char* title, const Color* bg, const Color* border, i32 layer) {
#ifndef __EMSCRIPTEN__
    Color bg_col = bg ? *bg : g_theme.bg_panel;
    Color border_col = border ? *border : g_theme.border;

    // Body
    draw_rect(r, bg_col, layer);
    draw_rect_outline(r, 1.5f, border_col, layer);

    // Title bar if present
    if (title && *title) {
        f32 title_h = 26.0f;
        Rect header_r = {r.x, r.y, r.w, title_h};
        draw_rect(header_r, g_theme.bg_card, layer + 1);
        draw_rect_outline(header_r, 1.0f, g_theme.border, layer + 1);
        draw_text(title, r.x + 10.0f, r.y + 13.0f, 13.0f, 1.0f, g_theme.text_accent, layer + 2, false, 0.2f);
    }
#endif
}

void draw_text(const char* text, f32 x, f32 y, f32 px, f32 spacing,
               const Color& col, i32 layer, bool centered, f32 glow) {
#ifndef __EMSCRIPTEN__
    s_native_backend.draw_text_screen(g_current_renderer, text, x, y, px, spacing, col, layer, centered, glow);
#endif
}

f32 measure(const char* text, f32 px, f32 spacing) {
#ifndef __EMSCRIPTEN__
    return s_native_backend.measure(text, px, spacing);
#else
    return 0.0f;
#endif
}

bool button(const char* label, const Rect& r, bool active, i32 layer) {
    Vec2 mpos = Input::mouse_pos();
    bool hovered = r.contains(mpos);
    bool clicked = hovered && Input::was_mouse_pressed(MouseButton::Left);

    Color bg = active ? g_theme.bg_active : (hovered ? g_theme.bg_hover : g_theme.bg_card);
    Color border = active ? g_theme.border_active : (hovered ? g_theme.border_bright : g_theme.border);
    Color text_col = active ? g_theme.text_primary : (hovered ? g_theme.text_primary : g_theme.text_secondary);

    draw_rect(r, bg, layer);
    draw_rect_outline(r, active || hovered ? 2.0f : 1.0f, border, layer);
    if (hovered) {
        draw_rect(r, Color{0.2f, 0.8f, 1.0f, 0.15f}, layer + 1, rhi::BlendMode::Additive);
    }

    Vec2 c = r.center();
    draw_text(label, c.x, c.y, 14.0f, 1.0f, text_col, layer + 2, true, hovered ? 0.3f : 0.0f);
    return clicked;
}

bool icon_button(const char* label, const char* icon_txt, const Rect& r, bool active, i32 layer) {
    Vec2 mpos = Input::mouse_pos();
    bool hovered = r.contains(mpos);
    bool clicked = hovered && Input::was_mouse_pressed(MouseButton::Left);

    Color bg = active ? g_theme.bg_active : (hovered ? g_theme.bg_hover : g_theme.bg_card);
    Color border = active ? g_theme.border_active : (hovered ? g_theme.border_bright : g_theme.border);

    draw_rect(r, bg, layer);
    draw_rect_outline(r, 1.5f, border, layer);

    if (icon_txt && *icon_txt) {
        draw_text(icon_txt, r.x + 8.0f, r.y + r.h * 0.5f, 15.0f, 0.0f, g_theme.text_accent, layer + 2, false);
    }
    if (label && *label) {
        f32 tx = (icon_txt && *icon_txt) ? r.x + 28.0f : r.x + 8.0f;
        draw_text(label, tx, r.y + r.h * 0.5f, 13.0f, 0.5f, g_theme.text_primary, layer + 2, false);
    }
    return clicked;
}

bool toggle_button(const char* label, bool* value, const Rect& r, i32 layer) {
    bool current = value ? *value : false;
    if (button(label, r, current, layer)) {
        if (value) *value = !*value;
        return true;
    }
    return false;
}

void progress_bar(const Rect& r, f32 ratio, const Color& fill_col, const Color* bg_col, const char* label, i32 layer) {
    Color bg = bg_col ? *bg_col : g_theme.bg_dark;
    draw_rect(r, bg, layer);
    draw_rect_outline(r, 1.0f, g_theme.border, layer);

    f32 clamped = std::clamp(ratio, 0.0f, 1.0f);
    if (clamped > 0.0f) {
        Rect fill_r = {r.x + 1.0f, r.y + 1.0f, (r.w - 2.0f) * clamped, r.h - 2.0f};
        draw_rect(fill_r, fill_col, layer + 1);
        Color glow = fill_col;
        glow.a = 0.35f;
        draw_rect(fill_r, glow, layer + 1, rhi::BlendMode::Additive);
    }

    if (label && *label) {
        Vec2 c = r.center();
        draw_text(label, c.x, c.y, 11.0f, 0.5f, g_theme.text_primary, layer + 2, true);
    }
}

void stat_card(const Rect& r, const char* title, const char* value, const char* subtitle, const Color* accent, i32 layer) {
    Color acc = accent ? *accent : g_theme.accent_cyan;
    draw_panel(r, nullptr, &g_theme.bg_card, &g_theme.border, layer);

    // Left accent bar
    draw_rect({r.x, r.y, 3.0f, r.h}, acc, layer + 1);

    if (title) {
        draw_text(title, r.x + 10.0f, r.y + 12.0f, 11.0f, 0.5f, g_theme.text_muted, layer + 2);
    }
    if (value) {
        draw_text(value, r.x + 10.0f, r.y + 28.0f, 18.0f, 1.0f, g_theme.text_primary, layer + 2, false, 0.2f);
    }
    if (subtitle) {
        draw_text(subtitle, r.x + 10.0f, r.y + r.h - 10.0f, 10.0f, 0.0f, acc, layer + 2);
    }
}

void badge(const char* text, f32 x, f32 y, const Color& bg, const Color& fg, i32 layer) {
    f32 w = measure(text, 11.0f, 0.0f) + 12.0f;
    f32 h = 18.0f;
    Rect r = {x, y - h * 0.5f, w, h};
    draw_rect(r, bg, layer);
    draw_rect_outline(r, 1.0f, fg, layer);
    draw_text(text, r.center().x, r.center().y, 11.0f, 0.0f, fg, layer + 1, true);
}

void tooltip(const char* text, const Vec2& pos, i32 layer) {
    if (!text || !*text) return;
    f32 w = measure(text, 12.0f, 0.5f) + 16.0f;
    f32 h = 24.0f;
    Rect r = {pos.x + 12.0f, pos.y + 12.0f, w, h};
    draw_rect(r, g_theme.bg_dark, layer);
    draw_rect_outline(r, 1.0f, g_theme.border_bright, layer);
    draw_text(text, r.center().x, r.center().y, 12.0f, 0.5f, g_theme.text_primary, layer + 1, true);
}

void flash(const char* text, f32 duration) {
    active_backend()->flash(text, duration);
}

void set_text(const char* element, const char* text) {
    active_backend()->set_text(element, text);
}

void set_visible(const char* element, bool visible) {
    active_backend()->set_visible(element, visible);
}

void set_fill(const char* element, f32 ratio) {
    active_backend()->set_fill(element, ratio);
}

const char* get_text(const char* element, const char* default_val) {
    return active_backend()->get_text(element, default_val);
}

bool visible(const char* element, bool default_val) {
    return active_backend()->is_visible(element, default_val);
}

f32 fill(const char* element, f32 default_val) {
    return active_backend()->get_fill(element, default_val);
}

void draw(Renderer2D* renderer) {
    begin_frame(renderer);
    active_backend()->draw(renderer);
    end_frame();
}

}
