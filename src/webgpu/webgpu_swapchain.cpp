#include "webgpu/webgpu_internal.h"
#include <cstdio>

namespace aether::webgpu {

WebGPUSwapchain::~WebGPUSwapchain() {
    if (current_view) {
        wgpuTextureViewRelease(current_view);
        current_view = nullptr;
    }
    if (current_texture) {
        wgpuTextureRelease(current_texture);
        current_texture = nullptr;
    }
    if (depth_view) {
        wgpuTextureViewRelease(depth_view);
        depth_view = nullptr;
    }
    if (depth_texture) {
        wgpuTextureRelease(depth_texture);
        depth_texture = nullptr;
    }
}

void WebGPUSwapchain::configure(WGPUDevice device) {
    if (!surface || !device) return;

    WGPUSurfaceConfiguration config = {};
    config.device = device;
    config.format = WGPUTextureFormat_BGRA8Unorm;
    config.usage = WGPUTextureUsage_RenderAttachment;
    config.width = w;
    config.height = h;
    config.presentMode = WGPUPresentMode_Fifo;
    config.alphaMode = WGPUCompositeAlphaMode_Auto;
    config.viewFormatCount = 0;
    config.viewFormats = nullptr;
    config.nextInChain = nullptr;

    wgpuSurfaceConfigure(surface, &config);
    configured = true;
    color_fmt = rhi::Format::BGRA8Unorm;

    // 与表面同尺寸的深度附件
    if (depth_view) {
        wgpuTextureViewRelease(depth_view);
        depth_view = nullptr;
    }
    if (depth_texture) {
        wgpuTextureRelease(depth_texture);
        depth_texture = nullptr;
    }
    depth_device = device;
    WGPUTextureDescriptor depth_desc = {};
    depth_desc.size = {(u32)w, (u32)h, 1};
    depth_desc.format = WGPUTextureFormat_Depth32Float;
    depth_desc.usage = WGPUTextureUsage_RenderAttachment;
    depth_desc.mipLevelCount = 1;
    depth_desc.sampleCount = 1;
    depth_desc.dimension = WGPUTextureDimension_2D;
    depth_texture = wgpuDeviceCreateTexture(device, &depth_desc);
    if (depth_texture) {
        WGPUTextureViewDescriptor view_desc = {};
        view_desc.format = WGPUTextureFormat_Depth32Float;
        view_desc.dimension = WGPUTextureViewDimension_2D;
        view_desc.baseMipLevel = 0;
        view_desc.mipLevelCount = 1;
        view_desc.baseArrayLayer = 0;
        view_desc.arrayLayerCount = 1;
        view_desc.aspect = WGPUTextureAspect_DepthOnly;
        depth_view = wgpuTextureCreateView(depth_texture, &view_desc);
        depth_fmt = rhi::Format::Depth32Float;
    }
}

rhi::RHITextureView* WebGPUSwapchain::get_current_view() {
    if (!surface || !configured) return nullptr;

    if (current_view) {
        wgpuTextureViewRelease(current_view);
        current_view = nullptr;
    }
    if (current_texture) {
        wgpuTextureRelease(current_texture);
        current_texture = nullptr;
    }

    WGPUSurfaceTexture surf_tex = {};
    wgpuSurfaceGetCurrentTexture(surface, &surf_tex);

    if (surf_tex.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal || !surf_tex.texture) {
        printf("Failed to get current surface texture, status: %d\n", (int)surf_tex.status);
        return nullptr;
    }

    current_texture = surf_tex.texture;

    WGPUTextureViewDescriptor view_desc = {};
    view_desc.format = WGPUTextureFormat_BGRA8Unorm;
    view_desc.dimension = WGPUTextureViewDimension_2D;
    view_desc.baseMipLevel = 0;
    view_desc.mipLevelCount = 1;
    view_desc.baseArrayLayer = 0;
    view_desc.arrayLayerCount = 1;
    view_desc.aspect = WGPUTextureAspect_All;

    current_view = wgpuTextureCreateView(current_texture, &view_desc);
    if (!current_view) {
        printf("Failed to create texture view\n");
        return nullptr;
    }

    static WebGPUTextureView wrapper;
    wrapper.view = current_view;
    wrapper.texture = nullptr;
    return &wrapper;
}

bool WebGPUSwapchain::present() {
    if (!surface || !configured) return false;
    return true;
}

rhi::RHITextureView* WebGPUSwapchain::get_depth_view() {
    if (!depth_view) return nullptr;
    static WebGPUTextureView wrapper;
    wrapper.view = depth_view;
    wrapper.texture = nullptr;
    return &wrapper;
}

}
