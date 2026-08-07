#pragma once

#include "core/platform.h"
#include "core/math.h"
#include "rhi/rhi.h"
#include <functional>
#include <memory>
#include <vector>

namespace aether::engine {

using PixelFill = std::function<void(u32 x, u32 y, u8 rgba[4])>;

std::shared_ptr<rhi::RHITexture> make_texture(rhi::RHIDevice* device, u32 width, u32 height, const PixelFill& fill);
std::shared_ptr<rhi::RHITexture> make_glow_texture(rhi::RHIDevice* device, u32 size = 64);
std::shared_ptr<rhi::RHITexture> make_circle_texture(rhi::RHIDevice* device, u32 size, const Color& color);
std::shared_ptr<rhi::RHITexture> make_solid_texture(rhi::RHIDevice* device, u32 width, u32 height, const Color& color);

}
