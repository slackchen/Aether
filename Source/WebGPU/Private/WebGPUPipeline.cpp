#include "WebGPUInternal.h"
#include <cstdio>

namespace Aether::WebGPU {

WebGPUCommandEncoder::WebGPUCommandEncoder(WGPUDevice device, WGPUQueue queue)
    : mDevice(device)
    , mQueue(queue)
{
    WGPUCommandEncoderDescriptor desc = {};
    desc.label = MakeStringView("MainCommandEncoder");
    mEncoder = wgpuDeviceCreateCommandEncoder(mDevice, &desc);
}

WebGPUCommandEncoder::~WebGPUCommandEncoder()
{
    if (mRenderPass) wgpuRenderPassEncoderRelease(mRenderPass);
    if (mCommandBuffer) wgpuCommandBufferRelease(mCommandBuffer);
    if (mEncoder) wgpuCommandEncoderRelease(mEncoder);
}

void WebGPUCommandEncoder::BeginRenderPass(const RHI::RenderPassDesc& desc)
{
    WGPURenderPassDescriptor renderPassDesc = {};
    Array<WGPURenderPassColorAttachment> colorAttachments;
    WGPURenderPassDepthStencilAttachment depthAttachment = {};

    colorAttachments.Resize(desc.ColorAttachmentCount);
    for (u32 i = 0; i < desc.ColorAttachmentCount; i++)
    {
        const RHI::RenderPassColorAttachment& attachment = desc.ColorAttachments[i];
        WebGPUTextureView* view = static_cast<WebGPUTextureView*>(attachment.View);
        colorAttachments[i].view = view->mView;
        colorAttachments[i].resolveTarget = nullptr;
        colorAttachments[i].depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
        colorAttachments[i].loadOp = ConvertLoadOp(attachment.LoadOp);
        colorAttachments[i].storeOp = ConvertStoreOp(attachment.StoreOp);
        colorAttachments[i].clearValue = {attachment.ClearColor[0], attachment.ClearColor[1], attachment.ClearColor[2], attachment.ClearColor[3]};
    }

    renderPassDesc.colorAttachmentCount = desc.ColorAttachmentCount;
    renderPassDesc.colorAttachments = colorAttachments.Data();

    if (desc.DepthAttachment)
    {
        const RHI::RenderPassDepthAttachment& depthInfo = *desc.DepthAttachment;
        WebGPUTextureView* view = static_cast<WebGPUTextureView*>(depthInfo.View);
        depthAttachment.view = view->mView;
        depthAttachment.depthLoadOp = ConvertLoadOp(depthInfo.LoadOp);
        depthAttachment.depthStoreOp = ConvertStoreOp(depthInfo.StoreOp);
        depthAttachment.depthClearValue = depthInfo.ClearDepth;
        depthAttachment.depthReadOnly = false;
        depthAttachment.stencilLoadOp = WGPULoadOp_Undefined;
        depthAttachment.stencilStoreOp = WGPUStoreOp_Undefined;
        depthAttachment.stencilClearValue = 0;
        depthAttachment.stencilReadOnly = true;
        renderPassDesc.depthStencilAttachment = &depthAttachment;
    }

    mRenderPass = wgpuCommandEncoderBeginRenderPass(mEncoder, &renderPassDesc);
}

void WebGPUCommandEncoder::EndRenderPass()
{
    if (mRenderPass)
    {
        wgpuRenderPassEncoderEnd(mRenderPass);
        wgpuRenderPassEncoderRelease(mRenderPass);
        mRenderPass = nullptr;
    }
}

void WebGPUCommandEncoder::SetPipeline(RHI::RHIRenderPipeline* pipeline)
{
    if (!mRenderPass || !pipeline) return;
    WebGPURenderPipeline* renderPipeline = static_cast<WebGPURenderPipeline*>(pipeline);
    wgpuRenderPassEncoderSetPipeline(mRenderPass, renderPipeline->mPipeline);
}

void WebGPUCommandEncoder::SetBindGroup(u32 groupIndex, RHI::RHIBindGroup* bindGroup)
{
    if (!mRenderPass || !bindGroup) return;
    WebGPUBindGroup* gpuBindGroup = static_cast<WebGPUBindGroup*>(bindGroup);
    wgpuRenderPassEncoderSetBindGroup(mRenderPass, groupIndex, gpuBindGroup->mBindGroup, 0, nullptr);
}

void WebGPUCommandEncoder::SetVertexBuffer(u32 slot, RHI::RHIBuffer* buffer, u64 offset)
{
    if (!mRenderPass || !buffer) return;
    WebGPUBuffer* gpuBuffer = static_cast<WebGPUBuffer*>(buffer);
    wgpuRenderPassEncoderSetVertexBuffer(mRenderPass, slot, gpuBuffer->mBuffer, offset, gpuBuffer->mBufferSize - offset);
}

void WebGPUCommandEncoder::SetIndexBuffer(RHI::RHIBuffer* buffer, u64 offset, RHI::IndexFormat format)
{
    if (!mRenderPass || !buffer) return;
    WebGPUBuffer* gpuBuffer = static_cast<WebGPUBuffer*>(buffer);
    WGPUIndexFormat wgpuFormat = format == RHI::IndexFormat::Uint16 ? WGPUIndexFormat_Uint16 : WGPUIndexFormat_Uint32;
    wgpuRenderPassEncoderSetIndexBuffer(mRenderPass, gpuBuffer->mBuffer, wgpuFormat, offset, gpuBuffer->mBufferSize - offset);
}

void WebGPUCommandEncoder::Draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance)
{
    if (!mRenderPass) return;
    wgpuRenderPassEncoderDraw(mRenderPass, vertexCount, instanceCount, firstVertex, firstInstance);
}

void WebGPUCommandEncoder::DrawIndexed(u32 indexCount, u32 instanceCount, u32 firstIndex, i32 vertexOffset, u32 firstInstance)
{
    if (!mRenderPass) return;
    wgpuRenderPassEncoderDrawIndexed(mRenderPass, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void WebGPUCommandEncoder::Finish()
{
    if (mRenderPass)
    {
        EndRenderPass();
    }
    if (mEncoder && !mCommandBuffer)
    {
        WGPUCommandBufferDescriptor desc = {};
        desc.label = MakeStringView("MainCommandBuffer");
        mCommandBuffer = wgpuCommandEncoderFinish(mEncoder, &desc);
    }
}

void WebGPUCommandEncoder::Submit()
{
    if (!mCommandBuffer) Finish();
    if (mCommandBuffer)
    {
        wgpuQueueSubmit(mQueue, 1, &mCommandBuffer);
        static bool sLoggedOnce = false;
        if (!sLoggedOnce)
        {
            sLoggedOnce = true;
            printf("WebGPU first command buffer submitted\n");
        }
    }
}

}
