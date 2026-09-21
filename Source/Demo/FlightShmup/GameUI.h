#pragma once

#include "Core.h"

namespace Aether::Engine
{
class Renderer2D;
}

namespace Shmup::UI
{

void SetScore(Aether::i32 value);
void SetHiscore(Aether::i32 value);
void SetLives(Aether::i32 value);
void SetWeapon(Aether::i32 level);

void ShowScreen(const char* element, bool show);
void SetText(const char* element, const char* text);

void ShowBoss(const char* name);
void SetBossHp(Aether::f32 ratio);
void HideBoss();

void FlashMessage(const char* text, Aether::f32 duration = 1.5f);

void Draw(Aether::Engine::Renderer2D* renderer);

}
