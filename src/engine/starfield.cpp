#include "engine/starfield.h"
#include "engine/shaders.h"
#include <cstdio>
#include <cstring>
#include <vector>

namespace aether::engine {

bool Starfield::init(rhi::RHIDevice* device, rhi::Format color_format) {
    device_ = device;

    const f32 positions[3][2] = {{-1.0f, -1.0f}, {3.0f, -1.0f}, {-1.0f, 3.0f}};
    rhi::BufferDesc vb_desc;
    vb_desc.size = sizeof(positions);
    vb_desc.usage = (u32)rhi::BufferUsage::Vertex | (u32)rhi::BufferUsage::CopyDst;
    vb_desc.memory_type = rhi::BufferMemoryType::DeviceLocal;
    vertex_buffer_ = device_->create_buffer(vb_desc, positions);
    if (!vertex_buffer_) {
        printf("Starfield: failed to create vertex buffer\n");
        return false;
    }

    rhi::BufferDesc ub_desc;
    ub_desc.size = sizeof(Params);
    ub_desc.usage = (u32)rhi::BufferUsage::Uniform | (u32)rhi::BufferUsage::CopyDst;
    ub_desc.memory_type = rhi::BufferMemoryType::DeviceLocal;
    uniform_buffer_ = device_->create_buffer(ub_desc, nullptr);
    if (!uniform_buffer_) {
        printf("Starfield: failed to create uniform buffer\n");
        return false;
    }

    rhi::BindGroupLayoutDesc layout_desc;
    layout_desc.entries = {{0, rhi::BindGroupEntryKind::Uniform}};
    std::vector<rhi::BindGroupEntry> bg_entries;
    bg_entries.push_back({0, uniform_buffer_.get(), nullptr, nullptr, 0, 0});
    bind_group_ = device_->create_bind_group(layout_desc, bg_entries);
    if (!bind_group_) {
        printf("Starfield: failed to create bind group\n");
        return false;
    }

    std::vector<rhi::VertexAttribute> attributes;
    attributes.push_back({0, 0, rhi::Format::Float32x2});
    rhi::VertexBufferLayout vertex_layout;
    vertex_layout.stride = sizeof(f32) * 2;
    vertex_layout.attribute_count = (u32)attributes.size();
    vertex_layout.attributes = attributes.data();

    const auto& sources = shaders::starfield();
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
        printf("Starfield: failed to create shaders\n");
        return false;
    }

    rhi::ColorTargetState color_target;
    color_target.format = color_format;

    rhi::RenderPipelineDesc rp_desc;
    rp_desc.vertex_shader = vs.get();
    rp_desc.fragment_shader = fs.get();
    rp_desc.primitive_topology = rhi::PrimitiveTopology::TriangleList;
    rp_desc.vertex_layout = vertex_layout;
    rp_desc.color_target_count = 1;
    rp_desc.color_targets = &color_target;
    rp_desc.front_face = rhi::FrontFace::CCW;
    rp_desc.cull_mode = rhi::CullMode::None;
    rp_desc.blend_mode = rhi::BlendMode::Alpha;
    rp_desc.bind_group_layouts.push_back(layout_desc);

    pipeline_ = device_->create_render_pipeline(rp_desc);
    if (!pipeline_) {
        printf("Starfield: failed to create pipeline\n");
        return false;
    }

    return true;
}

void Starfield::render(rhi::RHICommandEncoder* encoder, f32 time, f32 speed, f32 width, f32 height) {
    if (!device_) return;

    Params params;
    params.time = time;
    params.speed = speed;
    params.resolution[0] = width;
    params.resolution[1] = height;
    device_->update_buffer(uniform_buffer_.get(), 0, &params, sizeof(params));

    encoder->set_bind_group(0, bind_group_.get());
    encoder->set_pipeline(pipeline_.get());
    encoder->set_vertex_buffer(0, vertex_buffer_.get(), 0);
    encoder->draw(3, 1, 0, 0);
}

}
