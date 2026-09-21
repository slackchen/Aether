#pragma once

#include "Container/RefPtr.h"
#include "Core.h"
#include "RHI.h"

namespace Aether::Engine {

class Starfield
{
public:
    struct Params
    {
        f32 Time = 0.0f;
        f32 Speed = 1.0f;
        f32 Resolution[2] = {1.0f, 1.0f};
    };

    bool Init(RHI::RHIDevice* device, RHI::Format colorFormat);
    void Render(RHI::RHICommandEncoder* encoder, f32 time, f32 speed, f32 width, f32 height);

private:
    RHI::RHIDevice* mDevice = nullptr;
    RefPtr<RHI::RHIBuffer> mVertexBuffer;
    RefPtr<RHI::RHIBuffer> mUniformBuffer;
    RefPtr<RHI::RHIBindGroup> mBindGroup;
    RefPtr<RHI::RHIRenderPipeline> mPipeline;
};

}
