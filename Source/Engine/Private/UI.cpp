#include "UI.h"
#include "Container/Array.h"
#include "Container/HashMap.h"
#include "Container/String.h"
#include "Input.h"
#include "Math/Math.h"
#include "Platform.h"
#include "Renderer2D.h"
#include "Texture.h"

#include <cmath>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/em_js.h>
#else
#define NOMINMAX
#include <windows.h>
// Win32's DrawText macro would rename our UI::DrawText overloads; UI draws
// text through SpriteBatch glyphs, never through the Win32 API.
#undef DrawText
#endif

namespace Aether::Engine::UI {

namespace {

Theme gTheme;
Renderer2D* gCurrentRenderer = nullptr;

#ifdef __EMSCRIPTEN__

EM_JS(void, JsUiInit, (), {
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

EM_JS(void, JsUiSetText, (const char* element, const char* text), {
    Module.__aether_ui.set(UTF8ToString(element), UTF8ToString(text));
});
EM_JS(void, JsUiSetVisible, (const char* element, int visible), {
    Module.__aether_ui.visible(UTF8ToString(element), visible ? true : false);
});
EM_JS(void, JsUiSetFill, (const char* element, double ratio), {
    Module.__aether_ui.fill(UTF8ToString(element), ratio);
});
EM_JS(void, JsUiFlash, (const char* text, double duration), {
    Module.__aether_ui.flash(UTF8ToString(text), duration);
});

class DomBackend : public Backend
{
public:
    void SetText(const char* element, const char* text) override
    {
        JsUiSetText(element, text);
    }
    void SetVisible(const char* element, bool visible) override
    {
        JsUiSetVisible(element, visible ? 1 : 0);
    }
    void SetFill(const char* element, f32 ratio) override
    {
        JsUiSetFill(element, (double)ratio);
    }
    void Flash(const char* text, f32 duration) override
    {
        JsUiFlash(text, (double)duration);
    }
};

DomBackend sDomBackend;

#else

constexpr u32 ATLAS_CELL = 44;
constexpr u32 ATLAS_COLS = 32;
constexpr u32 ATLAS_ROWS = 32;
constexpr u32 ATLAS_WIDTH = ATLAS_COLS * ATLAS_CELL;
constexpr u32 ATLAS_HEIGHT = ATLAS_ROWS * ATLAS_CELL;
constexpr u32 ATLAS_FONT_PX = 30;
constexpr u32 ATLAS_PAD = (ATLAS_CELL - ATLAS_FONT_PX) / 2;

constexpr i32 LAYER_FLASH = 38;

struct GlyphInfo
{
    u32 UvX = 0;
    u32 UvY = 0;
    u32 BbX = 0;
    u32 BbY = 0;
    u32 BbW = 0;
    u32 BbH = 0;
    u32 Advance = 0;
};

class NativeBackend : public Backend
{
public:
    NativeBackend() = default;
    ~NativeBackend()
    {
        if (mFont) DeleteObject(mFont);
        if (mDib) DeleteObject(mDib);
        if (mMemDc) DeleteDC(mMemDc);
    }

    void SetText(const char* element, const char* text) override
    {
        mTextMap.FindOrAdd(element) = text;
    }
    void SetVisible(const char* element, bool visible) override
    {
        mVisibleMap.FindOrAdd(element) = visible;
    }
    void SetFill(const char* element, f32 ratio) override
    {
        mFillMap.FindOrAdd(element) = ratio;
    }
    void Flash(const char* text, f32 duration) override
    {
        mFlashText = text;
        mFlashStart = Aether::Platform::NowSeconds();
        mFlashDuration = duration > 0.0f ? duration : 1.5f;
    }

    const char* GetText(const char* key, const char* fallback) override
    {
        const String* found = mTextMap.Find(key);
        return found ? found->CStr() : fallback;
    }
    bool IsVisible(const char* key, bool fallback) override
    {
        const bool* found = mVisibleMap.Find(key);
        return found ? *found : fallback;
    }
    f32 GetFill(const char* key, f32 fallback) override
    {
        const f32* found = mFillMap.Find(key);
        return found ? *found : fallback;
    }

    void Draw(Renderer2D* renderer) override
    {
        if (!renderer || !renderer->Device()) return;
        EnsureAssets(renderer);
        if (!mAtlas) return;

        if (!mFlashText.IsEmpty())
        {
            double now = Aether::Platform::NowSeconds();
            double fade = now - mFlashStart;
            double total = mFlashDuration + 0.5;
            if (fade < total)
            {
                f32 alpha = fade < mFlashDuration
                                ? 1.0f
                                : (f32)(1.0 - (fade - mFlashDuration) / 0.5);
                const char* flash = mFlashText.CStr();
                f32 halfW = 360.0f * renderer->Aspect();
                DrawTextScreen(renderer, flash, halfW, 274.0f, 38.0f, 6.0f, Math::Color{1.0f, 0.86f, 0.47f, alpha * 0.8f},
                               LAYER_FLASH, true, 0.4f);
                DrawTextScreen(renderer, flash, halfW, 274.0f, 38.0f, 6.0f, Math::Color{1.0f, 1.0f, 1.0f, alpha},
                               LAYER_FLASH, true, 0.0f);
            }
            else
            {
                mFlashText.Clear();
            }
        }
    }

    void DrawRectScreen(Renderer2D* renderer, const Rect& r, const Math::Color& col, i32 layer,
                        RHI::BlendMode blend = RHI::BlendMode::Alpha)
    {
        if (!renderer || !renderer->Device() || r.W <= 0.0f || r.H <= 0.0f) return;
        EnsureAssets(renderer);
        if (!mSolid) return;

        f32 halfH = 360.0f;
        f32 halfW = halfH * renderer->Aspect();
        Math::Vec2 cam = renderer->GetCamera().Position;
        f32 sx = (r.X - halfW) + cam.x + r.W * 0.5f;
        f32 sy = (r.Y - halfH) + cam.y + r.H * 0.5f;

        renderer->Sprites().Add(mSolid, {sx, sy}, {r.W, r.H}, col, 0.0f, layer, blend);
    }

    void DrawGlowScreen(Renderer2D* renderer, const Rect& r, const Math::Color& col, i32 layer)
    {
        if (!renderer || !renderer->Device() || r.W <= 0.0f || r.H <= 0.0f) return;
        EnsureAssets(renderer);
        if (!mGlow) return;

        f32 halfH = 360.0f;
        f32 halfW = halfH * renderer->Aspect();
        Math::Vec2 cam = renderer->GetCamera().Position;
        f32 sx = (r.X - halfW) + cam.x + r.W * 0.5f;
        f32 sy = (r.Y - halfH) + cam.y + r.H * 0.5f;

        renderer->Sprites().Add(mGlow, {sx, sy}, {r.W * 1.5f, r.H * 1.5f}, col, 0.0f, layer, RHI::BlendMode::Additive);
    }

    void EnsureGlyph(RHI::RHIDevice* device, wchar_t wc)
    {
        if (mGlyphMap.Find(wc)) return;
        if (!mMemDc || mNextSlot >= ATLAS_COLS * ATLAS_ROWS) return;

        u32 slot = mNextSlot++;
        u32 col = slot % ATLAS_COLS;
        u32 row = slot / ATLAS_COLS;
        u32 x0 = col * ATLAS_CELL;
        u32 y0 = row * ATLAS_CELL;

        wchar_t str[2] = {wc, 0};
        SetTextColor(mMemDc, RGB(255, 255, 255));
        TextOutW(mMemDc, (LONG)x0 + 2, (LONG)y0 + ATLAS_PAD, str, 1);

        SIZE s = {};
        GetTextExtentPoint32W(mMemDc, str, 1, &s);

        const u8* bgra = (const u8*)mDibBits;
        u32 minX = ATLAS_CELL, maxX = 0, minY = ATLAS_CELL, maxY = 0;
        for (u32 yy = 0; yy < ATLAS_CELL; yy++)
        {
            for (u32 xx = 0; xx < ATLAS_CELL; xx++)
            {
                u32 idx = ((y0 + yy) * ATLAS_WIDTH + (x0 + xx)) * 4;
                u8 lum = bgra[idx + 1];
                mRgba[idx + 0] = 255;
                mRgba[idx + 1] = 255;
                mRgba[idx + 2] = 255;
                mRgba[idx + 3] = lum;
                if (lum > 0)
                {
                    if (xx < minX) minX = xx;
                    if (xx > maxX) maxX = xx;
                    if (yy < minY) minY = yy;
                    if (yy > maxY) maxY = yy;
                }
            }
        }

        GlyphInfo g;
        g.UvX = x0;
        g.UvY = y0;
        g.Advance = (u32)s.cx + 1;
        if (maxX >= minX && maxY >= minY)
        {
            g.BbX = minX;
            g.BbY = minY;
            g.BbW = maxX - minX + 1;
            g.BbH = maxY - minY + 1;
        }
        mGlyphMap.Add(wc, g);
        mAtlasDirty = true;
    }

    Array<wchar_t> ToWideString(const char* text)
    {
        Array<wchar_t> wstr;
        if (!text || !*text) return wstr;
        int len = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
        if (len <= 1) return wstr;
        wstr.Resize((u32)(len - 1));
        MultiByteToWideChar(CP_UTF8, 0, text, -1, wstr.Data(), len);
        return wstr;
    }

    void DrawTextScreen(Renderer2D* renderer, const char* text, f32 screenX, f32 screenY,
                        f32 px, f32 spacing, const Math::Color& col, i32 layer, bool centered, f32 glow)
    {
        if (!renderer || !renderer->Device() || !text || !*text) return;
        EnsureAssets(renderer);

        Array<wchar_t> wtext = ToWideString(text);
        if (wtext.IsEmpty()) return;

        for (wchar_t wc : wtext)
        {
            EnsureGlyph(renderer->Device(), wc);
        }

        if (mAtlasDirty)
        {
            mAtlas = MakeTexture(renderer->Device(), ATLAS_WIDTH, ATLAS_HEIGHT,
                                 [this](u32 x, u32 y, u8 out[4])
                                 {
                                     u32 idx = (y * ATLAS_WIDTH + x) * 4;
                                     out[0] = mRgba[idx + 0];
                                     out[1] = mRgba[idx + 1];
                                     out[2] = mRgba[idx + 2];
                                     out[3] = mRgba[idx + 3];
                                 });
            mAtlasDirty = false;
        }

        if (!mAtlas) return;

        f32 halfH = 360.0f;
        f32 halfW = halfH * renderer->Aspect();
        Math::Vec2 cam = renderer->GetCamera().Position;
        f32 worldX = (screenX - halfW) + cam.x;
        f32 worldY = (screenY - halfH) + cam.y;

        f32 scale = px / (f32)ATLAS_FONT_PX;
        f32 total = Measure(text, px, spacing);
        f32 x = centered ? worldX - total * 0.5f : worldX;
        f32 y = worldY;

        f32 inkTop = 1e30f, inkBottom = -1e30f;
        for (wchar_t wc : wtext)
        {
            GlyphInfo* g = mGlyphMap.Find(wc);
            if (!g) continue;
            if (g->BbW == 0 || g->BbH == 0) continue;
            if ((f32)g->BbY < inkTop) inkTop = (f32)g->BbY;
            if ((f32)(g->BbY + g->BbH) > inkBottom) inkBottom = (f32)(g->BbY + g->BbH);
        }
        f32 inkCenter = (inkTop + inkBottom) * 0.5f;

        for (wchar_t wc : wtext)
        {
            GlyphInfo* g = mGlyphMap.Find(wc);
            if (g)
            {
                if (g->BbW > 0 && g->BbH > 0)
                {
                    f32 xMidOrig = (f32)g->BbX + (f32)g->BbW * 0.5f;
                    f32 yMidOrig = (f32)g->BbY + (f32)g->BbH * 0.5f;
                    f32 cx = x + xMidOrig * scale;
                    f32 cy = y + (yMidOrig - inkCenter) * scale;
                    f32 w = (f32)g->BbW * scale;
                    f32 h = (f32)g->BbH * scale;

                    f32 u0 = (f32)(g->UvX + g->BbX) / (f32)ATLAS_WIDTH;
                    f32 v0 = (f32)(g->UvY + g->BbY) / (f32)ATLAS_HEIGHT;
                    f32 u1 = u0 + (f32)g->BbW / (f32)ATLAS_WIDTH;
                    f32 v1 = v0 + (f32)g->BbH / (f32)ATLAS_HEIGHT;

                    if (glow > 0.0f && mGlow)
                    {
                        Math::Color glowCol = col;
                        glowCol.a = glow * col.a * 0.4f;
                        renderer->Sprites().AddUv(mAtlas, {u0, v0}, {u1, v1}, {cx, cy}, {w * 1.3f, h * 1.3f},
                                                  glowCol, 0.0f, layer - 1, RHI::BlendMode::Additive);
                    }

                    renderer->Sprites().AddUv(mAtlas, {u0, v0}, {u1, v1}, {cx, cy}, {w, h},
                                              col, 0.0f, layer, RHI::BlendMode::Alpha);
                }
                x += (f32)g->Advance * scale + spacing;
            }
            else
            {
                x += px * 0.6f + spacing;
            }
        }
    }

    f32 Measure(const char* text, f32 px, f32 spacing)
    {
        Array<wchar_t> wtext = ToWideString(text);
        if (wtext.IsEmpty()) return 0.0f;

        f32 scale = px / (f32)ATLAS_FONT_PX;
        f32 total = 0.0f;
        u32 n = 0;
        for (wchar_t wc : wtext)
        {
            GlyphInfo* g = mGlyphMap.Find(wc);
            if (g)
            {
                total += (f32)g->Advance * scale;
            }
            else
            {
                total += (wc > 128 ? px * 0.95f : px * 0.55f);
            }
            n++;
        }
        if (n > 1) total += spacing * (f32)(n - 1);
        return total;
    }

private:
    void EnsureAssets(Renderer2D* renderer)
    {
        if (mMemDc) return;
        BuildAtlas(renderer->Device());
        mSolid = MakeSolidTexture(renderer->Device(), 4, 4, {1.0f, 1.0f, 1.0f, 1.0f});
        mGlow = MakeGlowTexture(renderer->Device(), 256);
    }

    bool BuildAtlas(RHI::RHIDevice* device)
    {
        mBuildInProgress = true;
        mMemDc = CreateCompatibleDC(nullptr);
        if (!mMemDc)
        {
            mBuildInProgress = false;
            return false;
        }

        BITMAPINFO bi = {};
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = (LONG)ATLAS_WIDTH;
        bi.bmiHeader.biHeight = -(LONG)ATLAS_HEIGHT;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;

        mDib = CreateDIBSection(mMemDc, &bi, DIB_RGB_COLORS, &mDibBits, nullptr, 0);
        if (!mDib)
        {
            DeleteDC(mMemDc);
            mMemDc = nullptr;
            mBuildInProgress = false;
            return false;
        }
        SelectObject(mMemDc, mDib);

        HBRUSH bg = CreateSolidBrush(RGB(0, 0, 0));
        RECT full = {0, 0, (LONG)ATLAS_WIDTH, (LONG)ATLAS_HEIGHT};
        FillRect(mMemDc, &full, bg);
        DeleteObject(bg);

        mFont = CreateFontW(-(LONG)ATLAS_FONT_PX, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei");
        SelectObject(mMemDc, mFont);

        SetBkMode(mMemDc, TRANSPARENT);
        SetTextAlign(mMemDc, TA_TOP | TA_LEFT);

        mRgba.Clear();
        mRgba.Resize(ATLAS_WIDTH * ATLAS_HEIGHT * 4);

        // Pre-populate ASCII printable range (32..126)
        for (u32 i = 0; i < 95; i++)
        {
            EnsureGlyph(device, (wchar_t)(i + 32));
        }

        mAtlas = MakeTexture(device, ATLAS_WIDTH, ATLAS_HEIGHT,
                             [this](u32 x, u32 y, u8 out[4])
                             {
                                 u32 idx = (y * ATLAS_WIDTH + x) * 4;
                                 out[0] = mRgba[idx + 0];
                                 out[1] = mRgba[idx + 1];
                                 out[2] = mRgba[idx + 2];
                                 out[3] = mRgba[idx + 3];
                             });

        mAtlasDirty = false;
        mBuildInProgress = false;
        return mAtlas.IsValid();
    }

    HashMap<String, String> mTextMap;
    HashMap<String, bool> mVisibleMap;
    HashMap<String, f32> mFillMap;

    String mFlashText;
    double mFlashStart = 0.0;
    f32 mFlashDuration = 1.5f;

    RefPtr<RHI::RHITexture> mAtlas;
    RefPtr<RHI::RHITexture> mSolid;
    RefPtr<RHI::RHITexture> mGlow;
    Array<u8> mRgba;
    HashMap<wchar_t, GlyphInfo> mGlyphMap;
    u32 mNextSlot = 0;
    bool mAtlasDirty = false;
    bool mBuildInProgress = false;

    HDC mMemDc = nullptr;
    HBITMAP mDib = nullptr;
    HFONT mFont = nullptr;
    void* mDibBits = nullptr;
};

NativeBackend sNativeBackend;

#endif

Backend* sBackend = nullptr;

Backend* ActiveBackend()
{
    if (sBackend) return sBackend;
#ifdef __EMSCRIPTEN__
    return &sDomBackend;
#else
    return &sNativeBackend;
#endif
}

} // namespace

void Init()
{
#ifdef __EMSCRIPTEN__
    JsUiInit();
#endif
}

void SetBackend(Backend* backend)
{
    sBackend = backend;
}

Theme& GetTheme()
{
    return gTheme;
}

void BeginFrame(Renderer2D* renderer)
{
    gCurrentRenderer = renderer;
}

void EndFrame()
{
    gCurrentRenderer = nullptr;
}

void DrawRect(const Rect& r, const Math::Color& col, i32 layer, RHI::BlendMode blend)
{
#ifndef __EMSCRIPTEN__
    sNativeBackend.DrawRectScreen(gCurrentRenderer, r, col, layer, blend);
#endif
}

void DrawRectOutline(const Rect& r, f32 thickness, const Math::Color& col, i32 layer)
{
#ifndef __EMSCRIPTEN__
    if (r.W <= 0.0f || r.H <= 0.0f || thickness <= 0.0f) return;
    // Top, bottom, left, right borders
    DrawRect({r.X, r.Y, r.W, thickness}, col, layer);
    DrawRect({r.X, r.Y + r.H - thickness, r.W, thickness}, col, layer);
    DrawRect({r.X, r.Y + thickness, thickness, r.H - thickness * 2.0f}, col, layer);
    DrawRect({r.X + r.W - thickness, r.Y + thickness, thickness, r.H - thickness * 2.0f}, col, layer);
#endif
}

void DrawPanel(const Rect& r, const char* title, const Math::Color* bg, const Math::Color* border, i32 layer)
{
#ifndef __EMSCRIPTEN__
    Math::Color bgColor = bg ? *bg : gTheme.BgPanel;
    Math::Color borderColor = border ? *border : gTheme.Border;

    // Body
    DrawRect(r, bgColor, layer);
    DrawRectOutline(r, 1.5f, borderColor, layer);

    // Title bar if present
    if (title && *title)
    {
        f32 titleH = 26.0f;
        Rect headerRect = {r.X, r.Y, r.W, titleH};
        DrawRect(headerRect, gTheme.BgCard, layer + 1);
        DrawRectOutline(headerRect, 1.0f, gTheme.Border, layer + 1);
        DrawText(title, r.X + 10.0f, r.Y + 13.0f, 13.0f, 1.0f, gTheme.TextAccent, layer + 2, false, 0.2f);
    }
#endif
}

void DrawText(const char* text, f32 x, f32 y, f32 px, f32 spacing,
              const Math::Color& col, i32 layer, bool centered, f32 glow)
{
#ifndef __EMSCRIPTEN__
    sNativeBackend.DrawTextScreen(gCurrentRenderer, text, x, y, px, spacing, col, layer, centered, glow);
#endif
}

f32 Measure(const char* text, f32 px, f32 spacing)
{
#ifndef __EMSCRIPTEN__
    return sNativeBackend.Measure(text, px, spacing);
#else
    return 0.0f;
#endif
}

bool Button(const char* label, const Rect& r, bool active, i32 layer)
{
    Math::Vec2 mpos = Input::MousePos();
    bool hovered = r.Contains(mpos);
    bool clicked = hovered && Input::WasMousePressed(MouseButton::Left);

    Math::Color bg = active ? gTheme.BgActive : (hovered ? gTheme.BgHover : gTheme.BgCard);
    Math::Color border = active ? gTheme.BorderActive : (hovered ? gTheme.BorderBright : gTheme.Border);
    Math::Color textCol = active ? gTheme.TextPrimary : (hovered ? gTheme.TextPrimary : gTheme.TextSecondary);

    DrawRect(r, bg, layer);
    DrawRectOutline(r, active || hovered ? 2.0f : 1.0f, border, layer);
    if (hovered)
    {
        DrawRect(r, Math::Color{0.2f, 0.8f, 1.0f, 0.15f}, layer + 1, RHI::BlendMode::Additive);
    }

    Math::Vec2 c = r.Center();
    DrawText(label, c.x, c.y, 14.0f, 1.0f, textCol, layer + 2, true, hovered ? 0.3f : 0.0f);
    return clicked;
}

bool IconButton(const char* label, const char* iconTxt, const Rect& r, bool active, i32 layer)
{
    Math::Vec2 mpos = Input::MousePos();
    bool hovered = r.Contains(mpos);
    bool clicked = hovered && Input::WasMousePressed(MouseButton::Left);

    Math::Color bg = active ? gTheme.BgActive : (hovered ? gTheme.BgHover : gTheme.BgCard);
    Math::Color border = active ? gTheme.BorderActive : (hovered ? gTheme.BorderBright : gTheme.Border);

    DrawRect(r, bg, layer);
    DrawRectOutline(r, 1.5f, border, layer);

    if (iconTxt && *iconTxt)
    {
        DrawText(iconTxt, r.X + 8.0f, r.Y + r.H * 0.5f, 15.0f, 0.0f, gTheme.TextAccent, layer + 2, false);
    }
    if (label && *label)
    {
        f32 tx = (iconTxt && *iconTxt) ? r.X + 28.0f : r.X + 8.0f;
        DrawText(label, tx, r.Y + r.H * 0.5f, 13.0f, 0.5f, gTheme.TextPrimary, layer + 2, false);
    }
    return clicked;
}

bool ToggleButton(const char* label, bool* value, const Rect& r, i32 layer)
{
    bool current = value ? *value : false;
    if (Button(label, r, current, layer))
    {
        if (value) *value = !*value;
        return true;
    }
    return false;
}

void ProgressBar(const Rect& r, f32 ratio, const Math::Color& fillCol, const Math::Color* bgCol, const char* label, i32 layer)
{
    Math::Color bg = bgCol ? *bgCol : gTheme.BgDark;
    DrawRect(r, bg, layer);
    DrawRectOutline(r, 1.0f, gTheme.Border, layer);

    f32 clamped = Math::Clamp(ratio, 0.0f, 1.0f);
    if (clamped > 0.0f)
    {
        Rect fillRect = {r.X + 1.0f, r.Y + 1.0f, (r.W - 2.0f) * clamped, r.H - 2.0f};
        DrawRect(fillRect, fillCol, layer + 1);
        Math::Color glow = fillCol;
        glow.a = 0.35f;
        DrawRect(fillRect, glow, layer + 1, RHI::BlendMode::Additive);
    }

    if (label && *label)
    {
        Math::Vec2 c = r.Center();
        DrawText(label, c.x, c.y, 11.0f, 0.5f, gTheme.TextPrimary, layer + 2, true);
    }
}

void StatCard(const Rect& r, const char* title, const char* value, const char* subtitle, const Math::Color* accent, i32 layer)
{
    Math::Color acc = accent ? *accent : gTheme.AccentCyan;
    DrawPanel(r, nullptr, &gTheme.BgCard, &gTheme.Border, layer);

    // Left accent bar
    DrawRect({r.X, r.Y, 3.0f, r.H}, acc, layer + 1);

    if (title)
    {
        DrawText(title, r.X + 10.0f, r.Y + 12.0f, 11.0f, 0.5f, gTheme.TextMuted, layer + 2);
    }
    if (value)
    {
        DrawText(value, r.X + 10.0f, r.Y + 28.0f, 18.0f, 1.0f, gTheme.TextPrimary, layer + 2, false, 0.2f);
    }
    if (subtitle)
    {
        DrawText(subtitle, r.X + 10.0f, r.Y + r.H - 10.0f, 10.0f, 0.0f, acc, layer + 2);
    }
}

void Badge(const char* text, f32 x, f32 y, const Math::Color& bg, const Math::Color& fg, i32 layer)
{
    f32 w = Measure(text, 11.0f, 0.0f) + 12.0f;
    f32 h = 18.0f;
    Rect r = {x, y - h * 0.5f, w, h};
    DrawRect(r, bg, layer);
    DrawRectOutline(r, 1.0f, fg, layer);
    DrawText(text, r.Center().x, r.Center().y, 11.0f, 0.0f, fg, layer + 1, true);
}

void Tooltip(const char* text, const Math::Vec2& pos, i32 layer)
{
    if (!text || !*text) return;
    f32 w = Measure(text, 12.0f, 0.5f) + 16.0f;
    f32 h = 24.0f;
    Rect r = {pos.x + 12.0f, pos.y + 12.0f, w, h};
    DrawRect(r, gTheme.BgDark, layer);
    DrawRectOutline(r, 1.0f, gTheme.BorderBright, layer);
    DrawText(text, r.Center().x, r.Center().y, 12.0f, 0.5f, gTheme.TextPrimary, layer + 1, true);
}

void Flash(const char* text, f32 duration)
{
    ActiveBackend()->Flash(text, duration);
}

void SetText(const char* element, const char* text)
{
    ActiveBackend()->SetText(element, text);
}

void SetVisible(const char* element, bool visible)
{
    ActiveBackend()->SetVisible(element, visible);
}

void SetFill(const char* element, f32 ratio)
{
    ActiveBackend()->SetFill(element, ratio);
}

const char* GetText(const char* element, const char* defaultValue)
{
    return ActiveBackend()->GetText(element, defaultValue);
}

bool Visible(const char* element, bool defaultValue)
{
    return ActiveBackend()->IsVisible(element, defaultValue);
}

f32 Fill(const char* element, f32 defaultValue)
{
    return ActiveBackend()->GetFill(element, defaultValue);
}

void Draw(Renderer2D* renderer)
{
    BeginFrame(renderer);
    ActiveBackend()->Draw(renderer);
    EndFrame();
}

}

