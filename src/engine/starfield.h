#pragma once

#include "core/platform.h"
#include "core/math.h"
#include "rhi/rhi.h"
#include <memory>

namespace aether::engine {

class Starfield {
public:
    struct Params {
        f32 time = 0.0f;
        f32 speed = 1.0f;
        f32 resolution[2] = {1.0f, 1.0f};
    };

    bool init(rhi::RHIDevice* device, rhi::Format color_format);
    void render(rhi::RHICommandEncoder* encoder, f32 time, f32 speed, f32 width, f32 height);

private:
    rhi::RHIDevice* device_ = nullptr;
    std::shared_ptr<rhi::RHIBuffer> vertex_buffer_;
    std::shared_ptr<rhi::RHIBuffer> uniform_buffer_;
    std::shared_ptr<rhi::RHIBindGroup> bind_group_;
    std::shared_ptr<rhi::RHIRenderPipeline> pipeline_;
};

}
