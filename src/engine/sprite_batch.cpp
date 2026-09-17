#include "engine/sprite_batch.h"
#include "engine/shaders.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

namespace aether::engine {

bool SpriteBatch::init(rhi::RHIDevice* device, rhi::Format color_format) {
    device_ = device;

    rhi::BufferDesc vb_desc;
    vb_desc.size = kMaxSprites * kVerticesPerSprite * kFloatsPerVertex * sizeof(f32);
    vb_desc.usage = (u32)rhi::BufferUsage::Vertex | (u32)rhi::BufferUsage::CopyDst;
    // 每帧更新的顶点缓冲用 HostVisible, 走 Map DISCARD (驱动重命名, 无同步停顿)
    vb_desc.memory_type = rhi::BufferMemoryType::HostVisible;
    vertex_buffer_ = device_->create_buffer(vb_desc, nullptr);
    if (!vertex_buffer_) {
        printf("SpriteBatch: failed to create vertex buffer\n");
        return false;
    }

    std::vector<u32> indices(kMaxSprites * 6);
    for (u32 s = 0; s < kMaxSprites; s++) {
        u32 base = s * 4;
        indices[s * 6 + 0] = base + 0;
        indices[s * 6 + 1] = base + 1;
        indices[s * 6 + 2] = base + 2;
        indices[s * 6 + 3] = base + 2;
        indices[s * 6 + 4] = base + 1;
        indices[s * 6 + 5] = base + 3;
    }
    rhi::BufferDesc ib_desc;
    ib_desc.size = indices.size() * sizeof(u32);
    ib_desc.usage = (u32)rhi::BufferUsage::Index | (u32)rhi::BufferUsage::CopyDst;
    ib_desc.memory_type = rhi::BufferMemoryType::DeviceLocal;
    index_buffer_ = device_->create_buffer(ib_desc, indices.data());
    if (!index_buffer_) {
        printf("SpriteBatch: failed to create index buffer\n");
        return false;
    }

    rhi::BufferDesc ub_desc;
    ub_desc.size = sizeof(f32) * 16;
    ub_desc.usage = (u32)rhi::BufferUsage::Uniform | (u32)rhi::BufferUsage::CopyDst;
    ub_desc.memory_type = rhi::BufferMemoryType::HostVisible;
    uniform_buffer_ = device_->create_buffer(ub_desc, nullptr);
    if (!uniform_buffer_) {
        printf("SpriteBatch: failed to create uniform buffer\n");
        return false;
    }

    rhi::BindGroupLayoutDesc layout_desc;
    layout_desc.entries = {{0, rhi::BindGroupEntryKind::Uniform},
                           {1, rhi::BindGroupEntryKind::Texture},
                           {2, rhi::BindGroupEntryKind::Sampler}};
    bind_group_layout_desc_ = layout_desc;

    rhi::SamplerDesc sampler_desc;
    sampler_desc.mag_filter = rhi::TextureFilter::Linear;
    sampler_desc.min_filter = rhi::TextureFilter::Linear;
    sampler_desc.address_mode_u = rhi::TextureAddressMode::ClampToEdge;
    sampler_desc.address_mode_v = rhi::TextureAddressMode::ClampToEdge;
    sampler_ = device_->create_sampler(sampler_desc);
    if (!sampler_) {
        printf("SpriteBatch: failed to create sampler\n");
        return false;
    }

    rhi::ColorTargetState color_target;
    color_target.format = color_format;

    std::vector<rhi::VertexAttribute> attributes;
    attributes.push_back({0, 0, rhi::Format::Float32x2});
    attributes.push_back({1, 8, rhi::Format::Float32x2});
    attributes.push_back({2, 16, rhi::Format::Float32x4});
    rhi::VertexBufferLayout vertex_layout;
    vertex_layout.stride = kFloatsPerVertex * sizeof(f32);
    vertex_layout.attribute_count = (u32)attributes.size();
    vertex_layout.attributes = attributes.data();

    const auto& sources = shaders::unlit_texture();
    rhi::ShaderModuleDesc vs_desc;
    vs_desc.code = sources.vertex;
    vs_desc.code_size = strlen(sources.vertex);
    vs_desc.entry_point = "vs_main";
    auto vs = device_->create_shader(rhi::ShaderStage::Vertex, vs_desc);
    rhi::ShaderModuleDesc fs_desc;
    fs_desc.code = sources.fragment;
    fs_desc.code_size = strlen(sources.fragment);
    fs_desc.entry_point = "fs_main";
    auto fs = device_->create_shader(rhi::ShaderStage::Fragment, fs_desc);
    if (!vs || !fs) {
        printf("SpriteBatch: failed to create shaders\n");
        return false;
    }

    for (auto blend : {rhi::BlendMode::Alpha, rhi::BlendMode::Additive}) {
        rhi::RenderPipelineDesc rp_desc;
        rp_desc.vertex_shader = vs.get();
        rp_desc.fragment_shader = fs.get();
        rp_desc.primitive_topology = rhi::PrimitiveTopology::TriangleList;
        rp_desc.vertex_layout = vertex_layout;
        rp_desc.color_target_count = 1;
        rp_desc.color_targets = &color_target;
        rp_desc.front_face = rhi::FrontFace::CCW;
        rp_desc.cull_mode = rhi::CullMode::None;
        rp_desc.blend_mode = blend;
        rp_desc.bind_group_layouts.push_back(layout_desc);

        auto pipeline = device_->create_render_pipeline(rp_desc);
        if (!pipeline) {
            printf("SpriteBatch: failed to create pipeline\n");
            return false;
        }
        if (blend == rhi::BlendMode::Alpha) {
            alpha_pipeline_ = pipeline;
        } else {
            additive_pipeline_ = pipeline;
        }
    }

    return true;
}

void SpriteBatch::clear() {
    sprites_.clear();
}

void SpriteBatch::add(std::shared_ptr<rhi::RHITexture> texture, const Vec2& position, const Vec2& size,
                      const Color& color, f32 rotation, f32 layer, rhi::BlendMode blend) {
    if (!texture) return;
    Sprite sprite;
    sprite.texture = std::move(texture);
    sprite.position = position;
    sprite.size = size;
    sprite.color = color;
    sprite.rotation = rotation;
    sprite.layer = layer;
    sprite.blend = blend;
    if (sprites_.size() < kMaxSprites) {
        sprites_.push_back(sprite);
    }
}

void SpriteBatch::add_uv(std::shared_ptr<rhi::RHITexture> texture, const Vec2& uv0, const Vec2& uv1,
                         const Vec2& position, const Vec2& size, const Color& color, f32 rotation,
                         f32 layer, rhi::BlendMode blend) {
    if (!texture) return;
    Sprite sprite;
    sprite.texture = std::move(texture);
    sprite.uv0 = uv0;
    sprite.uv1 = uv1;
    sprite.position = position;
    sprite.size = size;
    sprite.color = color;
    sprite.rotation = rotation;
    sprite.layer = layer;
    sprite.blend = blend;
    if (sprites_.size() < kMaxSprites) {
        sprites_.push_back(sprite);
    }
}

void SpriteBatch::add_quad(std::shared_ptr<rhi::RHITexture> texture,
                          const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3,
                          const Color& color, f32 layer,
                          const Vec2& uv0, const Vec2& uv1,
                          rhi::BlendMode blend) {
    if (!texture || sprites_.size() >= kMaxSprites) return;
    Sprite sprite;
    sprite.texture = std::move(texture);
    sprite.color = color;
    sprite.layer = layer;
    sprite.blend = blend;
    sprite.uv0 = uv0;
    sprite.uv1 = uv1;
    sprite.is_custom_quad = true;
    sprite.quad_pts[0] = p0;
    sprite.quad_pts[1] = p1;
    sprite.quad_pts[2] = p2;
    sprite.quad_pts[3] = p3;
    sprites_.push_back(sprite);
}

void SpriteBatch::build_quad(const Sprite& sprite, f32* verts) {
    const f32 uvs[4][2] = {{sprite.uv0.x, sprite.uv0.y},
                           {sprite.uv1.x, sprite.uv0.y},
                           {sprite.uv0.x, sprite.uv1.y},
                           {sprite.uv1.x, sprite.uv1.y}};

    if (sprite.is_custom_quad) {
        for (u32 i = 0; i < 4; i++) {
            verts[i * 8 + 0] = sprite.quad_pts[i].x;
            verts[i * 8 + 1] = sprite.quad_pts[i].y;
            verts[i * 8 + 2] = uvs[i][0];
            verts[i * 8 + 3] = uvs[i][1];
            verts[i * 8 + 4] = sprite.color.r;
            verts[i * 8 + 5] = sprite.color.g;
            verts[i * 8 + 6] = sprite.color.b;
            verts[i * 8 + 7] = sprite.color.a;
        }
        return;
    }

    f32 hx = sprite.size.x * 0.5f;
    f32 hy = sprite.size.y * 0.5f;
    f32 c = cosf(sprite.rotation);
    f32 s = sinf(sprite.rotation);

    const f32 local[4][2] = {{-hx, -hy}, {hx, -hy}, {-hx, hy}, {hx, hy}};

    for (u32 i = 0; i < 4; i++) {
        f32 rx = local[i][0] * c - local[i][1] * s;
        f32 ry = local[i][0] * s + local[i][1] * c;
        verts[i * 8 + 0] = sprite.position.x + rx;
        verts[i * 8 + 1] = sprite.position.y + ry;
        verts[i * 8 + 2] = uvs[i][0];
        verts[i * 8 + 3] = uvs[i][1];
        verts[i * 8 + 4] = sprite.color.r;
        verts[i * 8 + 5] = sprite.color.g;
        verts[i * 8 + 6] = sprite.color.b;
        verts[i * 8 + 7] = sprite.color.a;
    }
}

void SpriteBatch::render(rhi::RHICommandEncoder* encoder, const Mat4& vp) {
    if (!device_ || sprites_.empty()) return;

    device_->update_buffer(uniform_buffer_.get(), 0, vp.m, sizeof(vp.m));

    const u32 total = std::min(static_cast<u32>(sprites_.size()), kMaxSprites);

    // 按层排序索引 (不整块拷贝 Sprite, 避免每帧 shared_ptr 引用计数抖动)
    sort_scratch_.resize(total);
    for (u32 i = 0; i < total; i++) sort_scratch_[i] = i;
    std::stable_sort(sort_scratch_.begin(), sort_scratch_.end(), [&](u32 a, u32 b) {
        return sprites_[a].layer < sprites_[b].layer;
    });

    const u32 floats_per_sprite = kVerticesPerSprite * kFloatsPerVertex;
    vertex_scratch_.resize((size_t)total * floats_per_sprite);
    for (u32 i = 0; i < total; i++) {
        build_quad(sprites_[sort_scratch_[i]], &vertex_scratch_[(size_t)i * floats_per_sprite]);
    }
    device_->update_buffer(vertex_buffer_.get(), 0, vertex_scratch_.data(),
                           (u64)total * floats_per_sprite * sizeof(f32));

    u32 start = 0;
    while (start < total) {
        const Sprite& first = sprites_[sort_scratch_[start]];
        u32 end = start + 1;
        while (end < total) {
            const Sprite& next = sprites_[sort_scratch_[end]];
            if (next.texture.get() != first.texture.get() || next.blend != first.blend) break;
            end++;
        }

        u32 count = end - start;

        const rhi::RHITexture* tex_key = first.texture.get();
        auto it = bind_group_cache_.find(tex_key);
        if (it == bind_group_cache_.end()) {
            std::vector<rhi::BindGroupEntry> bg_entries;
            bg_entries.push_back({0, uniform_buffer_.get(), nullptr, nullptr, 0, 0});
            bg_entries.push_back({1, nullptr, first.texture->get_view(), nullptr, 0, 0});
            bg_entries.push_back({2, nullptr, nullptr, sampler_.get(), 0, 0});
            auto bg = device_->create_bind_group(bind_group_layout_desc_, bg_entries);
            if (!bg) {
                start = end;
                continue;
            }
            it = bind_group_cache_.emplace(tex_key, std::move(bg)).first;
        }
        encoder->set_bind_group(0, it->second.get());
        encoder->set_pipeline(first.blend == rhi::BlendMode::Additive ? additive_pipeline_.get() : alpha_pipeline_.get());
        encoder->set_vertex_buffer(0, vertex_buffer_.get(), 0);
        encoder->set_index_buffer(index_buffer_.get(), 0);
        encoder->draw_indexed(count * 6, 1, start * 6, 0, 0);

        start = end;
    }
}

}
