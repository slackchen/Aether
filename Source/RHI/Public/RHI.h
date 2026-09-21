#pragma once

#include "Container/RefPtr.h"
#include "Core.h"
#include "RHITypes.h"

namespace Aether::RHI {

//
// Render hardware interface. All resources are RefCounted and returned as
// RefPtr; device/encoder are exclusively owned via UniquePtr.
//
class RHIBuffer : public RefCounted
{
public:
    virtual void* Map() = 0;
    virtual void Unmap() = 0;
    virtual u64 Size() const = 0;
};

class RHIShader : public RefCounted
{
public:
};

class RHITexture : public RefCounted
{
public:
    virtual u32 Width() const = 0;
    virtual u32 Height() const = 0;
    virtual RHITextureView* GetView() = 0;
};

class RHISampler : public RefCounted
{
public:
};

class RHITextureView : public RefCounted
{
public:
};

class RHISwapchain : public RefCounted
{
public:
    virtual u32 Width() const = 0;
    virtual u32 Height() const = 0;
    virtual RHITextureView* GetCurrentView() = 0;
    virtual RHITextureView* GetDepthView() { return nullptr; }
    virtual Format ColorFormat() const = 0;
    virtual Format DepthFormat() const = 0;
    virtual bool Present() = 0;
};

class RHICommandEncoder
{
public:
    virtual ~RHICommandEncoder() = default;
    virtual void BeginRenderPass(const RenderPassDesc& desc) = 0;
    virtual void EndRenderPass() = 0;
    virtual void SetPipeline(class RHIRenderPipeline* pipeline) = 0;
    virtual void SetBindGroup(u32 groupIndex, class RHIBindGroup* bindGroup) = 0;
    virtual void SetVertexBuffer(u32 slot, RHIBuffer* buffer, u64 offset = 0) = 0;
    virtual void SetIndexBuffer(RHIBuffer* buffer, u64 offset = 0, IndexFormat format = IndexFormat::Uint32) = 0;
    virtual void Draw(u32 vertexCount, u32 instanceCount = 1, u32 firstVertex = 0, u32 firstInstance = 0) = 0;
    virtual void DrawIndexed(u32 indexCount, u32 instanceCount = 1, u32 firstIndex = 0, i32 vertexOffset = 0, u32 firstInstance = 0) = 0;
    virtual void Finish() = 0;
    virtual void Submit() = 0;
};

class RHIBindGroup : public RefCounted
{
public:
};

class RHIRenderPipeline : public RefCounted
{
public:
};

class RHIDevice
{
public:
    virtual ~RHIDevice() = default;
    virtual BackendType BackendType() const = 0;
    virtual bool Init(void* nativeWindowHandle = nullptr) = 0;
    virtual void WaitIdle() = 0;
    virtual void Tick() = 0;

    virtual bool IsReady() const { return true; }
    virtual bool IsFailed() const { return false; }

    virtual RefPtr<RHISwapchain> CreateSwapchain(u32 width, u32 height) = 0;
    virtual RefPtr<RHIBuffer> CreateBuffer(const BufferDesc& desc, const void* initialData = nullptr) = 0;
    virtual RefPtr<RHIShader> CreateShader(ShaderStage stage, const ShaderModuleDesc& desc) = 0;
    virtual RefPtr<RHITexture> CreateTexture(const TextureDesc& desc) = 0;
    virtual RefPtr<RHISampler> CreateSampler(const SamplerDesc& desc) = 0;
    virtual RefPtr<RHIRenderPipeline> CreateRenderPipeline(const RenderPipelineDesc& desc) = 0;
    virtual RefPtr<RHIBindGroup> CreateBindGroup(const BindGroupLayoutDesc& layoutDesc, const Array<BindGroupEntry>& entries) = 0;
    virtual void UpdateBuffer(RHIBuffer* buffer, u64 offset, const void* data, u64 size) = 0;
    virtual void UpdateTexture(RHITexture* texture, const void* data) = 0;
    virtual UniquePtr<RHICommandEncoder> CreateCommandEncoder() = 0;
    virtual void ToggleFullscreen() {}
};

UniquePtr<RHIDevice> CreateWebGPUDevice();
UniquePtr<RHIDevice> CreateD3D11Device();

}
