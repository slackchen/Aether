#include "ParticleSystem.h"
#include "SpriteBatch.h"

#include <cmath>
#include <cstdlib>

namespace Aether::Engine {

namespace
{
f32 Frand()
{
    return (f32)rand() / (f32)RAND_MAX;
}

f32 FrandRange(f32 minValue, f32 maxValue)
{
    return minValue + (maxValue - minValue) * Frand();
}
}

void ParticleSystem::SetTexture(RefPtr<RHI::RHITexture> texture)
{
    mTexture = std::move(texture);
}

void ParticleSystem::Emit(const ParticleBurst& burst)
{
    if (mParticles.Count() + burst.Count > MAX_PARTICLES)
    {
        // Drop the oldest particles so the new burst fits.
        const u32 overflow = mParticles.Count() + burst.Count - MAX_PARTICLES;
        for (u32 i = 0; i < overflow; i++)
        {
            mParticles.RemoveAt(0);
        }
    }

    for (u32 i = 0; i < burst.Count; i++)
    {
        f32 spreadRad = burst.Spread * 0.5f;
        f32 a;
        if (burst.BaseVelocity.x != 0.0f || burst.BaseVelocity.y != 0.0f)
        {
            f32 theta = atan2f(burst.BaseVelocity.y, burst.BaseVelocity.x);
            a = theta + FrandRange(-spreadRad, spreadRad);
        }
        else
        {
            a = FrandRange(0.0f, 6.28318f);
        }
        f32 spd = burst.Speed * (0.4f + 0.6f * Frand());

        Particle& p = mParticles.EmplaceAdd();
        p.Position = burst.Position;
        p.Velocity = {cosf(a) * spd + burst.BaseVelocity.x, sinf(a) * spd + burst.BaseVelocity.y};
        p.Life = burst.Life * (0.6f + 0.8f * Frand());
        p.MaxLife = p.Life;
        p.Size = burst.Size * (0.6f + 0.8f * Frand());
        p.Color = burst.Color;
        p.Drag = burst.Drag;
        p.Gravity = burst.Gravity;
    }
}

void ParticleSystem::Update(f32 dt)
{
    for (Particle& p : mParticles)
    {
        p.Life -= dt;
        if (p.Drag > 0.0f)
        {
            f32 factor = 1.0f / (1.0f + p.Drag * dt);
            p.Velocity.x *= factor;
            p.Velocity.y *= factor;
        }
        p.Velocity.y += p.Gravity * dt;
        p.Position.x += p.Velocity.x * dt;
        p.Position.y += p.Velocity.y * dt;
    }
    mParticles.RemoveIf([](const Particle& p)
    {
        return p.Life <= 0.0f;
    });
}

void ParticleSystem::Render(SpriteBatch& batch)
{
    if (!mTexture) return;
    for (const Particle& p : mParticles)
    {
        f32 t = p.Life / p.MaxLife;
        if (t > 1.0f) t = 1.0f;
        if (t < 0.0f) t = 0.0f;
        Math::Color c = p.Color;
        c.a *= t;
        batch.Add(mTexture, p.Position, Math::Vec2(p.Size, p.Size), c, 0.0f, 0, RHI::BlendMode::Additive);
    }
}

}
