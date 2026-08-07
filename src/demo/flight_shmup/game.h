#pragma once

#include "core/platform.h"
#include "core/math.h"
#include "engine/particle_system.h"
#include "engine/renderer2d.h"
#include "engine/timer.h"
#include "rhi/rhi.h"
#include "demo/flight_shmup/art.h"
#include <memory>
#include <vector>

using namespace aether;

namespace shmup {

enum class EnemyKind {
    Grunt,
    Zigzag,
    Sniper,
    Tank,
};

struct Bullet {
    Vec2 pos{0.0f, 0.0f};
    Vec2 vel{0.0f, 0.0f};
    f32 radius = 5.0f;
    bool friendly = false;
    Color color{1.0f, 1.0f, 1.0f, 1.0f};
    f32 life = 5.0f;
    bool dead = false;
};

struct Enemy {
    EnemyKind kind = EnemyKind::Grunt;
    Vec2 pos{0.0f, 0.0f};
    Vec2 vel{0.0f, 0.0f};
    f32 radius = 16.0f;
    i32 hp = 1;
    i32 max_hp = 1;
    i32 score = 100;
    f32 timer = 0.0f;
    f32 fire_timer = 0.0f;
    f32 hit_flash = 0.0f;
    f32 base_x = 0.0f;
    bool dead = false;
    Color color{1.0f, 0.4f, 0.3f, 1.0f};
};

struct PowerUp {
    Vec2 pos{0.0f, 0.0f};
    f32 timer = 0.0f;
    bool dead = false;
};

struct Player {
    Vec2 pos{0.0f, 0.0f};
    Vec2 vel{0.0f, 0.0f};
    f32 radius = 14.0f;
    i32 lives = 3;
    i32 weapon = 1;
    bool alive = true;
    f32 respawn_timer = 0.0f;
    f32 fire_timer = 0.0f;
    f32 invuln = 0.0f;
    f32 tilt = 0.0f;
    bool shield = false;
};

struct Boss {
    Vec2 pos{0.0f, -160.0f};
    f32 radius = 70.0f;
    i32 hp = 0;
    i32 max_hp = 900;
    f32 timer = 0.0f;
    f32 fire_timer = 0.0f;
    f32 spiral_angle = 0.0f;
    f32 spiral_timer = 0.0f;
    f32 hit_flash = 0.0f;
    bool active = false;
    bool defeated = false;
};

struct SpawnEvent {
    f32 t = 0.0f;
    EnemyKind kind = EnemyKind::Grunt;
    f32 x = 0.0f;
};

class Game {
public:
    enum class State {
        Title,
        Playing,
        BossIntro,
        Boss,
        GameOver,
        Win,
    };

    Game(aether::engine::Renderer2D* renderer, aether::engine::Timer* timer);
    ~Game();

    void update();
    void render();

private:
    void reset();
    void start_game();
    void end_level();
    void player_hit();

    void spawn_enemy(EnemyKind kind, f32 x);
    void spawn_player_bullet(const Vec2& pos, const Vec2& vel);
    void spawn_enemy_bullet(const Vec2& pos, f32 angle, f32 speed, const Color& color);
    void spawn_powerup(const Vec2& pos);
    void player_fire();
    void enemy_fire(Enemy& e);
    void boss_update(f32 dt);
    void boss_fire_pattern1(f32 dt);
    void boss_fire_pattern2(f32 dt);
    void boss_fire_pattern3(f32 dt);

    void update_entities(f32 dt);
    void update_player(f32 dt);
    void update_particles(f32 dt);
    void update_shake();
    void handle_collisions();
    void update_waves(f32 dt);

    void explode(const Vec2& pos, const Color& color, u32 count, f32 speed, f32 size);
    void add_score(i32 amount);
    void shake(f32 magnitude);
    void save_hiscore();
    void load_hiscore();

    f32 world_width() const;
    f32 half_w() const;
    Vec2 clamp_to_play_area(const Vec2& p) const;

    aether::engine::Renderer2D* renderer_ = nullptr;
    aether::engine::Timer* timer_ = nullptr;

    State state_ = State::Title;
    f32 state_timer_ = 0.0f;
    f32 elapsed_ = 0.0f;
    f32 real_time_ = 0.0f;
    bool paused_ = false;

    Player player_;
    std::vector<Bullet> player_bullets_;
    std::vector<Bullet> enemy_bullets_;
    std::vector<Enemy> enemies_;
    std::vector<PowerUp> powerups_;
    Boss boss_;

    std::vector<SpawnEvent> waves_;
    size_t wave_index_ = 0;

    i32 score_ = 0;
    i32 hiscore_ = 0;
    f32 shake_timer_ = 0.0f;
    f32 shake_magnitude_ = 0.0f;
    Vec2 camera_base_{0.0f, 0.0f};

    aether::engine::ParticleSystem particles_;
    aether::engine::ParticleSystem trail_;

    GameTextures tex_;
};

}
