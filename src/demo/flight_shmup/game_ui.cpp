#include "demo/flight_shmup/game_ui.h"
#include "engine/ui.h"
#include <string>

namespace shmup::ui {

namespace {

constexpr const char* kHudScore = "hud-score";
constexpr const char* kHudHiscore = "hud-hiscore";
constexpr const char* kHudLives = "hud-lives";
constexpr const char* kHudWeapon = "hud-weapon";
constexpr const char* kHudBoss = "hud-boss";
constexpr const char* kBossName = "boss-name";
constexpr const char* kBossFill = "boss-fill";

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

}