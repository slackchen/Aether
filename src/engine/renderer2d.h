#pragma once

#include "core/platform.h"
#include "engine/camera.h"
#include "engine/sprite_batch.h"
#include "engine/starfield.h"
#include "rhi/rhi.h"
#include <memory>

namespace aether::engine {

class Renderer2D {
public:
    enum class InitState {
        NotStarted,
        DeviceInit,
        DeviceReady,
        SwapchainCreated,
        ResourcesCreated,
        Ready,
        Failed,
    };

    Renderer2D() = default;
    ~Renderer2D() = default;

    bool init(u32 width, u32 height);
    void tick();
    void shutdown();

    bool is_ready() const { return ready_; }
    bool is_failed() const { return init_state_ == InitState::Failed; }
    u32 width() const { return width_; }
    u32 height() const { return height_; }
    f32 aspect() const { return height_ > 0 ? (f32)width_ / (f32)height_ : 1.0f; }

    bool begin_frame();
    void end_frame();

    Camera2D& camera() { return camera_; }
    SpriteBatch& sprites() { return sprite_batch_; }
    Starfield& starfield() { return starfield_; }
    rhi::RHIDevice* device() { return device_.get(); }
    rhi::RHICommandEncoder* encoder() { return encoder_.get(); }

    void clear_color(f32 r, f32 g, f32 b, f32 a);

private:
    void step_init();

    u32 width_ = 0;
    u32 height_ = 0;
    f32 clear_color_[4] = {0.0f, 0.0f, 0.0f, 1.0f};

    std::unique_ptr<rhi::RHIDevice> device_;
    std::shared_ptr<rhi::RHISwapchain> swapchain_;
    std::unique_ptr<rhi::RHICommandEncoder> encoder_;
    bool frame_started_ = false;
    bool ready_ = false;

    Camera2D camera_;
    SpriteBatch sprite_batch_;
    Starfield starfield_;
    InitState init_state_ = InitState::NotStarted;
};

}
