#include "Game.h"

#include "GameUI.h"
#include "Audio.h"
#include "Container/String.h"
#include "Input.h"
#include "Math/Math.h"
#include "Platform.h"
#include "Random.h"
#include "RHI.h"
#include "UI.h"

#include <cmath>
#include <cstdlib>

using namespace Aether;

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/em_js.h>
#endif

namespace Shmup
{

namespace
{

constexpr f32 LOGICAL_HEIGHT = 720.0f;
constexpr f32 PLAYER_START_Y = 300.0f;

// Shared deterministic RNG (replaces rand()/uniform distributions).
Random gRandom;

#ifndef __EMSCRIPTEN__

const char* HiscorePath()
{
    const char* env = getenv("LOCALAPPDATA");
    static String sPath;
    if (sPath.IsEmpty())
    {
        sPath = env ? env : "";
        sPath += "/aether_hiscore.txt";
    }
    return sPath.CStr();
}

i32 HiscoreLoad()
{
    FILE* f = fopen(HiscorePath(), "rb");
    if (!f)
    {
        return 0;
    }
    i32 v = 0;
    if (fscanf(f, "%d", &v) != 1)
    {
        v = 0;
    }
    fclose(f);
    return v < 0 ? 0 : v;
}

void HiscoreSave(i32 v)
{
    FILE* f = fopen(HiscorePath(), "wb");
    if (!f)
    {
        return;
    }
    fprintf(f, "%d", v);
    fclose(f);
}

#endif

}

#ifdef __EMSCRIPTEN__
EM_JS(int, JsHiscoreLoad, (), {
    var v = parseInt(localStorage.getItem("aether_hiscore") || "0", 10);
    return isNaN(v) ? 0 : v;
});

EM_JS(void, JsHiscoreSave, (int v), {
    localStorage.setItem("aether_hiscore", String(v));
});
#endif

Game::Game(Engine::Renderer2D* renderer, Engine::Timer* timer)
    : mRenderer(renderer)
    , mTimer(timer)
{
    LoadHiscore();
    mTex = MakeGameTextures(mRenderer->Device());
    mTrail.SetTexture(mTex.BulletGlow);
    mParticles.SetTexture(mTex.BulletGlow);

    f32 halfW = WorldWidth() * 0.5f;
    mWaves = {
        {2.0f, EnemyKind::Grunt, -halfW * 0.4f},
        {4.0f, EnemyKind::Grunt, halfW * 0.25f},
        {5.0f, EnemyKind::Grunt, -halfW * 0.55f},
        {7.0f, EnemyKind::Zigzag, 0.0f},
        {9.0f, EnemyKind::Zigzag, -halfW * 0.5f},
        {9.5f, EnemyKind::Zigzag, halfW * 0.5f},
        {12.0f, EnemyKind::Grunt, -halfW * 0.25f},
        {13.0f, EnemyKind::Grunt, halfW * 0.35f},
        {15.0f, EnemyKind::Sniper, -halfW * 0.3f},
        {17.0f, EnemyKind::Zigzag, -halfW * 0.2f},
        {18.0f, EnemyKind::Zigzag, halfW * 0.3f},
        {20.0f, EnemyKind::Tank, 0.0f},
        {23.0f, EnemyKind::Grunt, -halfW * 0.55f},
        {24.0f, EnemyKind::Grunt, -halfW * 0.3f},
        {25.0f, EnemyKind::Grunt, halfW * 0.25f},
        {26.0f, EnemyKind::Grunt, halfW * 0.5f},
        {29.0f, EnemyKind::Sniper, -halfW * 0.5f},
        {31.0f, EnemyKind::Sniper, halfW * 0.5f},
        {34.0f, EnemyKind::Zigzag, 0.0f},
        {36.0f, EnemyKind::Zigzag, -halfW * 0.5f},
        {36.5f, EnemyKind::Zigzag, halfW * 0.5f},
        {39.0f, EnemyKind::Tank, -halfW * 0.35f},
        {41.0f, EnemyKind::Tank, halfW * 0.35f},
        {45.0f, EnemyKind::Grunt, -halfW * 0.2f},
        {46.0f, EnemyKind::Grunt, halfW * 0.1f},
        {47.0f, EnemyKind::Grunt, halfW * 0.4f},
        {48.0f, EnemyKind::Sniper, 0.0f},
        {52.0f, EnemyKind::Zigzag, -halfW * 0.4f},
        {54.0f, EnemyKind::Zigzag, halfW * 0.4f},
        {54.5f, EnemyKind::Zigzag, 0.0f},
        {56.0f, EnemyKind::Tank, 0.0f},
        {60.0f, EnemyKind::Grunt, -halfW * 0.5f},
        {61.0f, EnemyKind::Grunt, -halfW * 0.15f},
        {62.0f, EnemyKind::Grunt, halfW * 0.3f},
        {64.0f, EnemyKind::Sniper, -halfW * 0.3f},
        {68.0f, EnemyKind::Tank, -halfW * 0.25f},
        {70.0f, EnemyKind::Tank, halfW * 0.25f},
        {73.0f, EnemyKind::Zigzag, 0.0f},
        {75.0f, EnemyKind::Sniper, halfW * 0.4f},
    };
}

Game::~Game() = default;

f32 Game::WorldWidth() const
{
    return mRenderer->Aspect() * LOGICAL_HEIGHT;
}

Math::Vec2 Game::ClampToPlayArea(const Math::Vec2& p) const
{
    f32 halfW = WorldWidth() * 0.5f;
    f32 halfH = LOGICAL_HEIGHT * 0.5f;
    return {Math::Clamp(p.x, -halfW + 36.0f, halfW - 36.0f),
            Math::Clamp(p.y, -halfH + 30.0f, halfH - 20.0f)};
}

void Game::LoadHiscore()
{
#ifdef __EMSCRIPTEN__
    mHiscore = JsHiscoreLoad();
#else
    mHiscore = HiscoreLoad();
#endif
}

void Game::SaveHiscore()
{
#ifdef __EMSCRIPTEN__
    JsHiscoreSave(mHiscore);
#else
    HiscoreSave(mHiscore);
#endif
}

void Game::Reset()
{
    mPlayer = Player();
    mPlayer.Pos = {0.0f, PLAYER_START_Y};
    mPlayerBullets.Clear();
    mEnemyBullets.Clear();
    mEnemies.Clear();
    mPowerUps.Clear();
    mBoss = Boss();
    mScore = 0;
    mElapsed = 0.0f;
    mWaveIndex = 0;
    mShakeTimer = 0.0f;
    mShakeMagnitude = 0.0f;
    UI::SetScore(0);
    UI::SetLives(mPlayer.Lives);
    UI::SetWeapon(mPlayer.Weapon);
    UI::HideBoss();
}

void Game::StartGame()
{
    Reset();
    mState = State::Playing;
    mPaused = false;
    UI::ShowScreen("panel-title", false);
    UI::ShowScreen("panel-gameover", false);
    UI::ShowScreen("panel-win", false);
    UI::ShowScreen("panel-pause", false);
    Engine::Audio::Unlock();
    Engine::Audio::SetMusicTrack(Engine::MusicTrack::FlightShmup);
    Engine::Audio::StartMusic(0);
}

void Game::EndLevel()
{
    if (mScore > mHiscore)
    {
        mHiscore = mScore;
        SaveHiscore();
    }
    UI::SetText("win-score", String::Format("SCORE %d", mScore).CStr());
    UI::ShowScreen("panel-win", true);
    UI::HideBoss();
    Engine::Audio::StopMusic();
    mState = State::Win;
}

void Game::Update()
{
    mRealTime += mTimer->UnscaledDelta();

    if (Engine::Input::WasPressed(Engine::Key::Pause))
    {
        if (mState == State::Playing || mState == State::BossIntro || mState == State::Boss)
        {
            mPaused = !mPaused;
            UI::ShowScreen("panel-pause", mPaused);
            if (mPaused)
            {
                Engine::Audio::StopMusic();
            }
            else
            {
                Engine::Audio::StartMusic(mState == State::Boss ? 1 : 0);
            }
        }
    }
    if (mPaused)
    {
        return;
    }

    switch (mState)
    {
        case State::Title:
        {
            UI::SetHiscore(mHiscore);
            if (Engine::Input::WasPressed(Engine::Key::Confirm))
            {
                Engine::Audio::Unlock();
                StartGame();
            }
            break;
        }
        case State::Playing:
        {
            f32 dt = mTimer->Delta();
            mElapsed += dt;
            UpdatePlayer(dt);
            UpdateEntities(dt);
            UpdateWaves(dt);
            UpdateParticles(dt);
            break;
        }
        case State::BossIntro:
        {
            f32 dt = mTimer->Delta();
            mStateTimer += dt;
            mBoss.Pos.y = Math::Lerp(-160.0f, -140.0f, Math::Clamp(mStateTimer / 2.5f, 0.0f, 1.0f));
            UpdateParticles(dt);
            if (mStateTimer >= 2.5f)
            {
                mState = State::Boss;
                mBoss.Active = true;
                mBoss.Hp = mBoss.MaxHp;
                mBoss.Pos.y = -140.0f;
                UI::ShowBoss("REVENANT DREADNOUGHT");
                Engine::Audio::SetMusicIntensity(1);
            }
            break;
        }
        case State::Boss:
        {
            f32 dt = mTimer->Delta();
            BossUpdate(dt);
            UpdatePlayer(dt);
            UpdateEntities(dt);
            UpdateParticles(dt);
            if (mBoss.Defeated)
            {
                EndLevel();
            }
            break;
        }
        case State::GameOver:
        case State::Win:
        {
            f32 dt = mTimer->Delta();
            UpdateParticles(dt);
            if (Engine::Input::WasPressed(Engine::Key::Confirm))
            {
                StartGame();
            }
            break;
        }
    }

    UI::SetScore(mScore);
    UI::SetHiscore(mHiscore);
    UI::SetLives(mPlayer.Lives);
    UI::SetWeapon(mPlayer.Weapon);
    if (mState == State::Boss && mBoss.Active && !mBoss.Defeated)
    {
        UI::SetBossHp((f32)mBoss.Hp / (f32)mBoss.MaxHp);
    }
    UpdateShake();
}

void Game::UpdatePlayer(f32 dt)
{
    if (!mPlayer.Alive)
    {
        mPlayer.RespawnTimer -= dt;
        if (mPlayer.RespawnTimer <= 0.0f)
        {
            mPlayer.Alive = true;
            mPlayer.Pos = {0.0f, PLAYER_START_Y};
            mPlayer.Vel = {0.0f, 0.0f};
            mPlayer.Invuln = 2.0f;
            mPlayer.Weapon = 1;
            mPlayer.Shield = false;
        }
        return;
    }

    if (mPlayer.Invuln > 0.0f)
    {
        mPlayer.Invuln -= dt;
    }

    Math::Vec2 dir{0.0f, 0.0f};
    if (Engine::Input::IsDown(Engine::Key::Up)) dir.y -= 1.0f;
    if (Engine::Input::IsDown(Engine::Key::Down)) dir.y += 1.0f;
    if (Engine::Input::IsDown(Engine::Key::Left)) dir.x -= 1.0f;
    if (Engine::Input::IsDown(Engine::Key::Right)) dir.x += 1.0f;

    bool slow = Engine::Input::IsDown(Engine::Key::Slow);
    f32 maxSpeed = slow ? 200.0f : 460.0f;
    if (dir.x != 0.0f || dir.y != 0.0f)
    {
        f32 len = sqrtf(dir.x * dir.x + dir.y * dir.y);
        dir.x /= len;
        dir.y /= len;
    }
    Math::Vec2 target = {dir.x * maxSpeed, dir.y * maxSpeed};
    f32 k = 1.0f - expf(-10.0f * dt);
    mPlayer.Vel = {Math::Lerp(mPlayer.Vel.x, target.x, k), Math::Lerp(mPlayer.Vel.y, target.y, k)};
    mPlayer.Pos.x += mPlayer.Vel.x * dt;
    mPlayer.Pos.y += mPlayer.Vel.y * dt;
    mPlayer.Pos = ClampToPlayArea(mPlayer.Pos);

    f32 targetTilt = Math::Clamp(mPlayer.Vel.x * 0.0006f, -0.45f, 0.45f);
    mPlayer.Tilt = Math::Lerp(mPlayer.Tilt, targetTilt, k);

    mPlayer.FireTimer -= dt;
    if (mPlayer.FireTimer <= 0.0f && Engine::Input::IsDown(Engine::Key::Fire))
    {
        PlayerFire();
        mPlayer.FireTimer = 0.16f / (1.0f + 0.12f * (mPlayer.Weapon - 1));
    }

    mTrail.Emit({mPlayer.Pos + Math::Vec2{0.0f, 26.0f}, mPlayer.Vel * -0.25f, 20.0f, 0.5f, 9.0f,
                 Math::Color{0.3f, 0.8f, 1.0f, 0.9f}, 0.4f, 1, 1.2f, 0.0f});
}

void Game::PlayerFire()
{
    const Math::Vec2 p = mPlayer.Pos;
    const f32 speed = 950.0f;
    SpawnPlayerBullet(p + Math::Vec2{0.0f, -30.0f}, Math::Vec2{0.0f, -speed});
    if (mPlayer.Weapon >= 2)
    {
        f32 a = 0.18f;
        SpawnPlayerBullet(p + Math::Vec2{0.0f, -26.0f}, Math::Vec2{-sinf(a) * speed, -cosf(a) * speed});
        SpawnPlayerBullet(p + Math::Vec2{0.0f, -26.0f}, Math::Vec2{sinf(a) * speed, -cosf(a) * speed});
    }
    if (mPlayer.Weapon >= 3)
    {
        f32 a = 0.32f;
        SpawnPlayerBullet(p + Math::Vec2{0.0f, -22.0f}, Math::Vec2{-sinf(a) * speed, -cosf(a) * speed});
        SpawnPlayerBullet(p + Math::Vec2{0.0f, -22.0f}, Math::Vec2{sinf(a) * speed, -cosf(a) * speed});
    }
    Engine::Audio::Play(Engine::Sfx::Laser);
    mParticles.Emit({p + Math::Vec2{0.0f, -30.0f}, {0.0f, -60.0f}, 40.0f, 0.4f, 7.0f,
                     Math::Color{0.5f, 0.95f, 1.0f, 1.0f}, 0.2f, 3, 1.0f, 0.0f});
}

void Game::SpawnPlayerBullet(const Math::Vec2& pos, const Math::Vec2& vel)
{
    Bullet b;
    b.Pos = pos;
    b.Vel = vel;
    b.Radius = 5.0f;
    b.Friendly = true;
    b.Color = Math::Color{0.5f, 0.95f, 1.0f, 1.0f};
    mPlayerBullets.Add(b);
}

void Game::SpawnEnemyBullet(const Math::Vec2& pos, f32 angle, f32 speed, const Math::Color& color)
{
    Bullet b;
    b.Pos = pos;
    b.Vel = {cosf(angle) * speed, sinf(angle) * speed};
    b.Radius = 6.0f;
    b.Friendly = false;
    b.Color = color;
    mEnemyBullets.Add(b);
}

void Game::SpawnEnemy(EnemyKind kind, f32 x)
{
    Enemy e;
    e.Kind = kind;
    e.Pos = {x, -LOGICAL_HEIGHT * 0.5f - 40.0f};
    e.BaseX = x;
    switch (kind)
    {
        case EnemyKind::Grunt:
            e.Radius = 16.0f;
            e.Hp = e.MaxHp = 2;
            e.Score = 100;
            e.Color = Math::Color{1.0f, 0.45f, 0.3f, 1.0f};
            break;
        case EnemyKind::Zigzag:
            e.Radius = 17.0f;
            e.Hp = e.MaxHp = 3;
            e.Score = 200;
            e.Color = Math::Color{1.0f, 0.65f, 0.15f, 1.0f};
            break;
        case EnemyKind::Sniper:
            e.Radius = 20.0f;
            e.Hp = e.MaxHp = 8;
            e.Score = 300;
            e.Color = Math::Color{0.7f, 0.45f, 1.0f, 1.0f};
            break;
        case EnemyKind::Tank:
            e.Radius = 30.0f;
            e.Hp = e.MaxHp = 20;
            e.Score = 500;
            e.Color = Math::Color{0.55f, 0.75f, 0.6f, 1.0f};
            break;
    }
    mEnemies.Add(e);
}

void Game::SpawnPowerUp(const Math::Vec2& pos)
{
    PowerUp pu;
    pu.Pos = pos;
    mPowerUps.Add(pu);
}

void Game::EnemyFire(Enemy& e)
{
    Math::Vec2 toPlayer = mPlayer.Alive ? (mPlayer.Pos - e.Pos) : Math::Vec2{0.0f, 1.0f};
    f32 baseAngle = atan2f(toPlayer.y, toPlayer.x);
    Math::Color c = Math::Color{1.0f, 0.35f, 0.5f, 1.0f};

    switch (e.Kind)
    {
        case EnemyKind::Grunt:
            SpawnEnemyBullet(e.Pos, baseAngle, 300.0f, c);
            break;
        case EnemyKind::Zigzag:
            for (i32 i = -1; i <= 1; i++)
            {
                SpawnEnemyBullet(e.Pos, baseAngle + i * 0.2f, 280.0f, c);
            }
            break;
        case EnemyKind::Sniper:
            for (i32 i = -2; i <= 2; i++)
            {
                SpawnEnemyBullet(e.Pos, baseAngle + i * 0.13f, 340.0f, c);
            }
            break;
        case EnemyKind::Tank:
            for (i32 i = -2; i <= 2; i++)
            {
                SpawnEnemyBullet(e.Pos, baseAngle + i * 0.24f, 250.0f, c);
            }
            break;
    }
}

void Game::BossFirePattern1(f32 dt)
{
    mBoss.FireTimer -= dt;
    if (mBoss.FireTimer <= 0.0f)
    {
        mBoss.FireTimer = 1.1f;
        Math::Vec2 toPlayer = mPlayer.Alive ? (mPlayer.Pos - mBoss.Pos) : Math::Vec2{0.0f, 1.0f};
        f32 base = atan2f(toPlayer.y, toPlayer.x);
        for (i32 i = -1; i <= 1; i++)
        {
            SpawnEnemyBullet(mBoss.Pos, base + i * 0.16f, 260.0f, Math::Color{1.0f, 0.3f, 0.4f, 1.0f});
        }
    }
}

void Game::BossFirePattern2(f32 dt)
{
    mBoss.SpiralTimer -= dt;
    if (mBoss.SpiralTimer <= -3.0f)
    {
        mBoss.SpiralTimer = 2.0f;
    }
    if (mBoss.SpiralTimer > 0.0f)
    {
        static constexpr f32 SPIRAL_STEP = 0.09f;
        mBoss.Timer -= dt;
        if (mBoss.Timer <= 0.0f)
        {
            mBoss.Timer = SPIRAL_STEP;
            mBoss.SpiralAngle += 0.5f;
            f32 speed = 240.0f;
            SpawnEnemyBullet(mBoss.Pos, mBoss.SpiralAngle, speed, Math::Color{1.0f, 0.5f, 0.8f, 1.0f});
            SpawnEnemyBullet(mBoss.Pos, mBoss.SpiralAngle + 3.14159f, speed, Math::Color{1.0f, 0.5f, 0.8f, 1.0f});
        }
    }
}

void Game::BossFirePattern3(f32 dt)
{
    mBoss.FireTimer -= dt;
    if (mBoss.FireTimer <= 0.0f)
    {
        mBoss.FireTimer = 0.7f;
        Math::Vec2 toPlayer = mPlayer.Alive ? (mPlayer.Pos - mBoss.Pos) : Math::Vec2{0.0f, 1.0f};
        f32 base = atan2f(toPlayer.y, toPlayer.x);
        SpawnEnemyBullet(mBoss.Pos, base, 430.0f, Math::Color{1.0f, 0.95f, 0.6f, 1.0f});
    }
}

void Game::BossUpdate(f32 dt)
{
    if (!mBoss.Active)
    {
        return;
    }

    mBoss.Timer += dt;
    f32 halfW = WorldWidth() * 0.5f;
    mBoss.Pos.x = sinf(mBoss.Timer * 0.7f) * halfW * 0.5f;

    f32 frac = (f32)mBoss.Hp / (f32)mBoss.MaxHp;
    BossFirePattern1(dt);
    if (frac < 0.66f) BossFirePattern2(dt);
    if (frac < 0.33f) BossFirePattern3(dt);

    if (mBoss.HitFlash > 0.0f)
    {
        mBoss.HitFlash -= dt;
    }
}

void Game::UpdateEntities(f32 dt)
{
    f32 halfH = LOGICAL_HEIGHT * 0.5f;

    for (Bullet& b : mPlayerBullets)
    {
        b.Pos.x += b.Vel.x * dt;
        b.Pos.y += b.Vel.y * dt;
        if (b.Pos.y < -halfH - 60.0f || b.Pos.x < -HalfW() - 60.0f || b.Pos.x > HalfW() + 60.0f)
        {
            b.Dead = true;
        }
    }
    for (Bullet& b : mEnemyBullets)
    {
        b.Pos.x += b.Vel.x * dt;
        b.Pos.y += b.Vel.y * dt;
        if (b.Pos.y > halfH + 80.0f) b.Dead = true;
    }

    for (Enemy& e : mEnemies)
    {
        e.Timer += dt;
        e.HitFlash = e.HitFlash > 0.0f ? e.HitFlash - dt : 0.0f;
        switch (e.Kind)
        {
            case EnemyKind::Grunt:
                e.Vel = {0.0f, 140.0f};
                e.FireTimer -= dt;
                if (e.FireTimer <= 0.0f && mPlayer.Alive)
                {
                    EnemyFire(e);
                    e.FireTimer = 2.2f;
                }
                break;
            case EnemyKind::Zigzag:
                e.Vel = {0.0f, 130.0f};
                e.Pos.x = e.BaseX + sinf(e.Timer * 2.2f) * 80.0f;
                e.FireTimer -= dt;
                if (e.FireTimer <= 0.0f && mPlayer.Alive)
                {
                    EnemyFire(e);
                    e.FireTimer = 2.6f;
                }
                break;
            case EnemyKind::Sniper:
                if (e.Pos.y < 150.0f)
                {
                    e.Vel = {0.0f, 200.0f};
                }
                else
                {
                    e.Vel = {0.0f, 0.0f};
                }
                e.FireTimer -= dt;
                if (e.FireTimer <= 0.0f && mPlayer.Alive)
                {
                    EnemyFire(e);
                    e.FireTimer = 3.2f;
                }
                break;
            case EnemyKind::Tank:
                e.Vel = {0.0f, 60.0f};
                e.FireTimer -= dt;
                if (e.FireTimer <= 0.0f && mPlayer.Alive)
                {
                    EnemyFire(e);
                    e.FireTimer = 2.4f;
                }
                break;
        }
        e.Pos.x += e.Vel.x * dt;
        e.Pos.y += e.Vel.y * dt;
        if (e.Pos.y > halfH + 90.0f) e.Dead = true;
    }

    for (PowerUp& pu : mPowerUps)
    {
        pu.Timer += dt;
        pu.Pos.y += 90.0f * dt;
        if (pu.Pos.y > halfH + 60.0f) pu.Dead = true;
    }

    HandleCollisions();

    mPlayerBullets.RemoveIf([](const Bullet& b) { return b.Dead; });
    mEnemyBullets.RemoveIf([](const Bullet& b) { return b.Dead; });
    mEnemies.RemoveIf([](const Enemy& e) { return e.Dead; });
    mPowerUps.RemoveIf([](const PowerUp& p) { return p.Dead; });
}

f32 Game::HalfW() const
{
    return WorldWidth() * 0.5f;
}

void Game::HandleCollisions()
{
    for (Bullet& pb : mPlayerBullets)
    {
        if (pb.Dead)
        {
            continue;
        }
        for (Enemy& e : mEnemies)
        {
            if (e.Dead)
            {
                continue;
            }
            f32 dx = pb.Pos.x - e.Pos.x, dy = pb.Pos.y - e.Pos.y;
            f32 rr = pb.Radius + e.Radius;
            if (dx * dx + dy * dy < rr * rr)
            {
                pb.Dead = true;
                e.Hp -= 1;
                e.HitFlash = 0.08f;
                Engine::Audio::Play(Engine::Sfx::Hit);
                if (e.Hp <= 0)
                {
                    e.Dead = true;
                    AddScore(e.Score);
                    Explode(e.Pos, e.Color, 26, 380.0f, 13.0f);
                    mTimer->AddHitStop(0.05f);
                    Shake(6.0f);
                    if (gRandom.NextF32() < 0.18f) SpawnPowerUp(e.Pos);
                }
                break;
            }
        }
        if (!pb.Dead && mBoss.Active && !mBoss.Defeated)
        {
            f32 dx = pb.Pos.x - mBoss.Pos.x, dy = pb.Pos.y - mBoss.Pos.y;
            f32 rr = pb.Radius + mBoss.Radius;
            if (dx * dx + dy * dy < rr * rr)
            {
                pb.Dead = true;
                mBoss.Hp -= 1;
                mBoss.HitFlash = 0.05f;
                Engine::Audio::Play(Engine::Sfx::Hit);
                if (mBoss.Hp <= 0)
                {
                    mBoss.Defeated = true;
                    AddScore(5000);
                    Explode(mBoss.Pos, Math::Color{0.9f, 0.5f, 0.9f, 1.0f}, 120, 700.0f, 26.0f);
                    Shake(22.0f);
                    mTimer->AddHitStop(0.25f);
                }
            }
        }
    }

    if (mPlayer.Alive && mPlayer.Invuln <= 0.0f)
    {
        for (Bullet& eb : mEnemyBullets)
        {
            f32 dx = eb.Pos.x - mPlayer.Pos.x, dy = eb.Pos.y - mPlayer.Pos.y;
            f32 rr = eb.Radius + mPlayer.Radius;
            if (dx * dx + dy * dy < rr * rr)
            {
                eb.Dead = true;
                PlayerHit();
                break;
            }
        }
        for (Enemy& e : mEnemies)
        {
            f32 dx = e.Pos.x - mPlayer.Pos.x, dy = e.Pos.y - mPlayer.Pos.y;
            f32 rr = e.Radius + mPlayer.Radius;
            if (dx * dx + dy * dy < rr * rr)
            {
                e.Hp -= 5;
                e.HitFlash = 0.08f;
                if (e.Hp <= 0)
                {
                    e.Dead = true;
                    AddScore(e.Score);
                    Explode(e.Pos, e.Color, 20, 300.0f, 12.0f);
                }
                PlayerHit();
                break;
            }
        }
        for (PowerUp& pu : mPowerUps)
        {
            f32 dx = pu.Pos.x - mPlayer.Pos.x, dy = pu.Pos.y - mPlayer.Pos.y;
            if (dx * dx + dy * dy < 40.0f * 40.0f)
            {
                pu.Dead = true;
                if (mPlayer.Weapon < 3)
                {
                    mPlayer.Weapon++;
                    UI::FlashMessage("WEAPON UP");
                }
                else
                {
                    mPlayer.Shield = true;
                    UI::FlashMessage("SHIELD UP");
                }
                AddScore(200);
                Engine::Audio::Play(Engine::Sfx::Powerup);
            }
        }
    }
}

void Game::UpdateWaves(f32 dt)
{
    (void)dt;
    while (mWaveIndex < mWaves.Count() && mElapsed >= mWaves[mWaveIndex].T)
    {
        SpawnEnemy(mWaves[mWaveIndex].Kind, mWaves[mWaveIndex].X);
        mWaveIndex++;
    }
    if (mWaveIndex >= mWaves.Count() && mEnemies.IsEmpty() && !mBoss.Active && mState == State::Playing)
    {
        mState = State::BossIntro;
        mStateTimer = 0.0f;
        UI::FlashMessage("WARNING");
        Engine::Audio::Play(Engine::Sfx::Warning);
    }
}

void Game::PlayerHit()
{
    if (!mPlayer.Alive || mPlayer.Invuln > 0.0f)
    {
        return;
    }
    if (mPlayer.Shield)
    {
        mPlayer.Shield = false;
        Engine::Audio::Play(Engine::Sfx::Hit);
        Shake(8.0f);
        UI::FlashMessage("SHIELD DOWN");
        return;
    }
    mPlayer.Alive = false;
    mPlayer.Lives--;
    mPlayer.RespawnTimer = 1.4f;
    UI::SetLives(mPlayer.Lives);
    Engine::Audio::Play(Engine::Sfx::Death);
    Explode(mPlayer.Pos, Math::Color{0.4f, 0.9f, 1.0f, 1.0f}, 70, 520.0f, 18.0f);
    mTimer->AddHitStop(0.15f);
    Shake(14.0f);
    if (mPlayer.Lives <= 0)
    {
        if (mScore > mHiscore)
        {
            mHiscore = mScore;
            SaveHiscore();
        }
        UI::SetText("gameover-score", String::Format("SCORE %d", mScore).CStr());
        UI::ShowScreen("panel-gameover", true);
        Engine::Audio::StopMusic();
        mState = State::GameOver;
    }
}

void Game::Explode(const Math::Vec2& pos, const Math::Color& color, u32 count, f32 speed, f32 size)
{
    mParticles.Emit({pos, {0.0f, 0.0f}, speed, 1.0f, size, color, 0.5f, count, 0.0f, 0.0f});
}

void Game::AddScore(i32 amount)
{
    mScore += amount;
    if (mScore > mHiscore) mHiscore = mScore;
}

void Game::Shake(f32 magnitude)
{
    mShakeMagnitude = magnitude > mShakeMagnitude ? magnitude : mShakeMagnitude;
    mShakeTimer = 0.35f;
}

void Game::UpdateShake()
{
    if (mShakeTimer > 0.0f)
    {
        mShakeTimer -= mTimer->UnscaledDelta();
        if (mShakeTimer < 0.0f) mShakeTimer = 0.0f;
        f32 m = mShakeMagnitude * (mShakeTimer / 0.35f);
        mCameraBase.x = gRandom.Range(-m, m);
        mCameraBase.y = gRandom.Range(-m, m);
    }
    else
    {
        mCameraBase = {0.0f, 0.0f};
        mShakeMagnitude = 0.0f;
    }
}

void Game::UpdateParticles(f32 dt)
{
    mParticles.Update(dt);
    mTrail.Update(dt);
}

void Game::Render()
{
    RHI::RHICommandEncoder* enc = mRenderer->Encoder();
    mRenderer->GetStarfield().Render(enc, mRealTime, mState == State::Boss ? 1.6f : 1.0f,
                                     (f32)mRenderer->Width(), (f32)mRenderer->Height());

    Engine::Camera2D& cam = mRenderer->GetCamera();
    cam.Position = mCameraBase;
    Engine::SpriteBatch& batch = mRenderer->Sprites();

    if (mPlayer.Alive)
    {
        f32 blink = 1.0f;
        if (mPlayer.Invuln > 0.0f && fmodf(mRealTime * 12.0f, 1.0f) > 0.5f) blink = 0.35f;
        batch.Add(mTex.Player, mPlayer.Pos, Math::Vec2{64.0f, 64.0f},
                  Math::Color{1.0f, 1.0f, 1.0f, blink}, mPlayer.Tilt, 10);
        if (mPlayer.Shield)
        {
            batch.Add(mTex.BulletGlow, mPlayer.Pos, Math::Vec2{130.0f, 130.0f},
                      Math::Color{0.4f, 0.9f, 1.0f, 0.5f}, 0.0f, 9, RHI::BlendMode::Additive);
        }
    }

    for (const Bullet& b : mPlayerBullets)
    {
        batch.Add(mTex.BulletGlow, b.Pos, Math::Vec2{14.0f, 26.0f}, b.Color, 0.0f, 4);
    }
    for (const Bullet& b : mEnemyBullets)
    {
        batch.Add(mTex.BulletGlow, b.Pos, Math::Vec2{24.0f, 24.0f}, b.Color, 0.0f, 4);
    }

    for (const Enemy& e : mEnemies)
    {
        Math::Color c = e.HitFlash > 0.0f ? Math::Color{1.0f, 1.0f, 1.0f, 1.0f} : e.Color;
        Math::Vec2 size{48.0f, 48.0f};
        RefPtr<RHI::RHITexture> tex = mTex.Grunt;
        f32 rot = 0.0f;
        switch (e.Kind)
        {
            case EnemyKind::Grunt: tex = mTex.Grunt; size = {48.0f, 48.0f}; break;
            case EnemyKind::Zigzag: tex = mTex.Zigzag; size = {48.0f, 48.0f}; rot = e.Timer * 2.0f; break;
            case EnemyKind::Sniper: tex = mTex.Sniper; size = {56.0f, 56.0f}; break;
            case EnemyKind::Tank: tex = mTex.Tank; size = {72.0f, 72.0f}; break;
        }
        batch.Add(std::move(tex), e.Pos, size, c, rot, 5);
    }

    if (mBoss.Active && !mBoss.Defeated)
    {
        Math::Color c = mBoss.HitFlash > 0.0f ? Math::Color{1.0f, 1.0f, 1.0f, 1.0f}
                                              : Math::Color{1.0f, 1.0f, 1.0f, 1.0f};
        batch.Add(mTex.BulletGlow, mBoss.Pos, Math::Vec2{260.0f, 260.0f},
                  Math::Color{0.7f, 0.3f, 0.9f, 0.35f}, 0.0f, 7, RHI::BlendMode::Additive);
        batch.Add(mTex.Boss, mBoss.Pos, Math::Vec2{192.0f, 128.0f}, c, 0.0f, 8);
    }

    for (const PowerUp& pu : mPowerUps)
    {
        f32 pulse = 0.7f + 0.3f * sinf(mRealTime * 6.0f);
        batch.Add(mTex.Powerup, pu.Pos, Math::Vec2{40.0f, 40.0f},
                  Math::Color{1.0f, 1.0f, 1.0f, pulse}, pu.Timer * 1.5f, 6);
    }

    mTrail.Render(batch);
    mParticles.Render(batch);

    batch.Render(enc, cam.ViewProjection(mRenderer->Aspect()));
}

}
