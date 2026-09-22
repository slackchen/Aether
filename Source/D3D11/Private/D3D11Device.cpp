#include "D3D11Internal.h"
#include <cstdio>
#include <cstring>

namespace Aether::D3D11 {

namespace {

const char* const INPUT_SEMANTIC = "ATTRIB";

DXGI_FORMAT VertexFormat(RHI::Format format)
{
    switch (format)
    {
        case RHI::Format::Float32x2: return DXGI_FORMAT_R32G32_FLOAT;
        case RHI::Format::Float32x3: return DXGI_FORMAT_R32G32B32_FLOAT;
        case RHI::Format::Float32x4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case RHI::Format::RGBA8Unorm: return DXGI_FORMAT_R8G8B8A8_UNORM;
        default: return DXGI_FORMAT_R32G32B32_FLOAT;
    }
}

D3D11_PRIMITIVE_TOPOLOGY ConvertTopology(RHI::PrimitiveTopology topology)
{
    switch (topology)
    {
        case RHI::PrimitiveTopology::PointList: return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
        case RHI::PrimitiveTopology::LineList: return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
        case RHI::PrimitiveTopology::LineStrip: return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
        case RHI::PrimitiveTopology::TriangleStrip: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        default: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    }
}

}  // namespace

bool D3D11Device::Init(void* nativeWindowHandle)
{
    mHwnd = (HWND)nativeWindowHandle;
    if (!mHwnd)
    {
        printf("D3D11: no native window handle\n");
        return false;
    }

    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0};

    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                                   flags,
                                   levels, 1, D3D11_SDK_VERSION, &mDevice, nullptr, &mContext);
    if (FAILED(hr))
    {
        printf("D3D11: hardware device creation failed (0x%08lx), trying WARP\n", (unsigned long)hr);
        hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, flags,
                               levels, 1, D3D11_SDK_VERSION, &mDevice, nullptr, &mContext);
    }
    if (FAILED(hr))
    {
        printf("D3D11: device creation failed (0x%08lx)\n", (unsigned long)hr);
        return false;
    }
    mDevice.As(&mInfoQueue);
    if (mInfoQueue)
    {
        printf("D3D11: debug info queue available\n");
        fflush(stdout);
    }

    ComPtr<IDXGIDevice> dxgiDevice;
    ComPtr<IDXGIAdapter> adapter;
    ComPtr<IDXGIFactory2> factory;
    if (FAILED(mDevice.As(&dxgiDevice)) ||
        FAILED(dxgiDevice->GetAdapter(&adapter)) ||
        FAILED(adapter->GetParent(IID_PPV_ARGS(&factory))))
    {
        printf("D3D11: failed to get DXGI factory\n");
        return false;
    }

    DXGI_SWAP_CHAIN_DESC1 desc1 = {};
    desc1.Width = 1280;
    desc1.Height = 720;
    desc1.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc1.Stereo = FALSE;
    desc1.SampleDesc.Count = 1;
    desc1.SampleDesc.Quality = 0;
    desc1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    // 3 buffers + frame latency 3: the CPU assembles frame N+2 while the GPU
    // still renders N, so Present no longer stalls mid-frame work.
    desc1.BufferCount = 3;
    desc1.Scaling = DXGI_SCALING_STRETCH;
    desc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc1.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    desc1.Flags = 0;

    hr = factory->CreateSwapChainForHwnd(mDevice.Get(), mHwnd, &desc1, nullptr, nullptr, &mSwapchain);
    if (FAILED(hr))
    {
        printf("D3D11: failed to create swap chain (0x%08lx)\n", (unsigned long)hr);
        return false;
    }
    factory->MakeWindowAssociation(mHwnd, DXGI_MWA_NO_ALT_ENTER);

    ComPtr<IDXGISwapChain2> swapchain2;
    if (SUCCEEDED(mSwapchain.As(&swapchain2)))
    {
        swapchain2->SetMaximumFrameLatency(3);
    }

    printf("D3D11 device initialized\n");
    fflush(stdout);
    mInitOk = true;
    return true;
}

void D3D11Device::WaitIdle()
{
    if (mContext)
    {
        mContext->Flush();
    }
}

void D3D11Device::Tick()
{
    if (!mInfoQueue) return;
    UINT64 num = mInfoQueue->GetNumStoredMessages();
    for (UINT64 i = 0; i < num; i++)
    {
        SIZE_T len = 0;
        mInfoQueue->GetMessage(i, nullptr, &len);
        Array<u8> buf;
        buf.Resize((u32)len);
        D3D11_MESSAGE* msg = (D3D11_MESSAGE*)buf.Data();
        if (SUCCEEDED(mInfoQueue->GetMessage(i, msg, &len)))
        {
            printf("D3D11 [%s]: %s\n",
                   msg->Severity == D3D11_MESSAGE_SEVERITY_ERROR ? "error" :
                   msg->Severity == D3D11_MESSAGE_SEVERITY_WARNING ? "warning" : "info",
                   msg->pDescription ? msg->pDescription : "?");
        }
    }
    if (num > 0)
    {
        mInfoQueue->ClearStoredMessages();
        fflush(stdout);
    }
}

RefPtr<RHI::RHISwapchain> D3D11Device::CreateSwapchain(u32 width, u32 height)
{
    if (!mSwapchain) return nullptr;
    RefPtr<D3D11Swapchain> swapchain = MakeRef<D3D11Swapchain>(mDevice, mSwapchain, mHwnd, width, height);
    mCurrentSwapchain = swapchain;
    return swapchain;
}

RefPtr<RHI::RHIBuffer> D3D11Device::CreateBuffer(const RHI::BufferDesc& desc, const void* initialData)
{
    if (!mDevice) return nullptr;

    bool constant = (desc.Usage & (u32)RHI::BufferUsage::Uniform) != 0;
    u64 byteWidth = constant ? ((desc.Size + 15) & ~(u64)15) : desc.Size;
    if (byteWidth < 16) byteWidth = 16;

    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = (UINT)byteWidth;
    bd.Usage = D3D11_USAGE_DEFAULT;
    if (desc.MemoryType == RHI::BufferMemoryType::HostVisible ||
        desc.MemoryType == RHI::BufferMemoryType::HostCoherent)
    {
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    }
    if (constant) bd.BindFlags |= D3D11_BIND_CONSTANT_BUFFER;
    if (desc.Usage & (u32)RHI::BufferUsage::Vertex) bd.BindFlags |= D3D11_BIND_VERTEX_BUFFER;
    if (desc.Usage & (u32)RHI::BufferUsage::Index) bd.BindFlags |= D3D11_BIND_INDEX_BUFFER;
    if (desc.Usage & (u32)RHI::BufferUsage::Storage) bd.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

    RefPtr<D3D11Buffer> buf = MakeRef<D3D11Buffer>();
    buf->mContext = mContext;
    buf->mSize = desc.Size;
    buf->mDynamic = bd.Usage == D3D11_USAGE_DYNAMIC;
    // 动态缓冲走 Map 路径, 不需要整块 CPU 镜像 (省内存)
    if (!buf->mDynamic || initialData)
    {
        buf->mCpuData.Resize((u32)byteWidth);
        if (initialData)
        {
            memcpy(buf->mCpuData.Data(), initialData, desc.Size);
        }
    }

    if (bd.Usage == D3D11_USAGE_DYNAMIC && initialData)
    {
        if (FAILED(mDevice->CreateBuffer(&bd, nullptr, &buf->mBuffer)))
        {
            printf("D3D11: failed to create dynamic buffer\n");
            return nullptr;
        }
        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(mContext->Map(buf->mBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            memcpy(mapped.pData, initialData, desc.Size);
            mContext->Unmap(buf->mBuffer.Get(), 0);
        }
    }
    else
    {
        D3D11_SUBRESOURCE_DATA init = {};
        init.pSysMem = initialData;
        if (FAILED(mDevice->CreateBuffer(&bd, initialData ? &init : nullptr, &buf->mBuffer)))
        {
            printf("D3D11: failed to create buffer\n");
            return nullptr;
        }
    }

    return buf;
}

RefPtr<RHI::RHITexture> D3D11Device::CreateTexture(const RHI::TextureDesc& desc)
{
    if (!mDevice) return nullptr;

    D3D11_TEXTURE2D_DESC td = {};
    td.Width = desc.Width;
    td.Height = desc.Height;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.SampleDesc.Quality = 0;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    td.CPUAccessFlags = 0;

    RefPtr<D3D11Texture> tex = MakeRef<D3D11Texture>();
    tex->mWidth = desc.Width;
    tex->mHeight = desc.Height;

    D3D11_SUBRESOURCE_DATA init = {};
    init.pSysMem = desc.InitialData;
    init.SysMemPitch = desc.Width * 4;
    init.SysMemSlicePitch = 0;

    if (FAILED(mDevice->CreateTexture2D(&td, desc.InitialData ? &init : nullptr, &tex->mTexture)))
    {
        printf("D3D11: failed to create texture\n");
        return nullptr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
    srvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvd.Texture2D.MipLevels = 1;

    RefPtr<D3D11TextureView> view = MakeRef<D3D11TextureView>();
    if (FAILED(mDevice->CreateShaderResourceView(tex->mTexture.Get(), &srvd, &view->mSrv)))
    {
        printf("D3D11: failed to create texture SRV\n");
        return nullptr;
    }
    tex->mView = view;
    return tex;
}

RefPtr<RHI::RHISampler> D3D11Device::CreateSampler(const RHI::SamplerDesc& desc)
{
    if (!mDevice) return nullptr;

    D3D11_SAMPLER_DESC sd = {};
    bool point = desc.MagFilter == RHI::TextureFilter::Nearest && desc.MinFilter == RHI::TextureFilter::Nearest;
    sd.Filter = point ? D3D11_FILTER_MIN_MAG_MIP_POINT : D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = desc.AddressModeU == RHI::TextureAddressMode::Repeat ? D3D11_TEXTURE_ADDRESS_WRAP : D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressV = desc.AddressModeV == RHI::TextureAddressMode::Repeat ? D3D11_TEXTURE_ADDRESS_WRAP : D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.MinLOD = 0.0f;
    sd.MaxLOD = 32.0f;
    sd.MaxAnisotropy = 1;
    sd.ComparisonFunc = D3D11_COMPARISON_NEVER;

    RefPtr<D3D11Sampler> sampler = MakeRef<D3D11Sampler>();
    if (FAILED(mDevice->CreateSamplerState(&sd, &sampler->mSampler)))
    {
        printf("D3D11: failed to create sampler\n");
        return nullptr;
    }
    return sampler;
}

RefPtr<RHI::RHIRenderPipeline> D3D11Device::CreateRenderPipeline(const RHI::RenderPipelineDesc& desc)
{
    if (!mDevice) return nullptr;

    const D3D11Shader* vs = static_cast<const D3D11Shader*>(desc.VertexShader);
    const D3D11Shader* fs = static_cast<const D3D11Shader*>(desc.FragmentShader);
    if (!vs || !vs->mVertexShader)
    {
        printf("D3D11: pipeline requires a vertex shader\n");
        return nullptr;
    }

    RefPtr<D3D11RenderPipeline> pipeline = MakeRef<D3D11RenderPipeline>();
    pipeline->mVertexShader = vs->mVertexShader;
    pipeline->mPixelShader = fs ? fs->mPixelShader : nullptr;
    pipeline->mTopology = ConvertTopology(desc.PrimitiveTopology);
    pipeline->mVertexStride = desc.VertexLayout.Stride;

    if (desc.VertexLayout.AttributeCount > 0 && desc.VertexLayout.Attributes)
    {
        Array<D3D11_INPUT_ELEMENT_DESC> elems;
        elems.Reserve(desc.VertexLayout.AttributeCount);
        for (u32 i = 0; i < desc.VertexLayout.AttributeCount; i++)
        {
            const RHI::VertexAttribute& attr = desc.VertexLayout.Attributes[i];
            D3D11_INPUT_ELEMENT_DESC el = {};
            el.SemanticName = INPUT_SEMANTIC;
            el.SemanticIndex = attr.Location;
            el.Format = VertexFormat(attr.Format);
            el.InputSlot = 0;
            el.AlignedByteOffset = attr.Offset;
            el.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
            elems.Add(el);
        }

        if (!vs->mBlob)
        {
            printf("D3D11: no compiled vertex shader blob available\n");
            return nullptr;
        }

        HRESULT inputLayoutHr = mDevice->CreateInputLayout(elems.Data(), (UINT)elems.Count(),
                                                  vs->mBlob->GetBufferPointer(),
                                                  vs->mBlob->GetBufferSize(),
                                                  &pipeline->mInputLayout);
        if (FAILED(inputLayoutHr))
        {
            printf("D3D11: failed to create input layout (0x%08lx)\n", (unsigned long)inputLayoutHr);
            ComPtr<ID3D11ShaderReflection> refl;
            if (SUCCEEDED(D3DReflect(vs->mBlob->GetBufferPointer(), vs->mBlob->GetBufferSize(),
                                     IID_ID3D11ShaderReflection, (void**)&refl)))
            {
                D3D11_SHADER_DESC sd = {};
                refl->GetDesc(&sd);
                printf("  VS inputs: %u\n", (unsigned)sd.InputParameters);
                for (UINT i = 0; i < sd.InputParameters; i++)
                {
                    D3D11_SIGNATURE_PARAMETER_DESC pd = {};
                    refl->GetInputParameterDesc(i, &pd);
                    printf("    %s[%u] type=%u mask=%u\n", pd.SemanticName, pd.SemanticIndex, pd.ComponentType, pd.Mask);
                }
            }
            return nullptr;
        }
    }

    D3D11_RASTERIZER_DESC rd = {};
    rd.FillMode = D3D11_FILL_SOLID;
    switch (desc.CullMode)
    {
        case RHI::CullMode::Front: rd.CullMode = D3D11_CULL_FRONT; break;
        case RHI::CullMode::Back:  rd.CullMode = D3D11_CULL_BACK;  break;
        default:                   rd.CullMode = D3D11_CULL_NONE;  break;
    }
    rd.FrontCounterClockwise = desc.FrontFace == RHI::FrontFace::CCW ? TRUE : FALSE;
    rd.DepthClipEnable = FALSE;
    rd.ScissorEnable = FALSE;
    rd.MultisampleEnable = FALSE;
    rd.AntialiasedLineEnable = FALSE;
    if (FAILED(mDevice->CreateRasterizerState(&rd, &pipeline->mRasterizer)))
    {
        printf("D3D11: failed to create rasterizer state\n");
        return nullptr;
    }

    D3D11_BLEND_DESC bd = {};
    bd.AlphaToCoverageEnable = FALSE;
    bd.IndependentBlendEnable = FALSE;
    bool additive = desc.BlendMode == RHI::BlendMode::Additive;
    for (u32 i = 0; i < 8; i++)
    {
        bd.RenderTarget[i].BlendEnable = TRUE;
        if (additive)
        {
            bd.RenderTarget[i].SrcBlend = D3D11_BLEND_SRC_ALPHA;
            bd.RenderTarget[i].DestBlend = D3D11_BLEND_ONE;
            bd.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
            bd.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
            bd.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ONE;
            bd.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        }
        else
        {
            bd.RenderTarget[i].SrcBlend = D3D11_BLEND_SRC_ALPHA;
            bd.RenderTarget[i].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
            bd.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
            bd.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
            bd.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
            bd.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        }
        bd.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    }
    if (FAILED(mDevice->CreateBlendState(&bd, &pipeline->mBlend)))
    {
        printf("D3D11: failed to create blend state\n");
        return nullptr;
    }

    // 深度状态: 未提供 DepthStencil 时显式禁用深度 (2D 精灵管线保持原行为)
    D3D11_DEPTH_STENCIL_DESC dsd = {};
    if (desc.DepthStencil)
    {
        dsd.DepthEnable = TRUE;
        dsd.DepthWriteMask = desc.DepthStencil->DepthWriteEnabled ? D3D11_DEPTH_WRITE_MASK_ALL
                                                                  : D3D11_DEPTH_WRITE_MASK_ZERO;
        switch (desc.DepthStencil->DepthCompare)
        {
            case RHI::CompareOp::Never:          dsd.DepthFunc = D3D11_COMPARISON_NEVER; break;
            case RHI::CompareOp::Equal:          dsd.DepthFunc = D3D11_COMPARISON_EQUAL; break;
            case RHI::CompareOp::LessOrEqual:    dsd.DepthFunc = D3D11_COMPARISON_LESS_EQUAL; break;
            case RHI::CompareOp::Greater:        dsd.DepthFunc = D3D11_COMPARISON_GREATER; break;
            case RHI::CompareOp::NotEqual:       dsd.DepthFunc = D3D11_COMPARISON_NOT_EQUAL; break;
            case RHI::CompareOp::GreaterOrEqual: dsd.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL; break;
            case RHI::CompareOp::Always:         dsd.DepthFunc = D3D11_COMPARISON_ALWAYS; break;
            default:                             dsd.DepthFunc = D3D11_COMPARISON_LESS; break;
        }
    }
    else
    {
        dsd.DepthEnable = FALSE;
        dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        dsd.DepthFunc = D3D11_COMPARISON_ALWAYS;
    }
    dsd.StencilEnable = FALSE;
    if (FAILED(mDevice->CreateDepthStencilState(&dsd, &pipeline->mDepthStencil)))
    {
        printf("D3D11: failed to create depth stencil state\n");
        return nullptr;
    }

    return pipeline;
}

RefPtr<RHI::RHIBindGroup> D3D11Device::CreateBindGroup(const RHI::BindGroupLayoutDesc& layoutDesc,
                                                      const Array<RHI::BindGroupEntry>& entries)
{
    (void)layoutDesc;
    RefPtr<D3D11BindGroup> bg = MakeRef<D3D11BindGroup>();
    for (const RHI::BindGroupEntry& e : entries)
    {
        if (e.Binding == 0 && e.Buffer)
        {
            bg->mConstantBuffer = static_cast<D3D11Buffer*>(e.Buffer)->mBuffer;
        }
        else if (e.Binding == 1 && e.TextureView)
        {
            bg->mShaderResourceView = static_cast<D3D11TextureView*>(e.TextureView)->mSrv;
        }
        else if (e.Binding == 2 && e.Sampler)
        {
            bg->mSampler = static_cast<D3D11Sampler*>(e.Sampler)->mSampler;
        }
    }
    return bg;
}

void D3D11Device::UpdateBuffer(RHI::RHIBuffer* buffer, u64 offset, const void* data, u64 size)
{
    D3D11Buffer* buf = static_cast<D3D11Buffer*>(buffer);
    if (!buf || !buf->mBuffer || !data || !size || !mContext) return;
    if (offset + size > buf->mSize) return;

    if (buf->mDynamic)
    {
        // offset 0 的更新视为 "每帧新内容": DISCARD 让驱动重命名后台缓冲,
        // CPU 不必等 GPU 读完上一帧数据 (NO_OVERWRITE 在 offset 0 反而会阻塞)。
        D3D11_MAP mapType = offset == 0 ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;
        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(mContext->Map(buf->mBuffer.Get(), 0, mapType, 0, &mapped)))
        {
            memcpy((u8*)mapped.pData + offset, data, size);
            mContext->Unmap(buf->mBuffer.Get(), 0);
        }
        return;
    }

    if (offset + size <= buf->mCpuData.Count())
    {
        memcpy(buf->mCpuData.Data() + offset, data, size);
    }

    // 局部上传: staging + CopySubresourceRegion。
    // UpdateSubresource 只能整块重传 (精灵 VB 曾是 64MB/帧×2), 且有驱动同步开销。
    if (mUploadStagingSize < size)
    {
        UINT cap = 1 << 20;                     // 起步 1MB, 按 2 的幂增长
        while (cap < size) cap <<= 1;
        D3D11_BUFFER_DESC sd = {};
        sd.ByteWidth = cap;
        sd.Usage = D3D11_USAGE_STAGING;
        sd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        if (SUCCEEDED(mDevice->CreateBuffer(&sd, nullptr, &mUploadStaging)))
        {
            mUploadStagingSize = cap;
        }
    }
    if (mUploadStaging)
    {
        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(mContext->Map(mUploadStaging.Get(), 0, D3D11_MAP_WRITE, 0, &mapped)))
        {
            memcpy(mapped.pData, data, size);
            mContext->Unmap(mUploadStaging.Get(), 0);
            D3D11_BOX box{0, 0, 0, (UINT)size, 1, 1};
            mContext->CopySubresourceRegion(buf->mBuffer.Get(), 0, (UINT)offset, 0, 0,
                                           mUploadStaging.Get(), 0, &box);
        }
    }
}

void D3D11Device::UpdateTexture(RHI::RHITexture* texture, const void* data)
{
    D3D11Texture* tex = static_cast<D3D11Texture*>(texture);
    if (!tex || !tex->mTexture || !data || !mContext) return;
    mContext->UpdateSubresource(tex->mTexture.Get(), 0, nullptr, data, tex->mWidth * 4, 0);
}

UniquePtr<RHI::RHICommandEncoder> D3D11Device::CreateCommandEncoder()
{
    if (!mContext) return UniquePtr<RHI::RHICommandEncoder>();
    f32 w = 1.0f;
    f32 h = 1.0f;
    if (mCurrentSwapchain)
    {
        w = (f32)mCurrentSwapchain->mWidth;
        h = (f32)mCurrentSwapchain->mHeight;
    }
    return UniquePtr<RHI::RHICommandEncoder>(new D3D11CommandEncoder(mContext, w, h));
}

void D3D11Device::ToggleFullscreen()
{
    if (mCurrentSwapchain)
    {
        mCurrentSwapchain->ToggleFullscreen();
    }
}

}

namespace Aether::RHI {

UniquePtr<RHIDevice> CreateD3D11Device()
{
    return UniquePtr<RHIDevice>(new D3D11::D3D11Device());
}

}
