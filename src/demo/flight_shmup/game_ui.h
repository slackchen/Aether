#pragma once

#include "core/platform.h"

namespace aether::engine {
class Renderer2D;
}

namespace shmup::ui {

void set_score(aether::i32 value);
void set_hiscore(aether::i32 value);
void set_lives(aether::i32 value);
void set_weapon(aether::i32 level);

void show_screen(const char* element, bool show);
void set_text(const char* element, const char* text);

void show_boss(const char* name);
void set_boss_hp(aether::f32 ratio);
void hide_boss();

void flash_message(const char* text, aether::f32 duration = 1.5f);

void draw(aether::engine::Renderer2D* renderer);

}

