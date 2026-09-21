#pragma once

#include "Container/RefPtr.h"
#include "Core.h"
#include "RHI.h"

namespace Shmup
{

//
// Procedurally generated pixel-art texture set for the shmup demo.
//
struct GameTextures
{
    Aether::RefPtr<Aether::RHI::RHITexture> Player;
    Aether::RefPtr<Aether::RHI::RHITexture> Grunt;
    Aether::RefPtr<Aether::RHI::RHITexture> Zigzag;
    Aether::RefPtr<Aether::RHI::RHITexture> Sniper;
    Aether::RefPtr<Aether::RHI::RHITexture> Tank;
    Aether::RefPtr<Aether::RHI::RHITexture> Boss;
    Aether::RefPtr<Aether::RHI::RHITexture> Powerup;
    Aether::RefPtr<Aether::RHI::RHITexture> BulletGlow;
};

GameTextures MakeGameTextures(Aether::RHI::RHIDevice* device);

}
