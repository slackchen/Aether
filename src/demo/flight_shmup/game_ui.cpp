#include "demo/flight_shmup/game_ui.h"
#include "engine/ui.h"
#include "engine/renderer2d.h"
#include "platform/platform.h"
#include <string>
#include <cmath>
#include <algorithm>

using namespace aether;

namespace shmup::ui {

namespace {

constexpr const char* kHudScore = "hud-score";
constexpr const char* kHudHiscore = "hud-hiscore";
constexpr const char* kHudLives = "hud-lives";
constexpr const char* kHudWeapon = "hud-weapon";
constexpr const char* kHudBoss = "hud-boss";
constexpr const char* kBossName = "boss-name";
constexpr const char* kBossFill = "boss-fill";

constexpr aether::i32 kLayerHud = 20;
constexpr aether::i32 kLayerBoss = 21;
constexpr aether::i32 kLayerPanel = 30;

}

void set_score(aether::i32 value) {
    aether::engine::ui::set_text(kHudScore, std::to_string(value).c_str());
}

void set_hiscore(aether::i32 value) {
    aether::engine::ui::set_text(kHudHiscore, std::to_string(value).c_str());
}

void set_lives(aether::i32 value) {
    aether::engine::ui::set_text(kHudLives, std::to_string(value).c_str());
}

void set_weapon(aether::i32 level) {
    aether::engine::ui::set_text(kHudWeapon, std::to_string(level).c_str());
}

void show_screen(const char* element, bool show) {
    aether::engine::ui::set_visible(element, show);
}

void set_text(const char* element, const char* text) {
    aether::engine::ui::set_text(element, text);
}

void show_boss(const char* name) {
    aether::engine::ui::set_visible(kHudBoss, true);
    aether::engine::ui::set_text(kBossName, name);
}

void set_boss_hp(aether::f32 ratio) {
    aether::engine::ui::set_fill(kBossFill, ratio);
}

void hide_boss() {
    aether::engine::ui::set_visible(kHudBoss, false);
}

void flash_message(const char* text, aether::f32 duration) {
    aether::engine::ui::flash(text, duration);
}

void draw(aether::engine::Renderer2D* renderer) {
    if (!renderer) return;

    aether::engine::ui::begin_frame(renderer);

    f32 half_h = 360.0f;
    f32 half_w = half_h * renderer->aspect();

    auto overlay = [&]() {
        aether::engine::ui::draw_rect({0.0f, 0.0f, half_w * 2.0f, half_h * 2.0f},
                                      aether::color(0.0f, 0.0f, 0.0f, 0.8f), kLayerPanel);
    };

    struct HudGroup {
        const char* label;
        const char* key;
    };
    const HudGroup groups[] = {
        {"SCORE", kHudScore},
        {"HI-SCORE", kHudHiscore},
        {"LIVES", kHudLives},
        {"WEAPON", kHudWeapon},
    };

    f32 gx = 20.0f;
    for (const auto& g : groups) {
        const char* value = aether::engine::ui::get_text(g.key, "0");
        f32 gw = aether::engine::ui::measure(g.label, 12.0f, 0.0f);
        f32 vw = aether::engine::ui::measure(value, 26.0f, 0.0f);
        if (vw > gw) gw = vw;
        aether::engine::ui::draw_text(g.label, gx, 14.0f, 12.0f, 0.0f,
                                      aether::color(0.50f, 0.72f, 0.91f, 1.0f), kLayerHud, false, 0.25f);
        aether::engine::ui::draw_text(value, gx, 36.0f, 26.0f, 0.0f,
                                      aether::color(0.81f, 0.91f, 1.0f, 1.0f), kLayerHud, false, 0.35f);
        gx += gw + 28.0f;
    }

    if (aether::engine::ui::visible(kHudBoss, false)) {
        const char* name = aether::engine::ui::get_text(kBossName, "BOSS");
        aether::engine::ui::draw_text(name, half_w, 12.0f, 14.0f, 4.0f,
                                      aether::color(1.0f, 0.82f, 0.82f, 1.0f), kLayerBoss, true, 0.6f);
        f32 bar_x = half_w - 280.0f;
        f32 bar_y = 36.0f;
        f32 bar_w = 560.0f;
        f32 bar_h = 14.0f;
        f32 ratio = aether::engine::ui::fill(kBossFill, 1.0f);
        ratio = std::clamp(ratio, 0.0f, 1.0f);

        aether::engine::ui::draw_rect({bar_x, bar_y, bar_w, bar_h}, aether::color(0.16f, 0.0f, 0.0f, 0.7f), kLayerBoss);
        aether::engine::ui::draw_rect_outline({bar_x, bar_y, bar_w, bar_h}, 2.0f, aether::color(1.0f, 0.31f, 0.31f, 0.8f), kLayerBoss);
        f32 fill_w = (bar_w - 4.0f) * ratio;
        if (fill_w > 0.0f) {
            aether::engine::ui::draw_rect({bar_x + 2.0f, bar_y + 2.0f, fill_w, bar_h - 4.0f},
                                          aether::color(0.95f, 0.28f, 0.16f, 1.0f), kLayerBoss);
        }
    }

    if (aether::engine::ui::visible("panel-title", true)) {
        overlay();
        aether::engine::ui::draw_text("AETHER STRIKE", half_w, 250.0f, 64.0f, 10.0f,
                                      aether::color(0.75f, 0.90f, 1.0f, 1.0f), kLayerPanel, true, 0.6f);
        aether::engine::ui::draw_text("VECTOR ASSAULT PROTOCOL", half_w, 330.0f, 18.0f, 3.0f,
                                      aether::color(0.62f, 0.83f, 1.0f, 1.0f), kLayerPanel, true, 0.3f);
        aether::engine::ui::draw_text("MOVE: W A S D / ARROWS", half_w, 395.0f, 15.0f, 1.0f,
                                      aether::color(0.44f, 0.63f, 0.78f, 1.0f), kLayerPanel, true);
        aether::engine::ui::draw_text("FIRE: J / SPACE   SLOW: SHIFT   PAUSE: P", half_w, 425.0f, 15.0f, 1.0f,
                                      aether::color(0.44f, 0.63f, 0.78f, 1.0f), kLayerPanel, true);
        aether::engine::ui::draw_text("GAMEPAD: STICK / D-PAD MOVE   RT FIRE   LT SLOW   START PAUSE", half_w, 455.0f,
                                      15.0f, 1.0f, aether::color(0.44f, 0.63f, 0.78f, 1.0f), kLayerPanel, true);

        const char* btn = "PRESS ENTER TO START";
        f32 pulse = 1.0f + 0.06f * sinf((f32)aether::platform::now_seconds() * 3.93f);
        f32 bw = aether::engine::ui::measure(btn, 18.0f, 3.0f) * pulse + 68.0f;
        f32 bh = 40.0f * pulse;
        aether::engine::ui::draw_rect({half_w - bw * 0.5f, 505.0f - bh * 0.5f, bw, bh},
                                      aether::color(0.75f, 0.90f, 1.0f, 0.9f), kLayerPanel);
        aether::engine::ui::draw_text(btn, half_w, 505.0f, 18.0f, 3.0f,
                                      aether::color(0.04f, 0.08f, 0.13f, 1.0f), kLayerPanel, true);
    }

    if (aether::engine::ui::visible("panel-pause", false)) {
        overlay();
        aether::engine::ui::draw_text("PAUSED", half_w, 290.0f, 46.0f, 8.0f,
                                      aether::color(1.0f, 0.85f, 0.63f, 1.0f), kLayerPanel, true, 0.5f);
        aether::engine::ui::draw_text("PRESS P TO RESUME", half_w, 370.0f, 18.0f, 3.0f,
                                      aether::color(0.62f, 0.83f, 1.0f, 1.0f), kLayerPanel, true);
    }

    if (aether::engine::ui::visible("panel-gameover", false)) {
        overlay();
        aether::engine::ui::draw_text("GAME OVER", half_w, 290.0f, 46.0f, 8.0f,
                                      aether::color(1.0f, 0.85f, 0.63f, 1.0f), kLayerPanel, true, 0.5f);
        aether::engine::ui::draw_text(aether::engine::ui::get_text("gameover-score", "SCORE 0"), half_w, 370.0f, 20.0f, 2.0f,
                                      aether::color(0.91f, 0.96f, 1.0f, 1.0f), kLayerPanel, true, 0.3f);
        aether::engine::ui::draw_text("PRESS ENTER TO RETRY", half_w, 430.0f, 15.0f, 1.0f,
                                      aether::color(0.44f, 0.63f, 0.78f, 1.0f), kLayerPanel, true);
    }

    if (aether::engine::ui::visible("panel-win", false)) {
        overlay();
        aether::engine::ui::draw_text("MISSION COMPLETE", half_w, 290.0f, 46.0f, 8.0f,
                                      aether::color(1.0f, 0.85f, 0.63f, 1.0f), kLayerPanel, true, 0.5f);
        aether::engine::ui::draw_text(aether::engine::ui::get_text("win-score", "SCORE 0"), half_w, 370.0f, 20.0f, 2.0f,
                                      aether::color(0.91f, 0.96f, 1.0f, 1.0f), kLayerPanel, true, 0.3f);
        aether::engine::ui::draw_text("PRESS ENTER TO PLAY AGAIN", half_w, 430.0f, 15.0f, 1.0f,
                                      aether::color(0.44f, 0.63f, 0.78f, 1.0f), kLayerPanel, true);
    }

    // Also draw active flash notifications
    aether::engine::ui::draw(renderer);

    aether::engine::ui::end_frame();
}

}