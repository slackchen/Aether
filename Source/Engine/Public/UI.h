#pragma once

#include "Core.h"
#include "Math/Color.h"
#include "Math/Vec2.h"
#include "RHI.h"

namespace Aether::Engine {

class Renderer2D;

namespace UI {

struct Rect
{
    f32 X = 0.0f;
    f32 Y = 0.0f;
    f32 W = 0.0f;
    f32 H = 0.0f;

    bool Contains(const Math::Vec2& p) const
    {
        return p.x >= X && p.x <= (X + W) && p.y >= Y && p.y <= (Y + H);
    }
    Math::Vec2 Center() const { return {X + W * 0.5f, Y + H * 0.5f}; }
    Rect Shrink(f32 pad) const { return {X + pad, Y + pad, Math::Max(0.0f, W - pad * 2.0f), Math::Max(0.0f, H - pad * 2.0f)}; }
    Rect Offset(f32 dx, f32 dy) const { return {X + dx, Y + dy, W, H}; }
};

struct Theme
{
    Math::Color BgDark{0.04f, 0.06f, 0.10f, 0.88f};
    Math::Color BgPanel{0.07f, 0.11f, 0.18f, 0.85f};
    Math::Color BgCard{0.10f, 0.16f, 0.25f, 0.80f};
    Math::Color BgHover{0.16f, 0.26f, 0.40f, 0.90f};
    Math::Color BgActive{0.20f, 0.38f, 0.60f, 0.95f};

    Math::Color Border{0.20f, 0.35f, 0.55f, 0.70f};
    Math::Color BorderBright{0.40f, 0.75f, 1.0f, 0.90f};
    Math::Color BorderActive{0.20f, 0.85f, 1.0f, 1.0f};

    Math::Color TextPrimary{0.92f, 0.96f, 1.0f, 1.0f};
    Math::Color TextSecondary{0.60f, 0.75f, 0.90f, 1.0f};
    Math::Color TextMuted{0.38f, 0.50f, 0.65f, 1.0f};
    Math::Color TextAccent{0.25f, 0.85f, 1.0f, 1.0f};

    Math::Color AccentBlue{0.20f, 0.60f, 1.0f, 1.0f};
    Math::Color AccentCyan{0.15f, 0.88f, 0.95f, 1.0f};
    Math::Color AccentGold{1.0f, 0.80f, 0.25f, 1.0f};
    Math::Color AccentOrange{1.0f, 0.52f, 0.18f, 1.0f};
    Math::Color AccentRed{0.95f, 0.28f, 0.25f, 1.0f};
    Math::Color AccentGreen{0.25f, 0.90f, 0.45f, 1.0f};
    Math::Color AccentPurple{0.78f, 0.40f, 1.0f, 1.0f};
};

class Backend
{
public:
    virtual ~Backend() = default;

    virtual void SetText(const char* element, const char* text) = 0;
    virtual void SetVisible(const char* element, bool visible) = 0;
    virtual void SetFill(const char* element, f32 ratio) = 0;
    virtual void Flash(const char* text, f32 duration) = 0;

    virtual const char* GetText(const char* element, const char* defaultValue = "") { return defaultValue; }
    virtual bool IsVisible(const char* element, bool defaultValue = false) { return defaultValue; }
    virtual f32 GetFill(const char* element, f32 defaultValue = 0.0f) { return defaultValue; }

    virtual void Draw(Renderer2D* renderer) {}
};

void Init();
void SetBackend(Backend* backend);
Theme& GetTheme();

// Frame lifecycle
void BeginFrame(Renderer2D* renderer);
void EndFrame();

// Drawing Primitives
void DrawRect(const Rect& r, const Math::Color& col, i32 layer = 20, RHI::BlendMode blend = RHI::BlendMode::Alpha);
void DrawRectOutline(const Rect& r, f32 thickness, const Math::Color& col, i32 layer = 20);
void DrawPanel(const Rect& r, const char* title = nullptr, const Math::Color* bg = nullptr, const Math::Color* border = nullptr, i32 layer = 20);
void DrawText(const char* text, f32 x, f32 y, f32 px = 14.0f, f32 spacing = 0.0f,
              const Math::Color& col = Math::Color{0.9f, 0.95f, 1.0f, 1.0f}, i32 layer = 20, bool centered = false, f32 glow = 0.0f);
f32 Measure(const char* text, f32 px = 14.0f, f32 spacing = 0.0f);

// Interactive UI Widgets
bool Button(const char* label, const Rect& r, bool active = false, i32 layer = 20);
bool IconButton(const char* label, const char* iconTxt, const Rect& r, bool active = false, i32 layer = 20);
bool ToggleButton(const char* label, bool* value, const Rect& r, i32 layer = 20);
void ProgressBar(const Rect& r, f32 ratio, const Math::Color& fillCol, const Math::Color* bgCol = nullptr, const char* label = nullptr, i32 layer = 20);
void StatCard(const Rect& r, const char* title, const char* value, const char* subtitle = nullptr, const Math::Color* accent = nullptr, i32 layer = 20);
void Badge(const char* text, f32 x, f32 y, const Math::Color& bg, const Math::Color& fg, i32 layer = 20);
void Tooltip(const char* text, const Math::Vec2& pos, i32 layer = 35);
void Flash(const char* text, f32 duration = 1.5f);

// Key-Value legacy API
void SetText(const char* element, const char* text);
void SetVisible(const char* element, bool visible);
void SetFill(const char* element, f32 ratio);
const char* GetText(const char* element, const char* defaultValue = "");
bool Visible(const char* element, bool defaultValue = false);
f32 Fill(const char* element, f32 defaultValue = 0.0f);

void Draw(Renderer2D* renderer);

}

}
