#pragma once

#include "core/platform.h"
#include "core/math.h"
#include "rhi/rhi.h"
#include <memory>
#include <unordered_map>
#include <vector>

namespace aether::engine {

struct Sprite {
    std::shared_ptr<rhi::RHITexture> texture;
    Vec2 position{0.0f, 0.0f};
    Vec2 size{1.0f, 1.0f};
    Color color{1.0f, 1.0f, 1.0f, 1.0f};
    f32 rotation = 0.0f;
    f32 layer = 0.0f;
    rhi::BlendMode blend = rhi::BlendMode::Alpha;
    Vec2 uv0{0.0f, 0.0f};
    Vec2 uv1{1.0f, 1.0f};
    bool is_custom_quad = false;
    Vec2 quad_pts[4];
};

class SpriteBatch {
public:
    static constexpr u32 kMaxSprites = 16384;
    static constexpr u32 kVerticesPerSprite = 4;
    static constexpr u32 kFloatsPerVertex = 8;

    bool init(rhi::RHIDevice* device, rhi::Format color_format);
    void clear();
    void add(std::shared_ptr<rhi::RHITexture> texture, const Vec2& position, const Vec2& size,
             const Color& color, f32 rotation = 0.0f, f32 layer = 0.0f,
             rhi::BlendMode blend = rhi::BlendMode::Alpha);
    void add_uv(std::shared_ptr<rhi::RHITexture> texture, const Vec2& uv0, const Vec2& uv1,
                const Vec2& position, const Vec2& size, const Color& color, f32 rotation = 0.0f,
                f32 layer = 0.0f, rhi::BlendMode blend = rhi::BlendMode::Alpha);
    void add_quad(std::shared_ptr<rhi::RHITexture> texture,
                  const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3,
                  const Color& color, f32 layer = 0.0f,
                  const Vec2& uv0 = {0.0f, 0.0f}, const Vec2& uv1 = {1.0f, 1.0f},
                  rhi::BlendMode blend = rhi::BlendMode::Alpha);

    void render(rhi::RHICommandEncoder* encoder, const Mat4& vp);
    u32 sprite_count() const { return static_cast<u32>(sprites_.size()); }

private:
    void build_quad(const Sprite& sprite, f32* verts);
    rhi::RHIDevice* device_ = nullptr;
    std::shared_ptr<rhi::RHIBuffer> vertex_buffer_;
    std::shared_ptr<rhi::RHIBuffer> index_buffer_;
    std::shared_ptr<rhi::RHIBuffer> uniform_buffer_;
    std::shared_ptr<rhi::RHISampler> sampler_;
    std::shared_ptr<rhi::RHIRenderPipeline> alpha_pipeline_;
    std::shared_ptr<rhi::RHIRenderPipeline> additive_pipeline_;
    rhi::BindGroupLayoutDesc bind_group_layout_desc_;
    std::unordered_map<const rhi::RHITexture*, std::shared_ptr<rhi::RHIBindGroup>> bind_group_cache_;
    std::vector<Sprite> sprites_;
    // render() 复用的临时空间, 避免每帧分配/整块拷贝
    std::vector<u32> sort_scratch_;
    std::vector<f32> vertex_scratch_;
};

}
