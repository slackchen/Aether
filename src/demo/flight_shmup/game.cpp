#include "demo/flight_shmup/game.h"
#include "demo/flight_shmup/game_ui.h"
#include "engine/audio.h"
#include "engine/input.h"
#include "engine/ui.h"
#include "platform/platform.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

using namespace aether::engine;

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/em_js.h>
#endif

namespace shmup {

namespace {

f32 clampf(f32 v, f32 lo, f32 hi) { return v < lo ? lo : (v > hi ? hi : v); }
f32 lerpf(f32 a, f32 b, f32 t) { return a + (b - a) * t; }
f32 frand() { return (f32)rand() / (f32)RAND_MAX; }
f32 frand_range(f32 lo, f32 hi) { return lo + (hi - lo) * frand(); }

constexpr f32 kLogicalHeight = 720.0f;
constexpr f32 kPlayerStartY = 300.0f;

#ifndef __EMSCRIPTEN__

const char* hiscore_path() {
    const char* env = getenv("LOCALAPPDATA");
    static std::string path;
    if (path.empty()) {
        path = env ? env : "";
        path += "/aether_hiscore.txt";
    }
    return path.c_str();
}

i32 hiscore_load() {
    FILE* f = fopen(hiscore_path(), "rb");
    if (!f) return 0;
    i32 v = 0;
    if (fscanf(f, "%d", &v) != 1) v = 0;
    fclose(f);
    return v < 0 ? 0 : v;
}

void hiscore_save(i32 v) {
    FILE* f = fopen(hiscore_path(), "wb");
    if (!f) return;
    fprintf(f, "%d", v);
    fclose(f);
}

#endif

}

#ifdef __EMSCRIPTEN__
EM_JS(int, js_hiscore_load, (), {
    var v = parseInt(localStorage.getItem("aether_hiscore") || "0", 10);
    return isNaN(v) ? 0 : v;
});

EM_JS(void, js_hiscore_save, (int v), {
    localStorage.setItem("aether_hiscore", String(v));
});
#endif

Game::Game(aether::engine::Renderer2D* renderer, aether::engine::Timer* timer)
    : renderer_(renderer), timer_(timer) {
    load_hiscore();
    tex_ = make_game_textures(renderer_->device());
    trail_.set_texture(tex_.bullet_glow);
    particles_.set_texture(tex_.bullet_glow);

    f32 half_w = world_width() * 0.5f;
    waves_ = {
        {2.0f, EnemyKind::Grunt, -half_w * 0.4f},
        {4.0f, EnemyKind::Grunt, half_w * 0.25f},
        {5.0f, EnemyKind::Grunt, -half_w * 0.55f},
        {7.0f, EnemyKind::Zigzag, 0.0f},
        {9.0f, EnemyKind::Zigzag, -half_w * 0.5f},
        {9.5f, EnemyKind::Zigzag, half_w * 0.5f},
        {12.0f, EnemyKind::Grunt, -half_w * 0.25f},
        {13.0f, EnemyKind::Grunt, half_w * 0.35f},
        {15.0f, EnemyKind::Sniper, -half_w * 0.3f},
        {17.0f, EnemyKind::Zigzag, -half_w * 0.2f},
        {18.0f, EnemyKind::Zigzag, half_w * 0.3f},
        {20.0f, EnemyKind::Tank, 0.0f},
        {23.0f, EnemyKind::Grunt, -half_w * 0.55f},
        {24.0f, EnemyKind::Grunt, -half_w * 0.3f},
        {25.0f, EnemyKind::Grunt, half_w * 0.25f},
        {26.0f, EnemyKind::Grunt, half_w * 0.5f},
        {29.0f, EnemyKind::Sniper, -half_w * 0.5f},
        {31.0f, EnemyKind::Sniper, half_w * 0.5f},
        {34.0f, EnemyKind::Zigzag, 0.0f},
        {36.0f, EnemyKind::Zigzag, -half_w * 0.5f},
        {36.5f, EnemyKind::Zigzag, half_w * 0.5f},
        {39.0f, EnemyKind::Tank, -half_w * 0.35f},
        {41.0f, EnemyKind::Tank, half_w * 0.35f},
        {45.0f, EnemyKind::Grunt, -half_w * 0.2f},
        {46.0f, EnemyKind::Grunt, half_w * 0.1f},
        {47.0f, EnemyKind::Grunt, half_w * 0.4f},
        {48.0f, EnemyKind::Sniper, 0.0f},
        {52.0f, EnemyKind::Zigzag, -half_w * 0.4f},
        {54.0f, EnemyKind::Zigzag, half_w * 0.4f},
        {54.5f, EnemyKind::Zigzag, 0.0f},
        {56.0f, EnemyKind::Tank, 0.0f},
        {60.0f, EnemyKind::Grunt, -half_w * 0.5f},
        {61.0f, EnemyKind::Grunt, -half_w * 0.15f},
        {62.0f, EnemyKind::Grunt, half_w * 0.3f},
        {64.0f, EnemyKind::Sniper, -half_w * 0.3f},
        {68.0f, EnemyKind::Tank, -half_w * 0.25f},
        {70.0f, EnemyKind::Tank, half_w * 0.25f},
        {73.0f, EnemyKind::Zigzag, 0.0f},
        {75.0f, EnemyKind::Sniper, half_w * 0.4f},
    };
}

Game::~Game() = default;

f32 Game::world_width() const {
    return renderer_->aspect() * kLogicalHeight;
}

Vec2 Game::clamp_to_play_area(const Vec2& p) const {
    f32 half_w = world_width() * 0.5f;
    f32 half_h = kLogicalHeight * 0.5f;
    return {clampf(p.x, -half_w + 36.0f, half_w - 36.0f),
            clampf(p.y, -half_h + 30.0f, half_h - 20.0f)};
}

void Game::load_hiscore() {
#ifdef __EMSCRIPTEN__
    hiscore_ = js_hiscore_load();
#else
    hiscore_ = hiscore_load();
#endif
}

void Game::save_hiscore() {
#ifdef __EMSCRIPTEN__
    js_hiscore_save(hiscore_);
#else
    hiscore_save(hiscore_);
#endif
}

void Game::reset() {
    player_ = Player();
    player_.pos = {0.0f, kPlayerStartY};
    player_bullets_.clear();
    enemy_bullets_.clear();
    enemies_.clear();
    powerups_.clear();
    boss_ = Boss();
    score_ = 0;
    elapsed_ = 0.0f;
    wave_index_ = 0;
    shake_timer_ = 0.0f;
    shake_magnitude_ = 0.0f;
    ui::set_score(0);
    ui::set_lives(player_.lives);
    ui::set_weapon(player_.weapon);
    ui::hide_boss();
}

void Game::start_game() {
    reset();
    state_ = State::Playing;
    paused_ = false;
    ui::show_screen("panel-title", false);
    ui::show_screen("panel-gameover", false);
    ui::show_screen("panel-win", false);
    ui::show_screen("panel-pause", false);
    Audio::unlock();
    Audio::start_music(0);
}

void Game::end_level() {
    if (score_ > hiscore_) {
        hiscore_ = score_;
        save_hiscore();
    }
    ui::set_text("win-score", ("SCORE " + std::to_string(score_)).c_str());
    ui::show_screen("panel-win", true);
    ui::hide_boss();
    Audio::stop_music();
    state_ = State::Win;
}

void Game::update() {
    real_time_ += timer_->unscaled_delta();

    if (Input::was_pressed(Key::Pause)) {
        if (state_ == State::Playing || state_ == State::BossIntro || state_ == State::Boss) {
            paused_ = !paused_;
            ui::show_screen("panel-pause", paused_);
            if (paused_) Audio::stop_music();
            else Audio::start_music(state_ == State::Boss ? 1 : 0);
        }
    }
    if (paused_) return;

    switch (state_) {
        case State::Title: {
            ui::set_hiscore(hiscore_);
            if (Input::was_pressed(Key::Confirm)) {
                Audio::unlock();
                start_game();
            }
            break;
        }
        case State::Playing: {
            f32 dt = timer_->delta();
            elapsed_ += dt;
            update_player(dt);
            update_entities(dt);
            update_waves(dt);
            update_particles(dt);
            break;
        }
        case State::BossIntro: {
            f32 dt = timer_->delta();
            state_timer_ += dt;
            boss_.pos.y = lerpf(-160.0f, -140.0f, clampf(state_timer_ / 2.5f, 0.0f, 1.0f));
            update_particles(dt);
            if (state_timer_ >= 2.5f) {
                state_ = State::Boss;
                boss_.active = true;
                boss_.hp = boss_.max_hp;
                boss_.pos.y = -140.0f;
                ui::show_boss("REVENANT DREADNOUGHT");
                Audio::set_music_intensity(1);
            }
            break;
        }
        case State::Boss: {
            f32 dt = timer_->delta();
            boss_update(dt);
            update_player(dt);
            update_entities(dt);
            update_particles(dt);
            if (boss_.defeated) {
                end_level();
            }
            break;
        }
        case State::GameOver:
        case State::Win: {
            f32 dt = timer_->delta();
            update_particles(dt);
            if (Input::was_pressed(Key::Confirm)) {
                start_game();
            }
            break;
        }
    }

    ui::set_score(score_);
    ui::set_hiscore(hiscore_);
    ui::set_lives(player_.lives);
    ui::set_weapon(player_.weapon);
    if (state_ == State::Boss && boss_.active && !boss_.defeated) {
        ui::set_boss_hp((f32)boss_.hp / (f32)boss_.max_hp);
    }
    update_shake();
}

void Game::update_player(f32 dt) {
    if (!player_.alive) {
        player_.respawn_timer -= dt;
        if (player_.respawn_timer <= 0.0f) {
            player_.alive = true;
            player_.pos = {0.0f, kPlayerStartY};
            player_.vel = {0.0f, 0.0f};
            player_.invuln = 2.0f;
            player_.weapon = 1;
            player_.shield = false;
        }
        return;
    }

    if (player_.invuln > 0.0f) player_.invuln -= dt;

    Vec2 dir{0.0f, 0.0f};
    if (Input::is_down(Key::Up)) dir.y -= 1.0f;
    if (Input::is_down(Key::Down)) dir.y += 1.0f;
    if (Input::is_down(Key::Left)) dir.x -= 1.0f;
    if (Input::is_down(Key::Right)) dir.x += 1.0f;

    bool slow = Input::is_down(Key::Slow);
    f32 max_speed = slow ? 200.0f : 460.0f;
    if (dir.x != 0.0f || dir.y != 0.0f) {
        f32 len = sqrtf(dir.x * dir.x + dir.y * dir.y);
        dir.x /= len;
        dir.y /= len;
    }
    Vec2 target = {dir.x * max_speed, dir.y * max_speed};
    f32 k = 1.0f - expf(-10.0f * dt);
    player_.vel = {lerpf(player_.vel.x, target.x, k), lerpf(player_.vel.y, target.y, k)};
    player_.pos.x += player_.vel.x * dt;
    player_.pos.y += player_.vel.y * dt;
    player_.pos = clamp_to_play_area(player_.pos);

    f32 target_tilt = clampf(player_.vel.x * 0.0006f, -0.45f, 0.45f);
    player_.tilt = lerpf(player_.tilt, target_tilt, k);

    player_.fire_timer -= dt;
    if (player_.fire_timer <= 0.0f && Input::is_down(Key::Fire)) {
        player_fire();
        player_.fire_timer = 0.16f / (1.0f + 0.12f * (player_.weapon - 1));
    }

    trail_.emit({player_.pos + Vec2{0.0f, 26.0f}, player_.vel * -0.25f, 20.0f, 0.5f, 9.0f,
                 Color{0.3f, 0.8f, 1.0f, 0.9f}, 0.4f, 1, 1.2f, 0.0f});
}

void Game::player_fire() {
    const Vec2 p = player_.pos;
    const f32 speed = 950.0f;
    spawn_player_bullet(p + Vec2{0.0f, -30.0f}, Vec2{0.0f, -speed});
    if (player_.weapon >= 2) {
        f32 a = 0.18f;
        spawn_player_bullet(p + Vec2{0.0f, -26.0f}, Vec2{-sinf(a) * speed, -cosf(a) * speed});
        spawn_player_bullet(p + Vec2{0.0f, -26.0f}, Vec2{sinf(a) * speed, -cosf(a) * speed});
    }
    if (player_.weapon >= 3) {
        f32 a = 0.32f;
        spawn_player_bullet(p + Vec2{0.0f, -22.0f}, Vec2{-sinf(a) * speed, -cosf(a) * speed});
        spawn_player_bullet(p + Vec2{0.0f, -22.0f}, Vec2{sinf(a) * speed, -cosf(a) * speed});
    }
    Audio::play(Sfx::Laser);
    particles_.emit({p + Vec2{0.0f, -30.0f}, {0.0f, -60.0f}, 40.0f, 0.4f, 7.0f,
                     Color{0.5f, 0.95f, 1.0f, 1.0f}, 0.2f, 3, 1.0f, 0.0f});
}

void Game::spawn_player_bullet(const Vec2& pos, const Vec2& vel) {
    Bullet b;
    b.pos = pos;
    b.vel = vel;
    b.radius = 5.0f;
    b.friendly = true;
    b.color = Color{0.5f, 0.95f, 1.0f, 1.0f};
    player_bullets_.push_back(b);
}

void Game::spawn_enemy_bullet(const Vec2& pos, f32 angle, f32 speed, const Color& color) {
    Bullet b;
    b.pos = pos;
    b.vel = {cosf(angle) * speed, sinf(angle) * speed};
    b.radius = 6.0f;
    b.friendly = false;
    b.color = color;
    enemy_bullets_.push_back(b);
}

void Game::spawn_enemy(EnemyKind kind, f32 x) {
    Enemy e;
    e.kind = kind;
    e.pos = {x, -kLogicalHeight * 0.5f - 40.0f};
    e.base_x = x;
    switch (kind) {
        case EnemyKind::Grunt:
            e.radius = 16.0f;
            e.hp = e.max_hp = 2;
            e.score = 100;
            e.color = Color{1.0f, 0.45f, 0.3f, 1.0f};
            break;
        case EnemyKind::Zigzag:
            e.radius = 17.0f;
            e.hp = e.max_hp = 3;
            e.score = 200;
            e.color = Color{1.0f, 0.65f, 0.15f, 1.0f};
            break;
        case EnemyKind::Sniper:
            e.radius = 20.0f;
            e.hp = e.max_hp = 8;
            e.score = 300;
            e.color = Color{0.7f, 0.45f, 1.0f, 1.0f};
            break;
        case EnemyKind::Tank:
            e.radius = 30.0f;
            e.hp = e.max_hp = 20;
            e.score = 500;
            e.color = Color{0.55f, 0.75f, 0.6f, 1.0f};
            break;
    }
    enemies_.push_back(e);
}

void Game::spawn_powerup(const Vec2& pos) {
    PowerUp pu;
    pu.pos = pos;
    powerups_.push_back(pu);
}

void Game::enemy_fire(Enemy& e) {
    Vec2 to_player = player_.alive ? (player_.pos - e.pos) : Vec2{0.0f, 1.0f};
    f32 base_angle = atan2f(to_player.y, to_player.x);
    Color c = Color{1.0f, 0.35f, 0.5f, 1.0f};

    switch (e.kind) {
        case EnemyKind::Grunt:
            spawn_enemy_bullet(e.pos, base_angle, 300.0f, c);
            break;
        case EnemyKind::Zigzag:
            for (i32 i = -1; i <= 1; i++) {
                spawn_enemy_bullet(e.pos, base_angle + i * 0.2f, 280.0f, c);
            }
            break;
        case EnemyKind::Sniper:
            for (i32 i = -2; i <= 2; i++) {
                spawn_enemy_bullet(e.pos, base_angle + i * 0.13f, 340.0f, c);
            }
            break;
        case EnemyKind::Tank:
            for (i32 i = -2; i <= 2; i++) {
                spawn_enemy_bullet(e.pos, base_angle + i * 0.24f, 250.0f, c);
            }
            break;
    }
}

void Game::boss_fire_pattern1(f32 dt) {
    boss_.fire_timer -= dt;
    if (boss_.fire_timer <= 0.0f) {
        boss_.fire_timer = 1.1f;
        Vec2 to_player = player_.alive ? (player_.pos - boss_.pos) : Vec2{0.0f, 1.0f};
        f32 base = atan2f(to_player.y, to_player.x);
        for (i32 i = -1; i <= 1; i++) {
            spawn_enemy_bullet(boss_.pos, base + i * 0.16f, 260.0f, Color{1.0f, 0.3f, 0.4f, 1.0f});
        }
    }
}

void Game::boss_fire_pattern2(f32 dt) {
    boss_.spiral_timer -= dt;
    if (boss_.spiral_timer <= -3.0f) {
        boss_.spiral_timer = 2.0f;
    }
    if (boss_.spiral_timer > 0.0f) {
        static constexpr f32 kStep = 0.09f;
        boss_.timer -= dt;
        if (boss_.timer <= 0.0f) {
            boss_.timer = kStep;
            boss_.spiral_angle += 0.5f;
            f32 speed = 240.0f;
            spawn_enemy_bullet(boss_.pos, boss_.spiral_angle, speed, Color{1.0f, 0.5f, 0.8f, 1.0f});
            spawn_enemy_bullet(boss_.pos, boss_.spiral_angle + 3.14159f, speed, Color{1.0f, 0.5f, 0.8f, 1.0f});
        }
    }
}

void Game::boss_fire_pattern3(f32 dt) {
    boss_.fire_timer -= dt;
    if (boss_.fire_timer <= 0.0f) {
        boss_.fire_timer = 0.7f;
        Vec2 to_player = player_.alive ? (player_.pos - boss_.pos) : Vec2{0.0f, 1.0f};
        f32 base = atan2f(to_player.y, to_player.x);
        spawn_enemy_bullet(boss_.pos, base, 430.0f, Color{1.0f, 0.95f, 0.6f, 1.0f});
    }
}

void Game::boss_update(f32 dt) {
    if (!boss_.active) return;

    boss_.timer += dt;
    f32 half_w = world_width() * 0.5f;
    boss_.pos.x = sinf(boss_.timer * 0.7f) * half_w * 0.5f;

    f32 frac = (f32)boss_.hp / (f32)boss_.max_hp;
    boss_fire_pattern1(dt);
    if (frac < 0.66f) boss_fire_pattern2(dt);
    if (frac < 0.33f) boss_fire_pattern3(dt);

    if (boss_.hit_flash > 0.0f) boss_.hit_flash -= dt;
}

void Game::update_entities(f32 dt) {
    f32 half_h = kLogicalHeight * 0.5f;

    for (auto& b : player_bullets_) {
        b.pos.x += b.vel.x * dt;
        b.pos.y += b.vel.y * dt;
        if (b.pos.y < -half_h - 60.0f || b.pos.x < -half_w() - 60.0f || b.pos.x > half_w() + 60.0f) {
            b.dead = true;
        }
    }
    for (auto& b : enemy_bullets_) {
        b.pos.x += b.vel.x * dt;
        b.pos.y += b.vel.y * dt;
        if (b.pos.y > half_h + 80.0f) b.dead = true;
    }

    for (auto& e : enemies_) {
        e.timer += dt;
        e.hit_flash = e.hit_flash > 0.0f ? e.hit_flash - dt : 0.0f;
        switch (e.kind) {
            case EnemyKind::Grunt:
                e.vel = {0.0f, 140.0f};
                e.fire_timer -= dt;
                if (e.fire_timer <= 0.0f && player_.alive) {
                    enemy_fire(e);
                    e.fire_timer = 2.2f;
                }
                break;
            case EnemyKind::Zigzag:
                e.vel = {0.0f, 130.0f};
                e.pos.x = e.base_x + sinf(e.timer * 2.2f) * 80.0f;
                e.fire_timer -= dt;
                if (e.fire_timer <= 0.0f && player_.alive) {
                    enemy_fire(e);
                    e.fire_timer = 2.6f;
                }
                break;
            case EnemyKind::Sniper:
                if (e.pos.y < 150.0f) {
                    e.vel = {0.0f, 200.0f};
                } else {
                    e.vel = {0.0f, 0.0f};
                }
                e.fire_timer -= dt;
                if (e.fire_timer <= 0.0f && player_.alive) {
                    enemy_fire(e);
                    e.fire_timer = 3.2f;
                }
                break;
            case EnemyKind::Tank:
                e.vel = {0.0f, 60.0f};
                e.fire_timer -= dt;
                if (e.fire_timer <= 0.0f && player_.alive) {
                    enemy_fire(e);
                    e.fire_timer = 2.4f;
                }
                break;
        }
        e.pos.x += e.vel.x * dt;
        e.pos.y += e.vel.y * dt;
        if (e.pos.y > half_h + 90.0f) e.dead = true;
    }

    for (auto& pu : powerups_) {
        pu.timer += dt;
        pu.pos.y += 90.0f * dt;
        if (pu.pos.y > half_h + 60.0f) pu.dead = true;
    }

    handle_collisions();

    player_bullets_.erase(std::remove_if(player_bullets_.begin(), player_bullets_.end(),
                                         [](const Bullet& b) { return b.dead; }),
                          player_bullets_.end());
    enemy_bullets_.erase(std::remove_if(enemy_bullets_.begin(), enemy_bullets_.end(),
                                        [](const Bullet& b) { return b.dead; }),
                         enemy_bullets_.end());
    enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(),
                                  [](const Enemy& e) { return e.dead; }),
                   enemies_.end());
    powerups_.erase(std::remove_if(powerups_.begin(), powerups_.end(),
                                   [](const PowerUp& p) { return p.dead; }),
                    powerups_.end());
}

f32 Game::half_w() const {
    return world_width() * 0.5f;
}

void Game::handle_collisions() {
    for (auto& pb : player_bullets_) {
        if (pb.dead) continue;
        for (auto& e : enemies_) {
            if (e.dead) continue;
            f32 dx = pb.pos.x - e.pos.x, dy = pb.pos.y - e.pos.y;
            f32 rr = pb.radius + e.radius;
            if (dx * dx + dy * dy < rr * rr) {
                pb.dead = true;
                e.hp -= 1;
                e.hit_flash = 0.08f;
                Audio::play(Sfx::Hit);
                if (e.hp <= 0) {
                    e.dead = true;
                    add_score(e.score);
                    explode(e.pos, e.color, 26, 380.0f, 13.0f);
                    timer_->add_hit_stop(0.05f);
                    shake(6.0f);
                    if (frand() < 0.18f) spawn_powerup(e.pos);
                }
                break;
            }
        }
        if (!pb.dead && boss_.active && !boss_.defeated) {
            f32 dx = pb.pos.x - boss_.pos.x, dy = pb.pos.y - boss_.pos.y;
            f32 rr = pb.radius + boss_.radius;
            if (dx * dx + dy * dy < rr * rr) {
                pb.dead = true;
                boss_.hp -= 1;
                boss_.hit_flash = 0.05f;
                Audio::play(Sfx::Hit);
                if (boss_.hp <= 0) {
                    boss_.defeated = true;
                    add_score(5000);
                    explode(boss_.pos, Color{0.9f, 0.5f, 0.9f, 1.0f}, 120, 700.0f, 26.0f);
                    shake(22.0f);
                    timer_->add_hit_stop(0.25f);
                }
            }
        }
    }

    if (player_.alive && player_.invuln <= 0.0f) {
        for (auto& eb : enemy_bullets_) {
            f32 dx = eb.pos.x - player_.pos.x, dy = eb.pos.y - player_.pos.y;
            f32 rr = eb.radius + player_.radius;
            if (dx * dx + dy * dy < rr * rr) {
                eb.dead = true;
                player_hit();
                break;
            }
        }
        for (auto& e : enemies_) {
            f32 dx = e.pos.x - player_.pos.x, dy = e.pos.y - player_.pos.y;
            f32 rr = e.radius + player_.radius;
            if (dx * dx + dy * dy < rr * rr) {
                e.hp -= 5;
                e.hit_flash = 0.08f;
                if (e.hp <= 0) {
                    e.dead = true;
                    add_score(e.score);
                    explode(e.pos, e.color, 20, 300.0f, 12.0f);
                }
                player_hit();
                break;
            }
        }
        for (auto& pu : powerups_) {
            f32 dx = pu.pos.x - player_.pos.x, dy = pu.pos.y - player_.pos.y;
            if (dx * dx + dy * dy < 40.0f * 40.0f) {
                pu.dead = true;
                if (player_.weapon < 3) {
                    player_.weapon++;
                    ui::flash_message("WEAPON UP");
                } else {
                    player_.shield = true;
                    ui::flash_message("SHIELD UP");
                }
                add_score(200);
                Audio::play(Sfx::Powerup);
            }
        }
    }
}

void Game::update_waves(f32 dt) {
    (void)dt;
    while (wave_index_ < waves_.size() && elapsed_ >= waves_[wave_index_].t) {
        spawn_enemy(waves_[wave_index_].kind, waves_[wave_index_].x);
        wave_index_++;
    }
    if (wave_index_ >= waves_.size() && enemies_.empty() && !boss_.active && state_ == State::Playing) {
        state_ = State::BossIntro;
        state_timer_ = 0.0f;
        ui::flash_message("WARNING");
        Audio::play(Sfx::Warning);
    }
}

void Game::player_hit() {
    if (!player_.alive || player_.invuln > 0.0f) return;
    if (player_.shield) {
        player_.shield = false;
        Audio::play(Sfx::Hit);
        shake(8.0f);
        ui::flash_message("SHIELD DOWN");
        return;
    }
    player_.alive = false;
    player_.lives--;
    player_.respawn_timer = 1.4f;
    ui::set_lives(player_.lives);
    Audio::play(Sfx::Death);
    explode(player_.pos, Color{0.4f, 0.9f, 1.0f, 1.0f}, 70, 520.0f, 18.0f);
    timer_->add_hit_stop(0.15f);
    shake(14.0f);
    if (player_.lives <= 0) {
        if (score_ > hiscore_) {
            hiscore_ = score_;
            save_hiscore();
        }
        ui::set_text("gameover-score", ("SCORE " + std::to_string(score_)).c_str());
        ui::show_screen("panel-gameover", true);
        Audio::stop_music();
        state_ = State::GameOver;
    }
}

void Game::explode(const Vec2& pos, const Color& color, u32 count, f32 speed, f32 size) {
    particles_.emit({pos, {0.0f, 0.0f}, speed, 1.0f, size, color, 0.5f, count, 0.0f, 0.0f});
}

void Game::add_score(i32 amount) {
    score_ += amount;
    if (score_ > hiscore_) hiscore_ = score_;
}

void Game::shake(f32 magnitude) {
    shake_magnitude_ = magnitude > shake_magnitude_ ? magnitude : shake_magnitude_;
    shake_timer_ = 0.35f;
}

void Game::update_shake() {
    if (shake_timer_ > 0.0f) {
        shake_timer_ -= timer_->unscaled_delta();
        f32 m = shake_magnitude_ * (shake_timer_ / 0.35f);
        camera_base_.x = frand_range(-m, m);
        camera_base_.y = frand_range(-m, m);
    } else {
        camera_base_ = {0.0f, 0.0f};
        shake_magnitude_ = 0.0f;
    }
}

void Game::update_particles(f32 dt) {
    particles_.update(dt);
    trail_.update(dt);
}

void Game::render() {
    if (!renderer_->begin_frame()) return;

    auto* enc = renderer_->encoder();
    renderer_->starfield().render(enc, real_time_, state_ == State::Boss ? 1.6f : 1.0f,
                                  (f32)renderer_->width(), (f32)renderer_->height());

    auto& cam = renderer_->camera();
    cam.position = camera_base_;
    auto& batch = renderer_->sprites();

    if (player_.alive) {
        f32 blink = 1.0f;
        if (player_.invuln > 0.0f && fmodf(real_time_ * 12.0f, 1.0f) > 0.5f) blink = 0.35f;
        batch.add(tex_.player, player_.pos, vec2(64.0f, 64.0f), color(1.0f, 1.0f, 1.0f, blink),
                  player_.tilt, 10);
        if (player_.shield) {
            batch.add(tex_.bullet_glow, player_.pos, vec2(130.0f, 130.0f),
                      color(0.4f, 0.9f, 1.0f, 0.5f), 0.0f, 9, rhi::BlendMode::Additive);
        }
    }

    for (const auto& b : player_bullets_) {
        batch.add(tex_.bullet_glow, b.pos, vec2(14.0f, 26.0f), b.color, 0.0f, 4);
    }
    for (const auto& b : enemy_bullets_) {
        batch.add(tex_.bullet_glow, b.pos, vec2(24.0f, 24.0f), b.color, 0.0f, 4);
    }

    for (const auto& e : enemies_) {
        Color c = e.hit_flash > 0.0f ? color(1.0f, 1.0f, 1.0f, 1.0f) : e.color;
        Vec2 size{48.0f, 48.0f};
        std::shared_ptr<rhi::RHITexture> tex = tex_.grunt;
        f32 rot = 0.0f;
        switch (e.kind) {
            case EnemyKind::Grunt: tex = tex_.grunt; size = {48.0f, 48.0f}; break;
            case EnemyKind::Zigzag: tex = tex_.zigzag; size = {48.0f, 48.0f}; rot = e.timer * 2.0f; break;
            case EnemyKind::Sniper: tex = tex_.sniper; size = {56.0f, 56.0f}; break;
            case EnemyKind::Tank: tex = tex_.tank; size = {72.0f, 72.0f}; break;
        }
        batch.add(std::move(tex), e.pos, size, c, rot, 5);
    }

    if (boss_.active && !boss_.defeated) {
        Color c = boss_.hit_flash > 0.0f ? color(1.0f, 1.0f, 1.0f, 1.0f) : color(1.0f, 1.0f, 1.0f, 1.0f);
        batch.add(tex_.bullet_glow, boss_.pos, vec2(260.0f, 260.0f), color(0.7f, 0.3f, 0.9f, 0.35f), 0.0f, 7,
                  rhi::BlendMode::Additive);
        batch.add(tex_.boss, boss_.pos, vec2(192.0f, 128.0f), c, 0.0f, 8);
    }

    for (const auto& pu : powerups_) {
        f32 pulse = 0.7f + 0.3f * sinf(real_time_ * 6.0f);
        batch.add(tex_.powerup, pu.pos, vec2(40.0f, 40.0f), color(1.0f, 1.0f, 1.0f, pulse),
                  pu.timer * 1.5f, 6);
    }

    trail_.render(batch);
    particles_.render(batch);

    aether::engine::ui::draw(renderer_);

    batch.render(enc, cam.view_projection(renderer_->aspect()));

    renderer_->end_frame();
}

}
