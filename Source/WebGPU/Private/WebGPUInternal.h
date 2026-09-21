#pragma once

#include "Container/Array.h"
#include "Container/HashMap.h"
#include "Container/RefPtr.h"
#include "Container/String.h"
#include "Core.h"
#include "RHI.h"

#include <webgpu/webgpu.h>
#include <cstring>

namespace Aether::WebGPU {

//
// Helpers for building null-terminated WGPUStringViews from C literals.
//
inline WGPUStringView MakeStringView(const char* text)
{
    WGPUStringView view = {};
    view.data = text;
    view.length = text ? strlen(text) : 0;
    return view;
}

//
// Conversion from the RHI enum layer to raw WebGPU enums.
//
WGPUBufferUsage ConvertBufferUsage(u32 usage);
WGPUTextureUsage ConvertTextureUsage(u32 usage);
WGPUTextureFormat ConvertFormat(RHI::Format format);
WGPUTextureFormat ConvertTextureFormat(RHI::TextureFormat format);
WGPUPrimitiveTopology ConvertPrimitiveTopology(RHI::PrimitiveTopology topology);
WGPUFrontFace ConvertFrontFace(RHI::FrontFace face);
WGPUCullMode ConvertCullMode(RHI::CullMode mode);
WGPUCompareFunction ConvertCompareFunction(RHI::CompareOp op);
WGPULoadOp ConvertLoadOp(RHI::LoadOp op);
WGPUStoreOp ConvertStoreOp(RHI::StoreOp op);
WGPUVertexFormat ConvertVertexFormat(RHI::Format format);
WGPUShaderStage ConvertShaderStage(RHI::ShaderStage stage);
WGPUFilterMode ConvertTextureFilter(RHI::TextureFilter filter);
WGPUAddressMode ConvertAddressMode(RHI::TextureAddressMode mode);

//
// WebGPU buffer resource.
//
class WebGPUBuffer : public RHI::RHIBuffer
{
public:
    WGPUBuffer mBuffer = nullptr;
    u64 mBufferSize = 0;
    WGPUBufferUsage mUsageFlags = WGPUBufferUsage_None;
    void* mMappedData = nullptr;
    bool mMapped = false;

    ~WebGPUBuffer() override;
    void* Map() override;
    void Unmap() override;
    u64 Size() const override { return mBufferSize; }
};

//
// WebGPU shader module.
//
class WebGPUShader : public RHI::RHIShader
{
public:
    WGPUShaderModule mModule = nullptr;
    RHI::ShaderStage mStage = RHI::ShaderStage::Vertex;
    String mEntryPoint;

    ~WebGPUShader() override;
};

//
// WebGPU sampler.
//
class WebGPUSampler : public RHI::RHISampler
{
public:
    WGPUSampler mSampler = nullptr;

    ~WebGPUSampler() override;
};

//
// WebGPU texture view. Also used as a lightweight wrapper around swapchain
// views that the backend does not own.
//
class WebGPUTextureView : public RHI::RHITextureView
{
public:
    WGPUTextureView mView = nullptr;
    WGPUTexture mTexture = nullptr;

    ~WebGPUTextureView() override;
};

//
// WebGPU texture with its default view.
//
class WebGPUTexture : public RHI::RHITexture
{
public:
    WGPUTexture mTexture = nullptr;
    RefPtr<WebGPUTextureView> mTextureView;
    u32 mWidth = 0;
    u32 mHeight = 0;

    WebGPUTexture() = default;
    ~WebGPUTexture() override;
    u32 Width() const override { return mWidth; }
    u32 Height() const override { return mHeight; }
    RHI::RHITextureView* GetView() override;
};

//
// WebGPU bind group.
//
class WebGPUBindGroup : public RHI::RHIBindGroup
{
public:
    WGPUBindGroup mBindGroup = nullptr;

    ~WebGPUBindGroup() override;
};

//
// WebGPU render pipeline.
//
class WebGPURenderPipeline : public RHI::RHIRenderPipeline
{
public:
    WGPURenderPipeline mPipeline = nullptr;

    ~WebGPURenderPipeline() override;
};

//
// WebGPU swapchain over an Emscripten canvas surface, with an owned depth
// attachment of matching size.
//
class WebGPUSwapchain : public RHI::RHISwapchain
{
public:
    WGPUSurface mSurface = nullptr;
    WGPUTexture mCurrentTexture = nullptr;
    WGPUTextureView mCurrentView = nullptr;
    WGPUDevice mDepthDevice = nullptr;
    WGPUTexture mDepthTexture = nullptr;
    WGPUTextureView mDepthView = nullptr;
    u32 mWidth = 0;
    u32 mHeight = 0;
    RHI::Format mColorFormat = RHI::Format::BGRA8Unorm;
    RHI::Format mDepthFormat = RHI::Format::Undefined;
    bool mConfigured = false;

    WebGPUSwapchain() = default;
    ~WebGPUSwapchain() override;
    u32 Width() const override { return mWidth; }
    u32 Height() const override { return mHeight; }
    RHI::RHITextureView* GetCurrentView() override;
    RHI::RHITextureView* GetDepthView() override;
    RHI::Format ColorFormat() const override { return mColorFormat; }
    RHI::Format DepthFormat() const override { return mDepthFormat; }
    bool Present() override;

    void Configure(WGPUDevice device);
};

//
// WebGPU command encoder wrapping a render pass encoder and the queue used
// for submission.
//
class WebGPUCommandEncoder : public RHI::RHICommandEncoder
{
public:
    WGPUCommandEncoder mEncoder = nullptr;
    WGPURenderPassEncoder mRenderPass = nullptr;
    WGPUDevice mDevice = nullptr;
    WGPUQueue mQueue = nullptr;
    WGPUCommandBuffer mCommandBuffer = nullptr;

    WebGPUCommandEncoder(WGPUDevice device, WGPUQueue queue);
    ~WebGPUCommandEncoder() override;
    void BeginRenderPass(const RHI::RenderPassDesc& desc) override;
    void EndRenderPass() override;
    void SetPipeline(RHI::RHIRenderPipeline* pipeline) override;
    void SetBindGroup(u32 groupIndex, RHI::RHIBindGroup* bindGroup) override;
    void SetVertexBuffer(u32 slot, RHI::RHIBuffer* buffer, u64 offset) override;
    void SetIndexBuffer(RHI::RHIBuffer* buffer, u64 offset, RHI::IndexFormat format) override;
    void Draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance) override;
    void DrawIndexed(u32 indexCount, u32 instanceCount, u32 firstIndex, i32 vertexOffset, u32 firstInstance) override;
    void Finish() override;
    void Submit() override;
};

//
// WebGPU RHI device. Initialization is asynchronous on the web, so Init
// starts the request chain and Tick/StepInit drive it to Ready/Failed.
//
class WebGPUDevice : public RHI::RHIDevice
{
public:
    enum class InitState
    {
        NotStarted,
        CreatingInstance,
        InstanceCreated,
        SurfaceCreated,
        RequestingAdapter,
        AdapterReceived,
        RequestingDevice,
        DeviceReceived,
        Ready,
        Failed,
    };

    WGPUInstance mInstance = nullptr;
    WGPUAdapter mAdapter = nullptr;
    WGPUDevice mDevice = nullptr;
    WGPUQueue mQueue = nullptr;
    WGPUSurface mSurface = nullptr;
    HashMap<u64, WGPUBindGroupLayout> mBindGroupLayoutCache;
    RefPtr<RHI::RHISwapchain> mCurrentSwapchain;
    InitState mInitState = InitState::NotStarted;

    bool mAdapterDone = false;
    bool mDeviceDone = false;
    WGPURequestAdapterStatus mAdapterStatus = WGPURequestAdapterStatus_Success;
    WGPURequestDeviceStatus mDeviceStatus = WGPURequestDeviceStatus_Success;
    WGPUAdapter mPendingAdapter = nullptr;
    WGPUDevice mPendingDevice = nullptr;

    ~WebGPUDevice() override;
    RHI::BackendType BackendType() const override { return RHI::BackendType::WebGPU; }
    bool Init(void* nativeWindowHandle) override;
    void WaitIdle() override;
    void Tick() override;

    bool IsReady() const override { return mInitState == InitState::Ready; }
    bool IsFailed() const override { return mInitState == InitState::Failed; }

    RefPtr<RHI::RHISwapchain> CreateSwapchain(u32 width, u32 height) override;
    RefPtr<RHI::RHIBuffer> CreateBuffer(const RHI::BufferDesc& desc, const void* initialData) override;
    RefPtr<RHI::RHIShader> CreateShader(RHI::ShaderStage stage, const RHI::ShaderModuleDesc& desc) override;
    RefPtr<RHI::RHITexture> CreateTexture(const RHI::TextureDesc& desc) override;
    RefPtr<RHI::RHISampler> CreateSampler(const RHI::SamplerDesc& desc) override;
    RefPtr<RHI::RHIRenderPipeline> CreateRenderPipeline(const RHI::RenderPipelineDesc& desc) override;
    RefPtr<RHI::RHIBindGroup> CreateBindGroup(const RHI::BindGroupLayoutDesc& layoutDesc, const Array<RHI::BindGroupEntry>& entries) override;
    void UpdateBuffer(RHI::RHIBuffer* buffer, u64 offset, const void* data, u64 size) override;
    void UpdateTexture(RHI::RHITexture* texture, const void* data) override;
    UniquePtr<RHI::RHICommandEncoder> CreateCommandEncoder() override;

private:
    void StepInit();
    WGPUBindGroupLayout GetBindGroupLayout(const RHI::BindGroupLayoutDesc& layoutDesc);
    static void OnAdapterRequested(WGPURequestAdapterStatus status, WGPUAdapter adapter,
                                   WGPUStringView message, void* userdata1, void* userdata2);
    static void OnDeviceRequested(WGPURequestDeviceStatus status, WGPUDevice device,
                                  WGPUStringView message, void* userdata1, void* userdata2);
};

}
