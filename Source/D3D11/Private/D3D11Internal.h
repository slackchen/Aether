#pragma once

#include "RHI.h"

#ifndef __EMSCRIPTEN__
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#endif

namespace Aether::D3D11 {

#ifndef __EMSCRIPTEN__

using Microsoft::WRL::ComPtr;

//
// D3D11 后端对 RHI 资源接口的实现。
//
class D3D11Buffer : public RHI::RHIBuffer
{
public:
    ComPtr<ID3D11Buffer> mBuffer;
    ComPtr<ID3D11DeviceContext> mContext;
    u64 mSize = 0;
    bool mDynamic = false;
    bool mMapped = false;
    bool mDirty = false;
    Array<u8> mCpuData;

    ~D3D11Buffer() override = default;
    void* Map() override;
    void Unmap() override;
    u64 Size() const override { return mSize; }
};

class D3D11Shader : public RHI::RHIShader
{
public:
    RHI::ShaderStage mStage = RHI::ShaderStage::Vertex;
    ComPtr<ID3D11VertexShader> mVertexShader;
    ComPtr<ID3D11PixelShader> mPixelShader;
    ComPtr<ID3DBlob> mBlob;

    ~D3D11Shader() override = default;
};

class D3D11TextureView : public RHI::RHITextureView
{
public:
    ComPtr<ID3D11ShaderResourceView> mSrv;
    ComPtr<ID3D11RenderTargetView> mRtv;
    ComPtr<ID3D11DepthStencilView> mDsv;

    ~D3D11TextureView() override = default;
};

class D3D11Texture : public RHI::RHITexture
{
public:
    ComPtr<ID3D11Texture2D> mTexture;
    RefPtr<D3D11TextureView> mView;
    u32 mWidth = 0;
    u32 mHeight = 0;

    ~D3D11Texture() override = default;
    u32 Width() const override { return mWidth; }
    u32 Height() const override { return mHeight; }
    RHI::RHITextureView* GetView() override;
};

class D3D11Sampler : public RHI::RHISampler
{
public:
    ComPtr<ID3D11SamplerState> mSampler;

    ~D3D11Sampler() override = default;
};

class D3D11BindGroup : public RHI::RHIBindGroup
{
public:
    ComPtr<ID3D11Buffer> mConstantBuffer;
    ComPtr<ID3D11ShaderResourceView> mShaderResourceView;
    ComPtr<ID3D11SamplerState> mSampler;

    ~D3D11BindGroup() override = default;
};

class D3D11RenderPipeline : public RHI::RHIRenderPipeline
{
public:
    ComPtr<ID3D11VertexShader> mVertexShader;
    ComPtr<ID3D11PixelShader> mPixelShader;
    ComPtr<ID3D11InputLayout> mInputLayout;
    ComPtr<ID3D11RasterizerState> mRasterizer;
    ComPtr<ID3D11BlendState> mBlend;
    ComPtr<ID3D11DepthStencilState> mDepthStencil;
    u32 mVertexStride = 0;
    D3D11_PRIMITIVE_TOPOLOGY mTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    ~D3D11RenderPipeline() override = default;
};

class D3D11Swapchain : public RHI::RHISwapchain
{
public:
    ComPtr<ID3D11Device> mDevice;
    ComPtr<IDXGISwapChain1> mSwapchain;
    ComPtr<ID3D11RenderTargetView> mRtv;
    ComPtr<ID3D11Texture2D> mBackBuffer;
    ComPtr<ID3D11Texture2D> mDepthBuffer;
    ComPtr<ID3D11DepthStencilView> mDsv;
    HWND mHwnd = nullptr;
    u32 mWidth = 0;
    u32 mHeight = 0;
    u32 mRtvWidth = 0;
    u32 mRtvHeight = 0;
    RHI::Format mColorFormat = RHI::Format::BGRA8Unorm;
    bool mConfigured = false;
    LONG_PTR mSavedStyle = 0;
    RECT mSavedRect = {};
    bool mBorderless = false;

    D3D11Swapchain(ComPtr<ID3D11Device> device, ComPtr<IDXGISwapChain1> swapchain, HWND hwnd, u32 width, u32 height);
    ~D3D11Swapchain() override = default;
    u32 Width() const override { return mWidth; }
    u32 Height() const override { return mHeight; }
    RHI::RHITextureView* GetCurrentView() override;
    RHI::RHITextureView* GetDepthView() override;
    RHI::Format ColorFormat() const override { return mColorFormat; }
    RHI::Format DepthFormat() const override { return RHI::Format::Depth32Float; }
    bool Present() override;
    void ToggleFullscreen();
    void EnsureDepthBuffer();
};

class D3D11CommandEncoder : public RHI::RHICommandEncoder
{
public:
    ComPtr<ID3D11DeviceContext> mContext;
    u32 mVertexStride = 0;
    f32 mViewportWidth = 1.0f;
    f32 mViewportHeight = 1.0f;

    explicit D3D11CommandEncoder(ComPtr<ID3D11DeviceContext> context, f32 width, f32 height);
    ~D3D11CommandEncoder() override = default;
    void BeginRenderPass(const RHI::RenderPassDesc& desc) override;
    void EndRenderPass() override;
    void SetPipeline(RHI::RHIRenderPipeline* pipeline) override;
    void SetBindGroup(u32 groupIndex, RHI::RHIBindGroup* bindGroup) override;
    void SetVertexBuffer(u32 slot, RHI::RHIBuffer* buffer, u64 offset = 0) override;
    void SetIndexBuffer(RHI::RHIBuffer* buffer, u64 offset = 0, RHI::IndexFormat format = RHI::IndexFormat::Uint32) override;
    void Draw(u32 vertexCount, u32 instanceCount = 1, u32 firstVertex = 0, u32 firstInstance = 0) override;
    void DrawIndexed(u32 indexCount, u32 instanceCount = 1, u32 firstIndex = 0, i32 vertexOffset = 0, u32 firstInstance = 0) override;
    void Finish() override;
    void Submit() override;
};

class D3D11Device : public RHI::RHIDevice
{
public:
    ComPtr<ID3D11Device> mDevice;
    ComPtr<ID3D11DeviceContext> mContext;
    ComPtr<IDXGISwapChain1> mSwapchain;
    ComPtr<ID3D11InfoQueue> mInfoQueue;
    HWND mHwnd = nullptr;
    bool mInitOk = false;
    RefPtr<D3D11Swapchain> mCurrentSwapchain;

    // 局部上传用的 grow-only staging 缓冲 (避免 UpdateSubresource 整块重传)
    ComPtr<ID3D11Buffer> mUploadStaging;
    u64 mUploadStagingSize = 0;

    ~D3D11Device() override = default;
    RHI::BackendType BackendType() const override { return RHI::BackendType::D3D11; }
    bool Init(void* nativeWindowHandle = nullptr) override;
    void WaitIdle() override;
    void Tick() override;
    bool IsReady() const override { return mInitOk; }
    bool IsFailed() const override { return !mInitOk; }

    RefPtr<RHI::RHISwapchain> CreateSwapchain(u32 width, u32 height) override;
    RefPtr<RHI::RHIBuffer> CreateBuffer(const RHI::BufferDesc& desc, const void* initialData = nullptr) override;
    RefPtr<RHI::RHIShader> CreateShader(RHI::ShaderStage stage, const RHI::ShaderModuleDesc& desc) override;
    RefPtr<RHI::RHITexture> CreateTexture(const RHI::TextureDesc& desc) override;
    RefPtr<RHI::RHISampler> CreateSampler(const RHI::SamplerDesc& desc) override;
    RefPtr<RHI::RHIRenderPipeline> CreateRenderPipeline(const RHI::RenderPipelineDesc& desc) override;
    RefPtr<RHI::RHIBindGroup> CreateBindGroup(const RHI::BindGroupLayoutDesc& layoutDesc, const Array<RHI::BindGroupEntry>& entries) override;
    void UpdateBuffer(RHI::RHIBuffer* buffer, u64 offset, const void* data, u64 size) override;
    void UpdateTexture(RHI::RHITexture* texture, const void* data) override;
    UniquePtr<RHI::RHICommandEncoder> CreateCommandEncoder() override;
    void ToggleFullscreen() override;
};

#endif

}
