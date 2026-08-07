#include "webgpu/webgpu_internal.h"

namespace aether::webgpu {

WebGPUShader::~WebGPUShader() {
    if (module) {
        wgpuShaderModuleRelease(module);
    }
}

WebGPUTextureView::~WebGPUTextureView() {
    if (view) {
        wgpuTextureViewRelease(view);
    }
    if (texture) {
        wgpuTextureRelease(texture);
    }
}

WebGPUTexture::~WebGPUTexture() {
    if (texture) {
        wgpuTextureRelease(texture);
    }
}

rhi::RHITextureView* WebGPUTexture::get_view() {
    return texture_view.get();
}

WebGPUSampler::~WebGPUSampler() {
    if (sampler) {
        wgpuSamplerRelease(sampler);
    }
}

WebGPURenderPipeline::~WebGPURenderPipeline() {
    if (pipeline) {
        wgpuRenderPipelineRelease(pipeline);
    }
}

WebGPUBindGroup::~WebGPUBindGroup() {
    if (bind_group) {
        wgpuBindGroupRelease(bind_group);
    }
}

}
