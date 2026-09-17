#pragma once

#include "core/platform.h"
#include "core/math.h"
#include "rhi/rhi.h"
#include <string>
#include <vector>

namespace aether::engine {

class Renderer2D;

namespace ui {

struct Rect {
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 w = 0.0f;
    f32 h = 0.0f;

    bool contains(const Vec2& p) const {
        return p.x >= x && p.x <= (x + w) && p.y >= y && p.y <= (y + h);
    }
    Vec2 center() const { return {x + w * 0.5f, y + h * 0.5f}; }
    Rect shrink(f32 pad) const { return {x + pad, y + pad, std::max(0.0f, w - pad * 2.0f), std::max(0.0f, h - pad * 2.0f)}; }
    Rect offset(f32 dx, f32 dy) const { return {x + dx, y + dy, w, h}; }
};

struct Theme {
    Color bg_dark{0.04f, 0.06f, 0.10f, 0.88f};
    Color bg_panel{0.07f, 0.11f, 0.18f, 0.85f};
    Color bg_card{0.10f, 0.16f, 0.25f, 0.80f};
    Color bg_hover{0.16f, 0.26f, 0.40f, 0.90f};
    Color bg_active{0.20f, 0.38f, 0.60f, 0.95f};

    Color border{0.20f, 0.35f, 0.55f, 0.70f};
    Color border_bright{0.40f, 0.75f, 1.0f, 0.90f};
    Color border_active{0.20f, 0.85f, 1.0f, 1.0f};

    Color text_primary{0.92f, 0.96f, 1.0f, 1.0f};
    Color text_secondary{0.60f, 0.75f, 0.90f, 1.0f};
    Color text_muted{0.38f, 0.50f, 0.65f, 1.0f};
    Color text_accent{0.25f, 0.85f, 1.0f, 1.0f};

    Color accent_blue{0.20f, 0.60f, 1.0f, 1.0f};
    Color accent_cyan{0.15f, 0.88f, 0.95f, 1.0f};
    Color accent_gold{1.0f, 0.80f, 0.25f, 1.0f};
    Color accent_orange{1.0f, 0.52f, 0.18f, 1.0f};
    Color accent_red{0.95f, 0.28f, 0.25f, 1.0f};
    Color accent_green{0.25f, 0.90f, 0.45f, 1.0f};
    Color accent_purple{0.78f, 0.40f, 1.0f, 1.0f};
};

class Backend {
public:
    virtual ~Backend() = default;

    virtual void set_text(const char* element, const char* text) = 0;
    virtual void set_visible(const char* element, bool visible) = 0;
    virtual void set_fill(const char* element, f32 ratio) = 0;
    virtual void flash(const char* text, f32 duration) = 0;

    virtual const char* get_text(const char* element, const char* default_val = "") { return default_val; }
    virtual bool is_visible(const char* element, bool default_val = false) { return default_val; }
    virtual f32 get_fill(const char* element, f32 default_val = 0.0f) { return default_val; }

    virtual void draw(Renderer2D* renderer) {}
};

void init();
void set_backend(Backend* backend);
Theme& get_theme();

// Frame lifecycle
void begin_frame(Renderer2D* renderer);
void end_frame();

// Drawing Primitives
void draw_rect(const Rect& r, const Color& col, i32 layer = 20, rhi::BlendMode blend = rhi::BlendMode::Alpha);
void draw_rect_outline(const Rect& r, f32 thickness, const Color& col, i32 layer = 20);
void draw_panel(const Rect& r, const char* title = nullptr, const Color* bg = nullptr, const Color* border = nullptr, i32 layer = 20);
void draw_text(const char* text, f32 x, f32 y, f32 px = 14.0f, f32 spacing = 0.0f,
               const Color& col = Color{0.9f, 0.95f, 1.0f, 1.0f}, i32 layer = 20, bool centered = false, f32 glow = 0.0f);
f32 measure(const char* text, f32 px = 14.0f, f32 spacing = 0.0f);

// Interactive UI Widgets
bool button(const char* label, const Rect& r, bool active = false, i32 layer = 20);
bool icon_button(const char* label, const char* icon_txt, const Rect& r, bool active = false, i32 layer = 20);
bool toggle_button(const char* label, bool* value, const Rect& r, i32 layer = 20);
void progress_bar(const Rect& r, f32 ratio, const Color& fill_col, const Color* bg_col = nullptr, const char* label = nullptr, i32 layer = 20);
void stat_card(const Rect& r, const char* title, const char* value, const char* subtitle = nullptr, const Color* accent = nullptr, i32 layer = 20);
void badge(const char* text, f32 x, f32 y, const Color& bg, const Color& fg, i32 layer = 20);
void tooltip(const char* text, const Vec2& pos, i32 layer = 35);
void flash(const char* text, f32 duration = 1.5f);

// Key-Value legacy API
void set_text(const char* element, const char* text);
void set_visible(const char* element, bool visible);
void set_fill(const char* element, f32 ratio);
const char* get_text(const char* element, const char* default_val = "");
bool visible(const char* element, bool default_val = false);
f32 fill(const char* element, f32 default_val = 0.0f);

void draw(Renderer2D* renderer);

}

}
