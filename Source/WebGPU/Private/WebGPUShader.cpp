#include "WebGPUInternal.h"

namespace Aether::WebGPU {

WebGPUShader::~WebGPUShader()
{
    if (mModule)
    {
        wgpuShaderModuleRelease(mModule);
    }
}

WebGPUTextureView::~WebGPUTextureView()
{
    if (mView)
    {
        wgpuTextureViewRelease(mView);
    }
    if (mTexture)
    {
        wgpuTextureRelease(mTexture);
    }
}

WebGPUTexture::~WebGPUTexture()
{
    if (mTexture)
    {
        wgpuTextureRelease(mTexture);
    }
}

RHI::RHITextureView* WebGPUTexture::GetView()
{
    return mTextureView.Get();
}

WebGPUSampler::~WebGPUSampler()
{
    if (mSampler)
    {
        wgpuSamplerRelease(mSampler);
    }
}

WebGPURenderPipeline::~WebGPURenderPipeline()
{
    if (mPipeline)
    {
        wgpuRenderPipelineRelease(mPipeline);
    }
}

WebGPUBindGroup::~WebGPUBindGroup()
{
    if (mBindGroup)
    {
        wgpuBindGroupRelease(mBindGroup);
    }
}

}
