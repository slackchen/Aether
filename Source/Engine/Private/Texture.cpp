#include "Texture.h"
#include "Container/Array.h"

#include <cmath>

namespace Aether::Engine {

RefPtr<RHI::RHITexture> MakeTexture(RHI::RHIDevice* device, u32 width, u32 height, const PixelFill& fill)
{
    Array<u8> pixels;
    pixels.Resize(width * height * 4);
    for (u32 y = 0; y < height; y++)
    {
        for (u32 x = 0; x < width; x++)
        {
            fill(x, y, &pixels[(y * width + x) * 4]);
        }
    }

    RHI::TextureDesc desc;
    desc.Width = width;
    desc.Height = height;
    desc.Format = RHI::TextureFormat::RGBA8Unorm;
    desc.Usage = (u32)RHI::TextureUsage::Sampled | (u32)RHI::TextureUsage::CopyDst;
    desc.InitialData = pixels.Data();
    return device->CreateTexture(desc);
}

RefPtr<RHI::RHITexture> MakeGlowTexture(RHI::RHIDevice* device, u32 size)
{
    return MakeTexture(device, size, size, [size](u32 x, u32 y, u8 rgba[4])
    {
        f32 cx = (f32)x + 0.5f;
        f32 cy = (f32)y + 0.5f;
        f32 d = sqrtf((cx - size * 0.5f) * (cx - size * 0.5f) + (cy - size * 0.5f) * (cy - size * 0.5f));
        f32 t = d / (size * 0.5f);
        f32 intensity = expf(-t * t * 4.0f);
        u8 v = (u8)(255.0f * intensity);
        rgba[0] = v;
        rgba[1] = v;
        rgba[2] = v;
        rgba[3] = v;
    });
}

RefPtr<RHI::RHITexture> MakeCircleTexture(RHI::RHIDevice* device, u32 size, const Math::Color& color)
{
    f32 radius = size * 0.5f;
    f32 soft = radius * 0.18f;
    return MakeTexture(device, size, size, [size, radius, soft, color](u32 x, u32 y, u8 rgba[4])
    {
        f32 cx = (f32)x + 0.5f;
        f32 cy = (f32)y + 0.5f;
        f32 d = sqrtf((cx - size * 0.5f) * (cx - size * 0.5f) + (cy - size * 0.5f) * (cy - size * 0.5f));
        f32 a = 1.0f - (d - (radius - soft)) / soft;
        if (a < 0.0f) a = 0.0f;
        if (a > 1.0f) a = 1.0f;
        rgba[0] = (u8)(color.r * 255.0f);
        rgba[1] = (u8)(color.g * 255.0f);
        rgba[2] = (u8)(color.b * 255.0f);
        rgba[3] = (u8)(color.a * a * 255.0f);
    });
}

RefPtr<RHI::RHITexture> MakeSolidTexture(RHI::RHIDevice* device, u32 width, u32 height, const Math::Color& color)
{
    return MakeTexture(device, width, height, [width, height, color](u32 x, u32 y, u8 rgba[4])
    {
        (void)width; (void)height; (void)x; (void)y;
        rgba[0] = (u8)(color.r * 255.0f);
        rgba[1] = (u8)(color.g * 255.0f);
        rgba[2] = (u8)(color.b * 255.0f);
        rgba[3] = (u8)(color.a * 255.0f);
    });
}

}
