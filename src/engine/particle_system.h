#pragma once

#include "core/platform.h"
#include "core/math.h"
#include "rhi/rhi.h"
#include "engine/sprite_batch.h"
#include <memory>
#include <vector>

namespace aether::engine {

struct Particle {
    Vec2 position{0.0f, 0.0f};
    Vec2 velocity{0.0f, 0.0f};
    f32 life = 1.0f;
    f32 max_life = 1.0f;
    f32 size = 8.0f;
    Color color{1.0f, 1.0f, 1.0f, 1.0f};
    f32 drag = 0.0f;
    f32 gravity = 0.0f;
};

struct ParticleBurst {
    Vec2 position{0.0f, 0.0f};
    Vec2 base_velocity{0.0f, 0.0f};
    f32 speed = 100.0f;
    f32 spread = 1.0f;
    f32 size = 8.0f;
    Color color{1.0f, 1.0f, 1.0f, 1.0f};
    f32 life = 0.5f;
    u32 count = 8;
    f32 drag = 0.0f;
    f32 gravity = 0.0f;
};

class ParticleSystem {
public:
    static constexpr u32 kMaxParticles = 4096;

    void set_texture(std::shared_ptr<rhi::RHITexture> texture);
    void emit(const ParticleBurst& burst);
    void update(f32 dt);
    void render(SpriteBatch& batch);

    u32 count() const { return static_cast<u32>(particles_.size()); }

private:
    std::shared_ptr<rhi::RHITexture> texture_;
    std::vector<Particle> particles_;
};

}
