#pragma once

#include "core/platform.h"
#include "rhi/rhi.h"
#include "engine/camera.h"
#include "engine/scene.h"
#include <memory>

namespace aether::engine {

struct RendererConfig {
    u32 width = 1280;
    u32 height = 720;
    const char* title = "Aether Engine";
    rhi::BackendType backend = rhi::BackendType::WebGPU;
};

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool init(const RendererConfig& config);
    void shutdown();
    void tick();

    bool begin_frame();
    void end_frame();
    void draw();

    void set_scene(Scene* scene) { scene_ = scene; }
    void set_camera(const Camera& camera) { camera_ = camera; }

    void clear_color(f32 r, f32 g, f32 b, f32 a = 1.0f);
    bool is_ready() const { return ready_; }
    rhi::RHIDevice* device() { return device_.get(); }
    u32 width() const { return width_; }
    u32 height() const { return height_; }

private:
    enum class InitState {
        NotStarted,
        DeviceInit,
        DeviceReady,
        SwapchainCreated,
        PipelineCreated,
        Ready,
        Failed,
    };

    void step_init();
    void create_pipeline();
    void create_uniforms();

    std::unique_ptr<rhi::RHIDevice> device_;
    std::shared_ptr<rhi::RHISwapchain> swapchain_;
    std::shared_ptr<rhi::RHIShader> vertex_shader_;
    std::shared_ptr<rhi::RHIShader> fragment_shader_;
    std::shared_ptr<rhi::RHIRenderPipeline> pipeline_;
    std::shared_ptr<rhi::RHIBuffer> uniform_buffer_;
    std::shared_ptr<rhi::RHIBindGroup> bind_group_;
    std::unique_ptr<rhi::RHICommandEncoder> encoder_;

    RendererConfig config_;
    InitState init_state_ = InitState::NotStarted;
    u32 width_ = 0;
    u32 height_ = 0;
    f32 clear_color_[4] = {0.1f, 0.1f, 0.15f, 1.0f};
    bool frame_started_ = false;
    bool ready_ = false;

    Scene* scene_ = nullptr;
    Camera camera_;
};

}
