#include "engine/particle_system.h"
#include <algorithm>
#include <cmath>

namespace aether::engine {

namespace {
f32 frand() {
    return (f32)rand() / (f32)RAND_MAX;
}

f32 frand_range(f32 min, f32 max) {
    return min + (max - min) * frand();
}
}

void ParticleSystem::set_texture(std::shared_ptr<rhi::RHITexture> texture) {
    texture_ = std::move(texture);
}

void ParticleSystem::emit(const ParticleBurst& burst) {
    if (particles_.size() + burst.count > kMaxParticles) {
        particles_.erase(particles_.begin(), particles_.begin() + (particles_.size() + burst.count - kMaxParticles));
    }

    for (u32 i = 0; i < burst.count; i++) {
        f32 spread_rad = burst.spread * 0.5f;
        f32 a;
        if (burst.base_velocity.x != 0.0f || burst.base_velocity.y != 0.0f) {
            f32 theta = atan2f(burst.base_velocity.y, burst.base_velocity.x);
            a = theta + frand_range(-spread_rad, spread_rad);
        } else {
            a = frand_range(0.0f, 6.28318f);
        }
        f32 spd = burst.speed * (0.4f + 0.6f * frand());

        Particle p;
        p.position = burst.position;
        p.velocity = {cosf(a) * spd + burst.base_velocity.x, sinf(a) * spd + burst.base_velocity.y};
        p.life = burst.life * (0.6f + 0.8f * frand());
        p.max_life = p.life;
        p.size = burst.size * (0.6f + 0.8f * frand());
        p.color = burst.color;
        p.drag = burst.drag;
        p.gravity = burst.gravity;
        particles_.push_back(p);
    }
}

void ParticleSystem::update(f32 dt) {
    for (auto& p : particles_) {
        p.life -= dt;
        if (p.drag > 0.0f) {
            f32 factor = 1.0f / (1.0f + p.drag * dt);
            p.velocity.x *= factor;
            p.velocity.y *= factor;
        }
        p.velocity.y += p.gravity * dt;
        p.position.x += p.velocity.x * dt;
        p.position.y += p.velocity.y * dt;
    }
    particles_.erase(std::remove_if(particles_.begin(), particles_.end(), [](const Particle& p) {
                         return p.life <= 0.0f;
                     }),
                     particles_.end());
}

void ParticleSystem::render(SpriteBatch& batch) {
    if (!texture_) return;
    for (const auto& p : particles_) {
        f32 t = p.life / p.max_life;
        if (t > 1.0f) t = 1.0f;
        if (t < 0.0f) t = 0.0f;
        Color c = p.color;
        c.a *= t;
        batch.add(texture_, p.position, vec2(p.size, p.size), c, 0.0f, 0, rhi::BlendMode::Additive);
    }
}

}
