#include "engine/texture.h"
#include <cmath>
#include <cstring>

namespace aether::engine {

std::shared_ptr<rhi::RHITexture> make_texture(rhi::RHIDevice* device, u32 width, u32 height, const PixelFill& fill) {
    std::vector<u8> pixels(width * height * 4);
    for (u32 y = 0; y < height; y++) {
        for (u32 x = 0; x < width; x++) {
            fill(x, y, &pixels[(y * width + x) * 4]);
        }
    }

    rhi::TextureDesc desc;
    desc.width = width;
    desc.height = height;
    desc.format = rhi::TextureFormat::RGBA8Unorm;
    desc.usage = (u32)rhi::TextureUsage::Sampled | (u32)rhi::TextureUsage::CopyDst;
    desc.initial_data = pixels.data();
    return device->create_texture(desc);
}

std::shared_ptr<rhi::RHITexture> make_glow_texture(rhi::RHIDevice* device, u32 size) {
    return make_texture(device, size, size, [size](u32 x, u32 y, u8 rgba[4]) {
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

std::shared_ptr<rhi::RHITexture> make_circle_texture(rhi::RHIDevice* device, u32 size, const Color& color) {
    f32 radius = size * 0.5f;
    f32 soft = radius * 0.18f;
    return make_texture(device, size, size, [size, radius, soft, color](u32 x, u32 y, u8 rgba[4]) {
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

std::shared_ptr<rhi::RHITexture> make_solid_texture(rhi::RHIDevice* device, u32 width, u32 height, const Color& color) {
    return make_texture(device, width, height, [width, height, color](u32 x, u32 y, u8 rgba[4]) {
        (void)width; (void)height; (void)x; (void)y;
        rgba[0] = (u8)(color.r * 255.0f);
        rgba[1] = (u8)(color.g * 255.0f);
        rgba[2] = (u8)(color.b * 255.0f);
        rgba[3] = (u8)(color.a * 255.0f);
    });
}

}
