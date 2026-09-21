#include "WebGPUInternal.h"
#include <cstdio>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

namespace Aether::WebGPU {

WebGPUSwapchain::~WebGPUSwapchain()
{
    if (mCurrentView)
    {
        wgpuTextureViewRelease(mCurrentView);
        mCurrentView = nullptr;
    }
    if (mCurrentTexture)
    {
        wgpuTextureRelease(mCurrentTexture);
        mCurrentTexture = nullptr;
    }
    if (mDepthView)
    {
        wgpuTextureViewRelease(mDepthView);
        mDepthView = nullptr;
    }
    if (mDepthTexture)
    {
        wgpuTextureRelease(mDepthTexture);
        mDepthTexture = nullptr;
    }
}

void WebGPUSwapchain::Configure(WGPUDevice device)
{
    if (!mSurface || !device) return;

    WGPUSurfaceConfiguration config = {};
    config.device = device;
    config.format = WGPUTextureFormat_BGRA8Unorm;
    config.usage = WGPUTextureUsage_RenderAttachment;
    config.width = mWidth;
    config.height = mHeight;
    config.presentMode = WGPUPresentMode_Fifo;
    config.alphaMode = WGPUCompositeAlphaMode_Auto;
    config.viewFormatCount = 0;
    config.viewFormats = nullptr;
    config.nextInChain = nullptr;

    wgpuSurfaceConfigure(mSurface, &config);
    mConfigured = true;
    mColorFormat = RHI::Format::BGRA8Unorm;
#ifdef __EMSCRIPTEN__
    {
        double cssWidth = 0.0, cssHeight = 0.0;
        emscripten_get_element_css_size("#canvas", &cssWidth, &cssHeight);
        int backingWidth = 0, backingHeight = 0;
        emscripten_get_canvas_element_size("#canvas", &backingWidth, &backingHeight);
        printf("WebGPU swapchain configured %ux%u (canvas css %.0fx%.0f, backing %dx%d)\n",
               mWidth, mHeight, cssWidth, cssHeight, backingWidth, backingHeight);
    }
#endif

    // 与表面同尺寸的深度附件
    if (mDepthView)
    {
        wgpuTextureViewRelease(mDepthView);
        mDepthView = nullptr;
    }
    if (mDepthTexture)
    {
        wgpuTextureRelease(mDepthTexture);
        mDepthTexture = nullptr;
    }
    mDepthDevice = device;
    WGPUTextureDescriptor depthDesc = {};
    depthDesc.size = {(u32)mWidth, (u32)mHeight, 1};
    depthDesc.format = WGPUTextureFormat_Depth32Float;
    depthDesc.usage = WGPUTextureUsage_RenderAttachment;
    depthDesc.mipLevelCount = 1;
    depthDesc.sampleCount = 1;
    depthDesc.dimension = WGPUTextureDimension_2D;
    mDepthTexture = wgpuDeviceCreateTexture(device, &depthDesc);
    if (mDepthTexture)
    {
        WGPUTextureViewDescriptor viewDesc = {};
        viewDesc.format = WGPUTextureFormat_Depth32Float;
        viewDesc.dimension = WGPUTextureViewDimension_2D;
        viewDesc.baseMipLevel = 0;
        viewDesc.mipLevelCount = 1;
        viewDesc.baseArrayLayer = 0;
        viewDesc.arrayLayerCount = 1;
        viewDesc.aspect = WGPUTextureAspect_DepthOnly;
        mDepthView = wgpuTextureCreateView(mDepthTexture, &viewDesc);
        mDepthFormat = RHI::Format::Depth32Float;
        printf("WebGPU depth texture created %ux%u (view %s)\n", mWidth, mHeight, mDepthView ? "ok" : "FAILED");
    }
    else
    {
        printf("WebGPU depth texture creation FAILED\n");
    }
}

RHI::RHITextureView* WebGPUSwapchain::GetCurrentView()
{
    if (!mSurface || !mConfigured) return nullptr;

    if (mCurrentView)
    {
        wgpuTextureViewRelease(mCurrentView);
        mCurrentView = nullptr;
    }
    if (mCurrentTexture)
    {
        wgpuTextureRelease(mCurrentTexture);
        mCurrentTexture = nullptr;
    }

    WGPUSurfaceTexture surfaceTexture = {};
    wgpuSurfaceGetCurrentTexture(mSurface, &surfaceTexture);

    if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal || !surfaceTexture.texture)
    {
        printf("Failed to get current surface texture, status: %d\n", (int)surfaceTexture.status);
        return nullptr;
    }

    mCurrentTexture = surfaceTexture.texture;

    WGPUTextureViewDescriptor viewDesc = {};
    viewDesc.format = WGPUTextureFormat_BGRA8Unorm;
    viewDesc.dimension = WGPUTextureViewDimension_2D;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;
    viewDesc.aspect = WGPUTextureAspect_All;

    mCurrentView = wgpuTextureCreateView(mCurrentTexture, &viewDesc);
    if (!mCurrentView)
    {
        printf("Failed to create texture view\n");
        return nullptr;
    }

    static bool sLoggedOnce = false;
    if (!sLoggedOnce)
    {
        sLoggedOnce = true;
        printf("WebGPU surface texture acquired OK (%ux%u, status %d)\n", mWidth, mHeight, (int)surfaceTexture.status);
    }

    // The swapchain does not own this view for RefPtr purposes; hand out a
    // stable wrapper that mirrors the raw handle.
    static WebGPUTextureView sWrapper;
    sWrapper.mView = mCurrentView;
    sWrapper.mTexture = nullptr;
    return &sWrapper;
}

bool WebGPUSwapchain::Present()
{
    if (!mSurface || !mConfigured) return false;
    return true;
}

RHI::RHITextureView* WebGPUSwapchain::GetDepthView()
{
    if (!mDepthView) return nullptr;
    static WebGPUTextureView sWrapper;
    sWrapper.mView = mDepthView;
    sWrapper.mTexture = nullptr;
    return &sWrapper;
}

}
