#include "engine/renderer2d.h"
#include "platform/platform.h"
#include <cstdio>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include "webgpu/webgpu_internal.h"
#endif

namespace aether::engine {

bool Renderer2D::init(u32 width, u32 height) {
    width_ = width;
    height_ = height;
    init_state_ = InitState::DeviceInit;
    step_init();
    return !is_failed();
}

void Renderer2D::step_init() {
    switch (init_state_) {
        case InitState::NotStarted:
            break;
        case InitState::DeviceInit: {
#ifdef __EMSCRIPTEN__
            device_ = rhi::create_webgpu_device();
#else
            device_ = rhi::create_d3d11_device();
#endif
            if (!device_) {
                printf("Renderer2D: failed to create RHI device\n");
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
                init_state_ = InitState::Failed;
                return;
            }
            if (!device_->is_ready()) {
                break;
            }
#ifdef __EMSCRIPTEN__
            double canvas_width = 0, canvas_height = 0;
            emscripten_get_element_css_size("#canvas", &canvas_width, &canvas_height);
            if (canvas_width > 0 && canvas_height > 0) {
                width_ = static_cast<u32>(canvas_width);
                height_ = static_cast<u32>(canvas_height);
            }
#endif
            swapchain_ = device_->create_swapchain(width_, height_);
            if (!swapchain_) {
                printf("Renderer2D: failed to create swapchain\n");
                init_state_ = InitState::Failed;
                return;
            }
            init_state_ = InitState::SwapchainCreated;
            [[fallthrough]];
        }
        case InitState::SwapchainCreated: {
            if (!sprite_batch_.init(device_.get(), swapchain_->color_format())) {
                init_state_ = InitState::Failed;
                return;
            }
            if (!starfield_.init(device_.get(), swapchain_->color_format())) {
                init_state_ = InitState::Failed;
                return;
            }
            init_state_ = InitState::ResourcesCreated;
            [[fallthrough]];
        }
        case InitState::ResourcesCreated: {
            printf("Renderer2D initialized: %ux%u\n", width_, height_);
            ready_ = true;
            init_state_ = InitState::Ready;
            break;
        }
        case InitState::Ready:
        case InitState::Failed:
            break;
    }
}

void Renderer2D::tick() {
    if (device_) {
        device_->tick();
    }
    if (init_state_ != InitState::Ready && init_state_ != InitState::Failed) {
        step_init();
    }
}

void Renderer2D::shutdown() {
    if (device_) {
        device_->wait_idle();
    }
    ready_ = false;
    encoder_.reset();
    swapchain_.reset();
    device_.reset();
}

bool Renderer2D::begin_frame() {
    if (!ready_ || frame_started_) return false;

    auto* color_view = swapchain_->get_current_view();
    if (!color_view) return false;

    encoder_ = device_->create_command_encoder();
    if (!encoder_) return false;

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
    sprite_batch_.clear();
    frame_started_ = true;
    return true;
}

void Renderer2D::end_frame() {
    if (!frame_started_) return;

    encoder_->end_render_pass();
    encoder_->finish();
    encoder_->submit();
    swapchain_->present();
    encoder_.reset();
    frame_started_ = false;
}

void Renderer2D::clear_color(f32 r, f32 g, f32 b, f32 a) {
    clear_color_[0] = r;
    clear_color_[1] = g;
    clear_color_[2] = b;
    clear_color_[3] = a;
}

}
