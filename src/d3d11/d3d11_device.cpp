#include "d3d11/d3d11_internal.h"
#include <cstdio>
#include <cstring>
#include <vector>

namespace aether::d3d11 {

namespace {

const char* kInputSemantic = "ATTRIB";

DXGI_FORMAT vertex_format(rhi::Format format) {
    switch (format) {
        case rhi::Format::Float32x2: return DXGI_FORMAT_R32G32_FLOAT;
        case rhi::Format::Float32x3: return DXGI_FORMAT_R32G32B32_FLOAT;
        case rhi::Format::Float32x4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case rhi::Format::RGBA8Unorm: return DXGI_FORMAT_R8G8B8A8_UNORM;
        default: return DXGI_FORMAT_R32G32B32_FLOAT;
    }
}

D3D11_PRIMITIVE_TOPOLOGY convert_topology(rhi::PrimitiveTopology topology) {
    switch (topology) {
        case rhi::PrimitiveTopology::PointList: return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
        case rhi::PrimitiveTopology::LineList: return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
        case rhi::PrimitiveTopology::LineStrip: return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
        case rhi::PrimitiveTopology::TriangleStrip: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        default: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    }
}

}  // namespace

bool D3D11Device::init(void* native_window_handle) {
    hwnd = (HWND)native_window_handle;
    if (!hwnd) {
        printf("D3D11: no native window handle\n");
        return false;
    }

    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0};

    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                                   flags | D3D11_CREATE_DEVICE_DEBUG,
                                   levels, 1, D3D11_SDK_VERSION, &device, nullptr, &context);
    if (FAILED(hr)) {
        printf("D3D11: hardware debug device failed (0x%08lx), retrying without debug\n", (unsigned long)hr);
        hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
                               levels, 1, D3D11_SDK_VERSION, &device, nullptr, &context);
    }
    if (FAILED(hr)) {
        printf("D3D11: hardware device creation failed (0x%08lx), trying WARP\n", (unsigned long)hr);
        hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, flags,
                               levels, 1, D3D11_SDK_VERSION, &device, nullptr, &context);
    }
    if (FAILED(hr)) {
        printf("D3D11: device creation failed (0x%08lx)\n", (unsigned long)hr);
        return false;
    }
    device.As(&info_queue);
    if (info_queue) {
        printf("D3D11: debug info queue available\n");
        fflush(stdout);
    }

    ComPtr<IDXGIDevice> dxgi_device;
    ComPtr<IDXGIAdapter> adapter;
    ComPtr<IDXGIFactory2> factory;
    if (FAILED(device.As(&dxgi_device)) ||
        FAILED(dxgi_device->GetAdapter(&adapter)) ||
        FAILED(adapter->GetParent(IID_PPV_ARGS(&factory)))) {
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
    desc1.BufferCount = 2;
    desc1.Scaling = DXGI_SCALING_STRETCH;
    desc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc1.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    desc1.Flags = 0;

    hr = factory->CreateSwapChainForHwnd(device.Get(), hwnd, &desc1, nullptr, nullptr, &swapchain);
    if (FAILED(hr)) {
        printf("D3D11: failed to create swap chain (0x%08lx)\n", (unsigned long)hr);
        return false;
    }
    factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

    printf("D3D11 device initialized\n");
    fflush(stdout);
    init_ok = true;
    return true;
}

void D3D11Device::wait_idle() {
    if (context) {
        context->Flush();
    }
}

void D3D11Device::tick() {
    if (!info_queue) return;
    UINT64 num = info_queue->GetNumStoredMessages();
    for (UINT64 i = 0; i < num; i++) {
        SIZE_T len = 0;
        info_queue->GetMessage(i, nullptr, &len);
        std::vector<u8> buf(len);
        D3D11_MESSAGE* msg = (D3D11_MESSAGE*)buf.data();
        if (SUCCEEDED(info_queue->GetMessage(i, msg, &len))) {
            printf("D3D11 [%s]: %s\n",
                   msg->Severity == D3D11_MESSAGE_SEVERITY_ERROR ? "error" :
                   msg->Severity == D3D11_MESSAGE_SEVERITY_WARNING ? "warning" : "info",
                   msg->pDescription ? msg->pDescription : "?");
        }
    }
    if (num > 0) {
        info_queue->ClearStoredMessages();
        fflush(stdout);
    }
}

std::shared_ptr<rhi::RHISwapchain> D3D11Device::create_swapchain(u32 width, u32 height) {
    if (!swapchain) return nullptr;
    auto sc = std::make_shared<D3D11Swapchain>(device, swapchain, hwnd, width, height);
    current_swapchain = sc;
    return sc;
}

std::shared_ptr<rhi::RHIBuffer> D3D11Device::create_buffer(const rhi::BufferDesc& desc, const void* initial_data) {
    if (!device) return nullptr;

    bool constant = (desc.usage & (u32)rhi::BufferUsage::Uniform) != 0;
    u64 byte_width = constant ? ((desc.size + 15) & ~(u64)15) : desc.size;
    if (byte_width < 16) byte_width = 16;

    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = (UINT)byte_width;
    bd.Usage = D3D11_USAGE_DEFAULT;
    if (desc.memory_type == rhi::BufferMemoryType::HostVisible ||
        desc.memory_type == rhi::BufferMemoryType::HostCoherent) {
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    }
    if (constant) bd.BindFlags |= D3D11_BIND_CONSTANT_BUFFER;
    if (desc.usage & (u32)rhi::BufferUsage::Vertex) bd.BindFlags |= D3D11_BIND_VERTEX_BUFFER;
    if (desc.usage & (u32)rhi::BufferUsage::Index) bd.BindFlags |= D3D11_BIND_INDEX_BUFFER;
    if (desc.usage & (u32)rhi::BufferUsage::Storage) bd.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

    auto buf = std::make_shared<D3D11Buffer>();
    buf->ctx = context;
    buf->buffer_size = desc.size;
    buf->dynamic = bd.Usage == D3D11_USAGE_DYNAMIC;
    buf->cpu_data.assign((size_t)byte_width, 0);
    if (initial_data) {
        memcpy(buf->cpu_data.data(), initial_data, desc.size);
    }

    if (bd.Usage == D3D11_USAGE_DYNAMIC && initial_data) {
        if (FAILED(device->CreateBuffer(&bd, nullptr, &buf->buffer))) {
            printf("D3D11: failed to create dynamic buffer\n");
            return nullptr;
        }
        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(context->Map(buf->buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            memcpy(mapped.pData, initial_data, desc.size);
            context->Unmap(buf->buffer.Get(), 0);
        }
    } else {
        D3D11_SUBRESOURCE_DATA init = {};
        init.pSysMem = initial_data;
        if (FAILED(device->CreateBuffer(&bd, initial_data ? &init : nullptr, &buf->buffer))) {
            printf("D3D11: failed to create buffer\n");
            return nullptr;
        }
    }

    return buf;
}

std::shared_ptr<rhi::RHITexture> D3D11Device::create_texture(const rhi::TextureDesc& desc) {
    if (!device) return nullptr;

    D3D11_TEXTURE2D_DESC td = {};
    td.Width = desc.width;
    td.Height = desc.height;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.SampleDesc.Quality = 0;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    td.CPUAccessFlags = 0;

    auto tex = std::make_shared<D3D11Texture>();
    tex->tex_w = desc.width;
    tex->tex_h = desc.height;

    D3D11_SUBRESOURCE_DATA init = {};
    init.pSysMem = desc.initial_data;
    init.SysMemPitch = desc.width * 4;
    init.SysMemSlicePitch = 0;

    if (FAILED(device->CreateTexture2D(&td, desc.initial_data ? &init : nullptr, &tex->texture))) {
        printf("D3D11: failed to create texture\n");
        return nullptr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
    srvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvd.Texture2D.MipLevels = 1;

    auto view = std::make_shared<D3D11TextureView>();
    if (FAILED(device->CreateShaderResourceView(tex->texture.Get(), &srvd, &view->srv))) {
        printf("D3D11: failed to create texture SRV\n");
        return nullptr;
    }
    tex->texture_view = view;
    return tex;
}

std::shared_ptr<rhi::RHISampler> D3D11Device::create_sampler(const rhi::SamplerDesc& desc) {
    if (!device) return nullptr;

    D3D11_SAMPLER_DESC sd = {};
    bool point = desc.mag_filter == rhi::TextureFilter::Nearest && desc.min_filter == rhi::TextureFilter::Nearest;
    sd.Filter = point ? D3D11_FILTER_MIN_MAG_MIP_POINT : D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = desc.address_mode_u == rhi::TextureAddressMode::Repeat ? D3D11_TEXTURE_ADDRESS_WRAP : D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressV = desc.address_mode_v == rhi::TextureAddressMode::Repeat ? D3D11_TEXTURE_ADDRESS_WRAP : D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.MinLOD = 0.0f;
    sd.MaxLOD = 32.0f;
    sd.MaxAnisotropy = 1;
    sd.ComparisonFunc = D3D11_COMPARISON_NEVER;

    auto sampler = std::make_shared<D3D11Sampler>();
    if (FAILED(device->CreateSamplerState(&sd, &sampler->sampler))) {
        printf("D3D11: failed to create sampler\n");
        return nullptr;
    }
    return sampler;
}

std::shared_ptr<rhi::RHIRenderPipeline> D3D11Device::create_render_pipeline(const rhi::RenderPipelineDesc& desc) {
    if (!device) return nullptr;

    auto* vs = static_cast<const D3D11Shader*>(desc.vertex_shader);
    auto* fs = static_cast<const D3D11Shader*>(desc.fragment_shader);
    if (!vs || !vs->vs) {
        printf("D3D11: pipeline requires a vertex shader\n");
        return nullptr;
    }

    auto pipeline = std::make_shared<D3D11RenderPipeline>();
    pipeline->vs = vs->vs;
    pipeline->ps = fs ? fs->ps : nullptr;
    pipeline->topology = convert_topology(desc.primitive_topology);
    pipeline->vertex_stride = desc.vertex_layout.stride;

    if (desc.vertex_layout.attribute_count > 0 && desc.vertex_layout.attributes) {
        std::vector<D3D11_INPUT_ELEMENT_DESC> elems;
        elems.reserve(desc.vertex_layout.attribute_count);
        for (u32 i = 0; i < desc.vertex_layout.attribute_count; i++) {
            const auto& attr = desc.vertex_layout.attributes[i];
            D3D11_INPUT_ELEMENT_DESC el = {};
            el.SemanticName = kInputSemantic;
            el.SemanticIndex = attr.location;
            el.Format = vertex_format(attr.format);
            el.InputSlot = 0;
            el.AlignedByteOffset = attr.offset;
            el.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
            elems.push_back(el);
        }

        if (!vs->blob) {
            printf("D3D11: no compiled vertex shader blob available\n");
            return nullptr;
        }

        HRESULT il_hr = device->CreateInputLayout(elems.data(), (UINT)elems.size(),
                                                  vs->blob->GetBufferPointer(),
                                                  vs->blob->GetBufferSize(),
                                                  &pipeline->input_layout);
        if (FAILED(il_hr)) {
            printf("D3D11: failed to create input layout (0x%08lx)\n", (unsigned long)il_hr);
            ComPtr<ID3D11ShaderReflection> refl;
            if (SUCCEEDED(D3DReflect(vs->blob->GetBufferPointer(), vs->blob->GetBufferSize(),
                                     IID_ID3D11ShaderReflection, (void**)&refl))) {
                D3D11_SHADER_DESC sd = {};
                refl->GetDesc(&sd);
                printf("  VS inputs: %u\n", (unsigned)sd.InputParameters);
                for (UINT i = 0; i < sd.InputParameters; i++) {
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
    rd.CullMode = D3D11_CULL_NONE;
    rd.FrontCounterClockwise = desc.front_face == rhi::FrontFace::CCW ? TRUE : FALSE;
    rd.DepthClipEnable = FALSE;
    rd.ScissorEnable = FALSE;
    rd.MultisampleEnable = FALSE;
    rd.AntialiasedLineEnable = FALSE;
    if (FAILED(device->CreateRasterizerState(&rd, &pipeline->rasterizer))) {
        printf("D3D11: failed to create rasterizer state\n");
        return nullptr;
    }

    D3D11_BLEND_DESC bd = {};
    bd.AlphaToCoverageEnable = FALSE;
    bd.IndependentBlendEnable = FALSE;
    bool additive = desc.blend_mode == rhi::BlendMode::Additive;
    for (u32 i = 0; i < 8; i++) {
        bd.RenderTarget[i].BlendEnable = TRUE;
        if (additive) {
            bd.RenderTarget[i].SrcBlend = D3D11_BLEND_SRC_ALPHA;
            bd.RenderTarget[i].DestBlend = D3D11_BLEND_ONE;
            bd.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
            bd.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
            bd.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ONE;
            bd.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        } else {
            bd.RenderTarget[i].SrcBlend = D3D11_BLEND_SRC_ALPHA;
            bd.RenderTarget[i].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
            bd.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
            bd.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
            bd.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
            bd.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        }
        bd.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    }
    if (FAILED(device->CreateBlendState(&bd, &pipeline->blend))) {
        printf("D3D11: failed to create blend state\n");
        return nullptr;
    }

    return pipeline;
}

std::shared_ptr<rhi::RHIBindGroup> D3D11Device::create_bind_group(const rhi::BindGroupLayoutDesc& layout_desc,
                                                                  const std::vector<rhi::BindGroupEntry>& entries) {
    (void)layout_desc;
    auto bg = std::make_shared<D3D11BindGroup>();
    for (const auto& e : entries) {
        if (e.binding == 0 && e.buffer) {
            bg->constant_buffer = static_cast<D3D11Buffer*>(e.buffer)->buffer;
        } else if (e.binding == 1 && e.texture_view) {
            bg->srv = static_cast<D3D11TextureView*>(e.texture_view)->srv;
        } else if (e.binding == 2 && e.sampler) {
            bg->sampler = static_cast<D3D11Sampler*>(e.sampler)->sampler;
        }
    }
    return bg;
}

void D3D11Device::update_buffer(rhi::RHIBuffer* buffer, u64 offset, const void* data, u64 size) {
    auto* buf = static_cast<D3D11Buffer*>(buffer);
    if (!buf || !buf->buffer || !data || !size || !context) return;

    if (buf->dynamic) {
        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(context->Map(buf->buffer.Get(), 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mapped))) {
            memcpy((u8*)mapped.pData + offset, data, size);
            context->Unmap(buf->buffer.Get(), 0);
        }
        return;
    }

    if (offset + size <= buf->cpu_data.size()) {
        memcpy(buf->cpu_data.data() + offset, data, size);
        context->UpdateSubresource(buf->buffer.Get(), 0, nullptr, buf->cpu_data.data(), 0, 0);
        static u32 s_n = 0;
        if (s_n < 6) {
            const u8* d = (const u8*)buf->cpu_data.data();
            printf("UB: update %llu bytes, first=%u,%u,%u,%u,%u,%u,%u,%u dyn=%d\n",
                   (unsigned long long)size, d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], buf->dynamic);
            fflush(stdout);
            s_n++;
        }
    }
}

void D3D11Device::update_texture(rhi::RHITexture* texture, const void* data) {
    auto* tex = static_cast<D3D11Texture*>(texture);
    if (!tex || !tex->texture || !data || !context) return;
    context->UpdateSubresource(tex->texture.Get(), 0, nullptr, data, tex->tex_w * 4, 0);
}

std::unique_ptr<rhi::RHICommandEncoder> D3D11Device::create_command_encoder() {
    if (!context) return nullptr;
    f32 w = 1.0f;
    f32 h = 1.0f;
    if (current_swapchain) {
        w = (f32)current_swapchain->w;
        h = (f32)current_swapchain->h;
    }
    return std::make_unique<D3D11CommandEncoder>(context, w, h);
}

void D3D11Device::toggle_fullscreen() {
    if (current_swapchain) {
        current_swapchain->toggle_fullscreen();
    }
}

}

namespace aether::rhi {

std::unique_ptr<RHIDevice> create_d3d11_device() {
    return std::make_unique<d3d11::D3D11Device>();
}

}
