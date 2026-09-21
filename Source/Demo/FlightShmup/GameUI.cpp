#include "GameUI.h"

#include "Math/Color.h"
#include "Math/Math.h"
#include "Container/String.h"
#include "Platform.h"
#include "Renderer2D.h"
#include "UI.h"

#include <cmath>

using namespace Aether;

namespace Shmup::UI
{

namespace
{

constexpr const char* HUD_SCORE = "hud-score";
constexpr const char* HUD_HISCORE = "hud-hiscore";
constexpr const char* HUD_LIVES = "hud-lives";
constexpr const char* HUD_WEAPON = "hud-weapon";
constexpr const char* HUD_BOSS = "hud-boss";
constexpr const char* BOSS_NAME = "boss-name";
constexpr const char* BOSS_FILL = "boss-fill";

constexpr i32 LAYER_HUD = 20;
constexpr i32 LAYER_BOSS = 21;
constexpr i32 LAYER_PANEL = 30;

}

void SetScore(i32 value)
{
    Engine::UI::SetText(HUD_SCORE, String::Format("%d", value).CStr());
}

void SetHiscore(i32 value)
{
    Engine::UI::SetText(HUD_HISCORE, String::Format("%d", value).CStr());
}

void SetLives(i32 value)
{
    Engine::UI::SetText(HUD_LIVES, String::Format("%d", value).CStr());
}

void SetWeapon(i32 level)
{
    Engine::UI::SetText(HUD_WEAPON, String::Format("%d", level).CStr());
}

void ShowScreen(const char* element, bool show)
{
    Engine::UI::SetVisible(element, show);
}

void SetText(const char* element, const char* text)
{
    Engine::UI::SetText(element, text);
}

void ShowBoss(const char* name)
{
    Engine::UI::SetVisible(HUD_BOSS, true);
    Engine::UI::SetText(BOSS_NAME, name);
}

void SetBossHp(f32 ratio)
{
    Engine::UI::SetFill(BOSS_FILL, ratio);
}

void HideBoss()
{
    Engine::UI::SetVisible(HUD_BOSS, false);
}

void FlashMessage(const char* text, f32 duration)
{
    Engine::UI::Flash(text, duration);
}

void Draw(Engine::Renderer2D* renderer)
{
    if (!renderer)
    {
        return;
    }

    Engine::UI::BeginFrame(renderer);

    f32 halfH = 360.0f;
    f32 halfW = halfH * renderer->Aspect();

    auto overlay = [&]() {
        Engine::UI::DrawRect({0.0f, 0.0f, halfW * 2.0f, halfH * 2.0f},
                             Math::Color{0.0f, 0.0f, 0.0f, 0.8f}, LAYER_PANEL);
    };

    struct HudGroup
    {
        const char* Label;
        const char* Key;
    };
    const HudGroup groups[] = {
        {"SCORE", HUD_SCORE},
        {"HI-SCORE", HUD_HISCORE},
        {"LIVES", HUD_LIVES},
        {"WEAPON", HUD_WEAPON},
    };

    f32 gx = 20.0f;
    for (const HudGroup& g : groups)
    {
        const char* value = Engine::UI::GetText(g.Key, "0");
        f32 gw = Engine::UI::Measure(g.Label, 12.0f, 0.0f);
        f32 vw = Engine::UI::Measure(value, 26.0f, 0.0f);
        if (vw > gw)
        {
            gw = vw;
        }
        Engine::UI::DrawText(g.Label, gx, 14.0f, 12.0f, 0.0f,
                             Math::Color{0.50f, 0.72f, 0.91f, 1.0f}, LAYER_HUD, false, 0.25f);
        Engine::UI::DrawText(value, gx, 36.0f, 26.0f, 0.0f,
                             Math::Color{0.81f, 0.91f, 1.0f, 1.0f}, LAYER_HUD, false, 0.35f);
        gx += gw + 28.0f;
    }

    if (Engine::UI::Visible(HUD_BOSS, false))
    {
        const char* name = Engine::UI::GetText(BOSS_NAME, "BOSS");
        Engine::UI::DrawText(name, halfW, 12.0f, 14.0f, 4.0f,
                             Math::Color{1.0f, 0.82f, 0.82f, 1.0f}, LAYER_BOSS, true, 0.6f);
        f32 barX = halfW - 280.0f;
        f32 barY = 36.0f;
        f32 barW = 560.0f;
        f32 barH = 14.0f;
        f32 ratio = Engine::UI::Fill(BOSS_FILL, 1.0f);
        ratio = Math::Clamp(ratio, 0.0f, 1.0f);

        Engine::UI::DrawRect({barX, barY, barW, barH}, Math::Color{0.16f, 0.0f, 0.0f, 0.7f}, LAYER_BOSS);
        Engine::UI::DrawRectOutline({barX, barY, barW, barH}, 2.0f, Math::Color{1.0f, 0.31f, 0.31f, 0.8f}, LAYER_BOSS);
        f32 fillW = (barW - 4.0f) * ratio;
        if (fillW > 0.0f)
        {
            Engine::UI::DrawRect({barX + 2.0f, barY + 2.0f, fillW, barH - 4.0f},
                                 Math::Color{0.95f, 0.28f, 0.16f, 1.0f}, LAYER_BOSS);
        }
    }

    if (Engine::UI::Visible("panel-title", true))
    {
        overlay();
        Engine::UI::DrawText("AETHER STRIKE", halfW, 250.0f, 64.0f, 10.0f,
                             Math::Color{0.75f, 0.90f, 1.0f, 1.0f}, LAYER_PANEL, true, 0.6f);
        Engine::UI::DrawText("VECTOR ASSAULT PROTOCOL", halfW, 330.0f, 18.0f, 3.0f,
                             Math::Color{0.62f, 0.83f, 1.0f, 1.0f}, LAYER_PANEL, true, 0.3f);
        Engine::UI::DrawText("MOVE: W A S D / ARROWS", halfW, 395.0f, 15.0f, 1.0f,
                             Math::Color{0.44f, 0.63f, 0.78f, 1.0f}, LAYER_PANEL, true);
        Engine::UI::DrawText("FIRE: J / SPACE   SLOW: SHIFT   PAUSE: P", halfW, 425.0f, 15.0f, 1.0f,
                             Math::Color{0.44f, 0.63f, 0.78f, 1.0f}, LAYER_PANEL, true);
        Engine::UI::DrawText("GAMEPAD: STICK / D-PAD MOVE   RT FIRE   LT SLOW   START PAUSE", halfW, 455.0f,
                             15.0f, 1.0f, Math::Color{0.44f, 0.63f, 0.78f, 1.0f}, LAYER_PANEL, true);

        const char* btn = "PRESS ENTER TO START";
        f32 pulse = 1.0f + 0.06f * sinf((f32)Platform::NowSeconds() * 3.93f);
        f32 bw = Engine::UI::Measure(btn, 18.0f, 3.0f) * pulse + 68.0f;
        f32 bh = 40.0f * pulse;
        Engine::UI::DrawRect({halfW - bw * 0.5f, 505.0f - bh * 0.5f, bw, bh},
                             Math::Color{0.75f, 0.90f, 1.0f, 0.9f}, LAYER_PANEL);
        Engine::UI::DrawText(btn, halfW, 505.0f, 18.0f, 3.0f,
                             Math::Color{0.04f, 0.08f, 0.13f, 1.0f}, LAYER_PANEL, true);
    }

    if (Engine::UI::Visible("panel-pause", false))
    {
        overlay();
        Engine::UI::DrawText("PAUSED", halfW, 290.0f, 46.0f, 8.0f,
                             Math::Color{1.0f, 0.85f, 0.63f, 1.0f}, LAYER_PANEL, true, 0.5f);
        Engine::UI::DrawText("PRESS P TO RESUME", halfW, 370.0f, 18.0f, 3.0f,
                             Math::Color{0.62f, 0.83f, 1.0f, 1.0f}, LAYER_PANEL, true);
    }

    if (Engine::UI::Visible("panel-gameover", false))
    {
        overlay();
        Engine::UI::DrawText("GAME OVER", halfW, 290.0f, 46.0f, 8.0f,
                             Math::Color{1.0f, 0.85f, 0.63f, 1.0f}, LAYER_PANEL, true, 0.5f);
        Engine::UI::DrawText(Engine::UI::GetText("gameover-score", "SCORE 0"), halfW, 370.0f, 20.0f, 2.0f,
                             Math::Color{0.91f, 0.96f, 1.0f, 1.0f}, LAYER_PANEL, true, 0.3f);
        Engine::UI::DrawText("PRESS ENTER TO RETRY", halfW, 430.0f, 15.0f, 1.0f,
                             Math::Color{0.44f, 0.63f, 0.78f, 1.0f}, LAYER_PANEL, true);
    }

    if (Engine::UI::Visible("panel-win", false))
    {
        overlay();
        Engine::UI::DrawText("MISSION COMPLETE", halfW, 290.0f, 46.0f, 8.0f,
                             Math::Color{1.0f, 0.85f, 0.63f, 1.0f}, LAYER_PANEL, true, 0.5f);
        Engine::UI::DrawText(Engine::UI::GetText("win-score", "SCORE 0"), halfW, 370.0f, 20.0f, 2.0f,
                             Math::Color{0.91f, 0.96f, 1.0f, 1.0f}, LAYER_PANEL, true, 0.3f);
        Engine::UI::DrawText("PRESS ENTER TO PLAY AGAIN", halfW, 430.0f, 15.0f, 1.0f,
                             Math::Color{0.44f, 0.63f, 0.78f, 1.0f}, LAYER_PANEL, true);
    }

    // Also draw active flash notifications.
    Engine::UI::Draw(renderer);

    Engine::UI::EndFrame();
}

}
