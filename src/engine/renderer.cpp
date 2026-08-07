#include "engine/renderer.h"
#include "engine/shaders.h"
#include "platform/platform.h"
#include <cstdio>
#include <cstring>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include "webgpu/webgpu_internal.h"
#endif

namespace aether::engine {

Renderer::Renderer() = default;
Renderer::~Renderer() = default;

bool Renderer::init(const RendererConfig& config) {
    config_ = config;
    width_ = config.width;
    height_ = config.height;
    init_state_ = InitState::DeviceInit;
    step_init();
    return init_state_ != InitState::Failed;
}

void Renderer::step_init() {
    switch (init_state_) {
        case InitState::NotStarted:
            break;
        case InitState::DeviceInit: {
            switch (config_.backend) {
#ifdef __EMSCRIPTEN__
                case rhi::BackendType::WebGPU:
                    device_ = rhi::create_webgpu_device();
                    break;
#else
                case rhi::BackendType::D3D11:
                    device_ = rhi::create_d3d11_device();
                    break;
#endif
                default:
                    printf("Unsupported backend type\n");
                    init_state_ = InitState::Failed;
                    return;
            }
            if (!device_) {
                printf("Failed to create RHI device\n");
                init_state_ = InitState::Failed;
                return;
            }
            if (!device_->init(aether::platform::native_window_handle())) {
                init_state_ = InitState::Failed;
                return;
            }
            init_state_ = InitState::DeviceReady;
            [[fallthrough]];
        }
        case InitState::DeviceReady: {
            device_->tick();
            if (device_->is_failed()) {
                printf("Renderer device initialization failed\n");
                init_state_ = InitState::Failed;
                return;
            }
            if (!device_->is_ready()) {
                break;
            }
#ifdef __EMSCRIPTEN__
            double canvas_width, canvas_height;
            emscripten_get_element_css_size("#canvas", &canvas_width, &canvas_height);
            if (canvas_width > 0 && canvas_height > 0) {
                width_ = static_cast<u32>(canvas_width);
                height_ = static_cast<u32>(canvas_height);
            }
#endif
            swapchain_ = device_->create_swapchain(width_, height_);
            if (!swapchain_) {
                printf("Failed to create swapchain\n");
                init_state_ = InitState::Failed;
                return;
            }
            init_state_ = InitState::SwapchainCreated;
            [[fallthrough]];
        }
        case InitState::SwapchainCreated: {
            for (auto& entity : scene_->entities()) {
                if (entity.mesh && !entity.mesh->is_uploaded()) {
                    entity.mesh->upload(device_.get());
                }
            }
            create_pipeline();
            if (!pipeline_) {
                init_state_ = InitState::Failed;
                return;
            }
            init_state_ = InitState::PipelineCreated;
            [[fallthrough]];
        }
        case InitState::PipelineCreated: {
            printf("Renderer initialized: %ux%u\n", width_, height_);
            ready_ = true;
            init_state_ = InitState::Ready;
            break;
        }
        case InitState::Ready:
        case InitState::Failed:
            break;
    }
}

void Renderer::tick() {
    if (device_) {
        device_->tick();
    }
    if (init_state_ != InitState::Ready && init_state_ != InitState::Failed) {
        step_init();
    }
}

void Renderer::shutdown() {
    if (device_) {
        device_->wait_idle();
    }
    ready_ = false;
    pipeline_.reset();
    bind_group_.reset();
    uniform_buffer_.reset();
    vertex_shader_.reset();
    fragment_shader_.reset();
    swapchain_.reset();
    encoder_.reset();
    device_.reset();
}

void Renderer::create_pipeline() {
    if (!scene_ || scene_->entities().empty()) {
        printf("Renderer: no scene entities to render\n");
        return;
    }

    auto* mesh = scene_->entities().front().mesh.get();
    if (!mesh || !mesh->is_uploaded()) {
        printf("Renderer: no uploaded mesh to build pipeline for\n");
        return;
    }

    const auto& sources = shaders::unlit_color();

    rhi::ShaderModuleDesc vs_desc;
    vs_desc.code = sources.vertex;
    vs_desc.code_size = strlen(sources.vertex);
    vs_desc.entry_point = "vs_main";
    vertex_shader_ = device_->create_shader(rhi::ShaderStage::Vertex, vs_desc);
    if (!vertex_shader_) return;

    rhi::ShaderModuleDesc fs_desc;
    fs_desc.code = sources.fragment;
    fs_desc.code_size = strlen(sources.fragment);
    fs_desc.entry_point = "fs_main";
    fragment_shader_ = device_->create_shader(rhi::ShaderStage::Fragment, fs_desc);
    if (!fragment_shader_) return;

    rhi::ColorTargetState color_target;
    color_target.format = swapchain_->color_format();

    rhi::RenderPipelineDesc rp_desc;
    rp_desc.vertex_shader = vertex_shader_.get();
    rp_desc.fragment_shader = fragment_shader_.get();
    rp_desc.primitive_topology = rhi::PrimitiveTopology::TriangleList;
    rp_desc.vertex_layout = mesh->vertex_layout();
    rp_desc.color_target_count = 1;
    rp_desc.color_targets = &color_target;
    rp_desc.front_face = rhi::FrontFace::CCW;
    rp_desc.cull_mode = rhi::CullMode::None;
    rp_desc.depth_stencil = nullptr;

    pipeline_ = device_->create_render_pipeline(rp_desc);
    if (!pipeline_) {
        printf("Failed to create render pipeline\n");
        return;
    }

    create_uniforms();
}

void Renderer::create_uniforms() {
    rhi::BufferDesc ub_desc;
    ub_desc.size = sizeof(f32) * 16;
    ub_desc.usage = (u32)rhi::BufferUsage::Uniform | (u32)rhi::BufferUsage::CopyDst;
    ub_desc.memory_type = rhi::BufferMemoryType::DeviceLocal;
    uniform_buffer_ = device_->create_buffer(ub_desc, nullptr);
    if (!uniform_buffer_) {
        printf("Failed to create uniform buffer\n");
        return;
    }

    rhi::BindGroupLayoutDesc layout_desc;
    layout_desc.entries = {{0, rhi::BindGroupEntryKind::Uniform}};
    std::vector<rhi::BindGroupEntry> entries;
    entries.push_back({0, uniform_buffer_.get(), nullptr, nullptr, 0, 0});
    bind_group_ = device_->create_bind_group(layout_desc, entries);
    if (!bind_group_) {
        printf("Failed to create bind group\n");
    }
}

bool Renderer::begin_frame() {
    if (!ready_ || frame_started_) return false;
    encoder_ = device_->create_command_encoder();
    if (!encoder_) return false;

    auto* color_view = swapchain_->get_current_view();
    if (!color_view) return false;

    rhi::RenderPassColorAttachment color_attach;
    color_attach.view = color_view;
    color_attach.load_op = rhi::LoadOp::Clear;
    color_attach.store_op = rhi::StoreOp::Store;
    color_attach.clear_color[0] = clear_color_[0];
    color_attach.clear_color[1] = clear_color_[1];
    color_attach.clear_color[2] = clear_color_[2];
    color_attach.clear_color[3] = clear_color_[3];

    rhi::RenderPassDesc rp_desc;
    rp_desc.color_attachment_count = 1;
    rp_desc.color_attachments = &color_attach;
    rp_desc.depth_attachment = nullptr;

    encoder_->begin_render_pass(rp_desc);
    frame_started_ = true;
    return true;
}

void Renderer::draw() {
    if (!ready_ || !frame_started_ || !scene_ || !uniform_buffer_ || !bind_group_) return;

    f32 aspect = (height_ > 0) ? (f32)width_ / (f32)height_ : 1.0f;
    Mat4 view_proj = camera_.view_projection(aspect);

    for (auto& entity : scene_->entities()) {
        auto* mesh = entity.mesh.get();
        if (!mesh || !mesh->is_uploaded()) continue;

        Mat4 mvp = mat4_mul(view_proj, entity.transform.model_matrix());
        device_->update_buffer(uniform_buffer_.get(), 0, mvp.m, sizeof(mvp.m));

        encoder_->set_bind_group(0, bind_group_.get());
        encoder_->set_pipeline(pipeline_.get());
        encoder_->set_vertex_buffer(0, mesh->vertex_buffer().get(), 0);
        if (mesh->has_indices()) {
            encoder_->set_index_buffer(mesh->index_buffer().get(), 0);
            encoder_->draw_indexed(mesh->index_count(), 1, 0, 0, 0);
        } else {
            encoder_->draw(mesh->vertex_count(), 1, 0, 0);
        }
    }
}

void Renderer::end_frame() {
    if (!frame_started_) return;

    encoder_->end_render_pass();
    encoder_->finish();
    encoder_->submit();
    swapchain_->present();
    encoder_.reset();
    frame_started_ = false;
}

void Renderer::clear_color(f32 r, f32 g, f32 b, f32 a) {
    clear_color_[0] = r;
    clear_color_[1] = g;
    clear_color_[2] = b;
    clear_color_[3] = a;
}

}
