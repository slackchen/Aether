#pragma once

#include "core/platform.h"
#include "core/math.h"
#include "rhi/rhi.h"
#include <memory>

using namespace aether;

namespace shmup {

struct GameTextures {
    std::shared_ptr<rhi::RHITexture> player;
    std::shared_ptr<rhi::RHITexture> grunt;
    std::shared_ptr<rhi::RHITexture> zigzag;
    std::shared_ptr<rhi::RHITexture> sniper;
    std::shared_ptr<rhi::RHITexture> tank;
    std::shared_ptr<rhi::RHITexture> boss;
    std::shared_ptr<rhi::RHITexture> powerup;
    std::shared_ptr<rhi::RHITexture> bullet_glow;
};

GameTextures make_game_textures(rhi::RHIDevice* device);

}
