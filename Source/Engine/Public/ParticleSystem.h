#pragma once

#include "Container/Array.h"
#include "Container/RefPtr.h"
#include "Core.h"
#include "Math/Color.h"
#include "Math/Vec2.h"
#include "RHI.h"
#include "SpriteBatch.h"

namespace Aether::Engine {

struct Particle
{
    Math::Vec2 Position{0.0f, 0.0f};
    Math::Vec2 Velocity{0.0f, 0.0f};
    f32 Life = 1.0f;
    f32 MaxLife = 1.0f;
    f32 Size = 8.0f;
    Math::Color Color{1.0f, 1.0f, 1.0f, 1.0f};
    f32 Drag = 0.0f;
    f32 Gravity = 0.0f;
};

struct ParticleBurst
{
    Math::Vec2 Position{0.0f, 0.0f};
    Math::Vec2 BaseVelocity{0.0f, 0.0f};
    f32 Speed = 100.0f;
    f32 Spread = 1.0f;
    f32 Size = 8.0f;
    Math::Color Color{1.0f, 1.0f, 1.0f, 1.0f};
    f32 Life = 0.5f;
    u32 Count = 8;
    f32 Drag = 0.0f;
    f32 Gravity = 0.0f;
};

class ParticleSystem
{
public:
    static constexpr u32 MAX_PARTICLES = 4096;

    void SetTexture(RefPtr<RHI::RHITexture> texture);
    void Emit(const ParticleBurst& burst);
    void Update(f32 dt);
    void Render(SpriteBatch& batch);

    u32 Count() const { return mParticles.Count(); }

private:
    RefPtr<RHI::RHITexture> mTexture;
    Array<Particle> mParticles;
};

}
