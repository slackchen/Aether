#pragma once

#include "Container/Function.h"
#include "Container/RefPtr.h"
#include "Core.h"
#include "Math/Color.h"
#include "RHI.h"

namespace Aether::Engine {

using PixelFill = Function<void(u32 x, u32 y, u8* rgba)>;

RefPtr<RHI::RHITexture> MakeTexture(RHI::RHIDevice* device, u32 width, u32 height, const PixelFill& fill);
RefPtr<RHI::RHITexture> MakeGlowTexture(RHI::RHIDevice* device, u32 size = 64);
RefPtr<RHI::RHITexture> MakeCircleTexture(RHI::RHIDevice* device, u32 size, const Math::Color& color);
RefPtr<RHI::RHITexture> MakeSolidTexture(RHI::RHIDevice* device, u32 width, u32 height, const Math::Color& color);

}
