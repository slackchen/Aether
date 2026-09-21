#pragma once

#include "Art.h"
#include "Container/Array.h"
#include "Core.h"
#include "Math/Color.h"
#include "Math/Vec2.h"
#include "ParticleSystem.h"
#include "Renderer2D.h"
#include "Timer.h"

namespace Shmup
{

using Aether::Array;

enum class EnemyKind
{
    Grunt,
    Zigzag,
    Sniper,
    Tank,
};

struct Bullet
{
    Aether::Math::Vec2 Pos{0.0f, 0.0f};
    Aether::Math::Vec2 Vel{0.0f, 0.0f};
    Aether::f32 Radius = 5.0f;
    bool Friendly = false;
    Aether::Math::Color Color{1.0f, 1.0f, 1.0f, 1.0f};
    Aether::f32 Life = 5.0f;
    bool Dead = false;
};

struct Enemy
{
    EnemyKind Kind = EnemyKind::Grunt;
    Aether::Math::Vec2 Pos{0.0f, 0.0f};
    Aether::Math::Vec2 Vel{0.0f, 0.0f};
    Aether::f32 Radius = 16.0f;
    Aether::i32 Hp = 1;
    Aether::i32 MaxHp = 1;
    Aether::i32 Score = 100;
    Aether::f32 Timer = 0.0f;
    Aether::f32 FireTimer = 0.0f;
    Aether::f32 HitFlash = 0.0f;
    Aether::f32 BaseX = 0.0f;
    bool Dead = false;
    Aether::Math::Color Color{1.0f, 0.4f, 0.3f, 1.0f};
};

struct PowerUp
{
    Aether::Math::Vec2 Pos{0.0f, 0.0f};
    Aether::f32 Timer = 0.0f;
    bool Dead = false;
};

struct Player
{
    Aether::Math::Vec2 Pos{0.0f, 0.0f};
    Aether::Math::Vec2 Vel{0.0f, 0.0f};
    Aether::f32 Radius = 14.0f;
    Aether::i32 Lives = 3;
    Aether::i32 Weapon = 1;
    bool Alive = true;
    Aether::f32 RespawnTimer = 0.0f;
    Aether::f32 FireTimer = 0.0f;
    Aether::f32 Invuln = 0.0f;
    Aether::f32 Tilt = 0.0f;
    bool Shield = false;
};

struct Boss
{
    Aether::Math::Vec2 Pos{0.0f, -160.0f};
    Aether::f32 Radius = 70.0f;
    Aether::i32 Hp = 0;
    Aether::i32 MaxHp = 900;
    Aether::f32 Timer = 0.0f;
    Aether::f32 FireTimer = 0.0f;
    Aether::f32 SpiralAngle = 0.0f;
    Aether::f32 SpiralTimer = 0.0f;
    Aether::f32 HitFlash = 0.0f;
    bool Active = false;
    bool Defeated = false;
};

struct SpawnEvent
{
    Aether::f32 T = 0.0f;
    EnemyKind Kind = EnemyKind::Grunt;
    Aether::f32 X = 0.0f;
};

class Game
{
public:
    enum class State
    {
        Title,
        Playing,
        BossIntro,
        Boss,
        GameOver,
        Win,
    };

    Game(Aether::Engine::Renderer2D* renderer, Aether::Engine::Timer* timer);
    ~Game();

    void Update();
    void Render();

private:
    void Reset();
    void StartGame();
    void EndLevel();
    void PlayerHit();

    void SpawnEnemy(EnemyKind kind, Aether::f32 x);
    void SpawnPlayerBullet(const Aether::Math::Vec2& pos, const Aether::Math::Vec2& vel);
    void SpawnEnemyBullet(const Aether::Math::Vec2& pos, Aether::f32 angle, Aether::f32 speed,
                          const Aether::Math::Color& color);
    void SpawnPowerUp(const Aether::Math::Vec2& pos);
    void PlayerFire();
    void EnemyFire(Enemy& e);
    void BossUpdate(Aether::f32 dt);
    void BossFirePattern1(Aether::f32 dt);
    void BossFirePattern2(Aether::f32 dt);
    void BossFirePattern3(Aether::f32 dt);

    void UpdateEntities(Aether::f32 dt);
    void UpdatePlayer(Aether::f32 dt);
    void UpdateParticles(Aether::f32 dt);
    void UpdateShake();
    void HandleCollisions();
    void UpdateWaves(Aether::f32 dt);

    void Explode(const Aether::Math::Vec2& pos, const Aether::Math::Color& color, Aether::u32 count,
                 Aether::f32 speed, Aether::f32 size);
    void AddScore(Aether::i32 amount);
    void Shake(Aether::f32 magnitude);
    void SaveHiscore();
    void LoadHiscore();

    Aether::f32 WorldWidth() const;
    Aether::f32 HalfW() const;
    Aether::Math::Vec2 ClampToPlayArea(const Aether::Math::Vec2& p) const;

    Aether::Engine::Renderer2D* mRenderer = nullptr;
    Aether::Engine::Timer* mTimer = nullptr;

    State mState = State::Title;
    Aether::f32 mStateTimer = 0.0f;
    Aether::f32 mElapsed = 0.0f;
    Aether::f32 mRealTime = 0.0f;
    bool mPaused = false;

    Player mPlayer;
    Array<Bullet> mPlayerBullets;
    Array<Bullet> mEnemyBullets;
    Array<Enemy> mEnemies;
    Array<PowerUp> mPowerUps;
    Boss mBoss;

    Array<SpawnEvent> mWaves;
    Aether::u32 mWaveIndex = 0;

    Aether::i32 mScore = 0;
    Aether::i32 mHiscore = 0;
    Aether::f32 mShakeTimer = 0.0f;
    Aether::f32 mShakeMagnitude = 0.0f;
    Aether::Math::Vec2 mCameraBase{0.0f, 0.0f};

    Aether::Engine::ParticleSystem mParticles;
    Aether::Engine::ParticleSystem mTrail;

    GameTextures mTex;
};

}
