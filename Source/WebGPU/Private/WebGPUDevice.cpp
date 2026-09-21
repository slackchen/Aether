#include "WebGPUInternal.h"
#include <cstdio>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

namespace Aether::WebGPU {

WGPUBufferUsage ConvertBufferUsage(u32 usage)
{
    WGPUBufferUsage flags = WGPUBufferUsage_None;
    if (usage & (u32)RHI::BufferUsage::Vertex) flags |= WGPUBufferUsage_Vertex;
    if (usage & (u32)RHI::BufferUsage::Index) flags |= WGPUBufferUsage_Index;
    if (usage & (u32)RHI::BufferUsage::Uniform) flags |= WGPUBufferUsage_Uniform;
    if (usage & (u32)RHI::BufferUsage::Storage) flags |= WGPUBufferUsage_Storage;
    if (usage & (u32)RHI::BufferUsage::CopySrc) flags |= WGPUBufferUsage_CopySrc;
    if (usage & (u32)RHI::BufferUsage::CopyDst) flags |= WGPUBufferUsage_CopyDst;
    return flags;
}

WGPUTextureFormat ConvertFormat(RHI::Format format)
{
    switch (format)
    {
        case RHI::Format::RGBA8Unorm: return WGPUTextureFormat_RGBA8Unorm;
        case RHI::Format::BGRA8Unorm: return WGPUTextureFormat_BGRA8Unorm;
        case RHI::Format::Depth32Float: return WGPUTextureFormat_Depth32Float;
        case RHI::Format::Depth24PlusStencil8: return WGPUTextureFormat_Depth24PlusStencil8;
        default: return WGPUTextureFormat_Undefined;
    }
}

WGPUTextureUsage ConvertTextureUsage(u32 usage)
{
    WGPUTextureUsage flags = WGPUTextureUsage_None;
    if (usage & (u32)RHI::TextureUsage::Sampled) flags |= WGPUTextureUsage_TextureBinding;
    if (usage & (u32)RHI::TextureUsage::RenderAttachment) flags |= WGPUTextureUsage_RenderAttachment;
    if (usage & (u32)RHI::TextureUsage::CopyDst) flags |= WGPUTextureUsage_CopyDst;
    if (usage & (u32)RHI::TextureUsage::CopySrc) flags |= WGPUTextureUsage_CopySrc;
    return flags;
}

WGPUTextureFormat ConvertTextureFormat(RHI::TextureFormat format)
{
    switch (format)
    {
        case RHI::TextureFormat::RGBA8Unorm: return WGPUTextureFormat_RGBA8Unorm;
        default: return WGPUTextureFormat_Undefined;
    }
}

WGPUFilterMode ConvertTextureFilter(RHI::TextureFilter filter)
{
    return filter == RHI::TextureFilter::Nearest ? WGPUFilterMode_Nearest : WGPUFilterMode_Linear;
}

WGPUAddressMode ConvertAddressMode(RHI::TextureAddressMode mode)
{
    return mode == RHI::TextureAddressMode::Repeat ? WGPUAddressMode_Repeat : WGPUAddressMode_ClampToEdge;
}

WGPUPrimitiveTopology ConvertPrimitiveTopology(RHI::PrimitiveTopology topology)
{
    switch (topology)
    {
        case RHI::PrimitiveTopology::PointList: return WGPUPrimitiveTopology_PointList;
        case RHI::PrimitiveTopology::LineList: return WGPUPrimitiveTopology_LineList;
        case RHI::PrimitiveTopology::LineStrip: return WGPUPrimitiveTopology_LineStrip;
        case RHI::PrimitiveTopology::TriangleList: return WGPUPrimitiveTopology_TriangleList;
        case RHI::PrimitiveTopology::TriangleStrip: return WGPUPrimitiveTopology_TriangleStrip;
        default: return WGPUPrimitiveTopology_TriangleList;
    }
}

WGPUFrontFace ConvertFrontFace(RHI::FrontFace face)
{
    return face == RHI::FrontFace::CW ? WGPUFrontFace_CW : WGPUFrontFace_CCW;
}

WGPUCullMode ConvertCullMode(RHI::CullMode mode)
{
    switch (mode)
    {
        case RHI::CullMode::None: return WGPUCullMode_None;
        case RHI::CullMode::Front: return WGPUCullMode_Front;
        case RHI::CullMode::Back: return WGPUCullMode_Back;
        default: return WGPUCullMode_None;
    }
}

WGPUCompareFunction ConvertCompareFunction(RHI::CompareOp op)
{
    switch (op)
    {
        case RHI::CompareOp::Never: return WGPUCompareFunction_Never;
        case RHI::CompareOp::Less: return WGPUCompareFunction_Less;
        case RHI::CompareOp::Equal: return WGPUCompareFunction_Equal;
        case RHI::CompareOp::LessOrEqual: return WGPUCompareFunction_LessEqual;
        case RHI::CompareOp::Greater: return WGPUCompareFunction_Greater;
        case RHI::CompareOp::NotEqual: return WGPUCompareFunction_NotEqual;
        case RHI::CompareOp::GreaterOrEqual: return WGPUCompareFunction_GreaterEqual;
        case RHI::CompareOp::Always: return WGPUCompareFunction_Always;
        default: return WGPUCompareFunction_Always;
    }
}

WGPULoadOp ConvertLoadOp(RHI::LoadOp op)
{
    return op == RHI::LoadOp::Clear ? WGPULoadOp_Clear : (op == RHI::LoadOp::Load ? WGPULoadOp_Load : WGPULoadOp_Clear);
}

WGPUStoreOp ConvertStoreOp(RHI::StoreOp op)
{
    return op == RHI::StoreOp::DontCare ? WGPUStoreOp_Discard : WGPUStoreOp_Store;
}

WGPUVertexFormat ConvertVertexFormat(RHI::Format format)
{
    switch (format)
    {
        case RHI::Format::Float32x2: return WGPUVertexFormat_Float32x2;
        case RHI::Format::Float32x3: return WGPUVertexFormat_Float32x3;
        case RHI::Format::Float32x4: return WGPUVertexFormat_Float32x4;
        case RHI::Format::RGBA8Unorm: return WGPUVertexFormat_Float32x4;
        default: return WGPUVertexFormat_Float32x3;
    }
}

WGPUShaderStage ConvertShaderStage(RHI::ShaderStage stage)
{
    WGPUShaderStage flags = WGPUShaderStage_None;
    if ((u32)stage & (u32)RHI::ShaderStage::Vertex) flags |= WGPUShaderStage_Vertex;
    if ((u32)stage & (u32)RHI::ShaderStage::Fragment) flags |= WGPUShaderStage_Fragment;
    if ((u32)stage & (u32)RHI::ShaderStage::Compute) flags |= WGPUShaderStage_Compute;
    return flags;
}

WebGPUDevice::~WebGPUDevice()
{
    for (auto& entry : mBindGroupLayoutCache)
    {
        if (entry.Value)
        {
            wgpuBindGroupLayoutRelease(entry.Value);
        }
    }
    mBindGroupLayoutCache.Clear();
    if (mQueue)
    {
        wgpuQueueRelease(mQueue);
    }
    if (mDevice)
    {
        wgpuDeviceRelease(mDevice);
    }
    if (mAdapter)
    {
        wgpuAdapterRelease(mAdapter);
    }
    if (mSurface)
    {
        wgpuSurfaceRelease(mSurface);
    }
    if (mInstance)
    {
        wgpuInstanceRelease(mInstance);
    }
}

void WebGPUDevice::OnAdapterRequested(WGPURequestAdapterStatus status, WGPUAdapter adapter,
                                      WGPUStringView message, void* userdata1, void* userdata2)
{
    (void)message;
    (void)userdata2;
    WebGPUDevice* device = static_cast<WebGPUDevice*>(userdata1);
    device->mAdapterStatus = status;
    device->mPendingAdapter = adapter;
    device->mAdapterDone = true;
    if (status != WGPURequestAdapterStatus_Success)
    {
        printf("WebGPU adapter request failed\n");
    }
    else
    {
        printf("WebGPU adapter acquired\n");
    }
}

static void OnDeviceLost(WGPUDevice const*, WGPUDeviceLostReason reason, WGPUStringView message,
                         void*, void*)
{
    printf("WebGPU DEVICE LOST (reason %d): %.*s\n", (int)reason,
           (int)message.length, message.data ? message.data : "");
}

static void OnUncapturedError(WGPUDevice const*, WGPUErrorType type, WGPUStringView message,
                              void*, void*)
{
    printf("WebGPU ERROR (type %d): %.*s\n", (int)type,
           (int)message.length, message.data ? message.data : "");
}

void WebGPUDevice::OnDeviceRequested(WGPURequestDeviceStatus status, WGPUDevice device,
                                     WGPUStringView message, void* userdata1, void* userdata2)
{
    (void)userdata2;
    WebGPUDevice* self = static_cast<WebGPUDevice*>(userdata1);
    self->mDeviceStatus = status;
    self->mPendingDevice = device;
    self->mDeviceDone = true;
    if (status != WGPURequestDeviceStatus_Success)
    {
        printf("WebGPU device request failed: %.*s\n", (int)message.length,
               message.data ? message.data : "");
    }
}

bool WebGPUDevice::Init(void* nativeWindowHandle)
{
    (void)nativeWindowHandle;
    mInitState = InitState::CreatingInstance;
    StepInit();
    return !IsFailed();
}

void WebGPUDevice::StepInit()
{
    switch (mInitState)
    {
        case InitState::CreatingInstance:
        {
            WGPUInstanceDescriptor instanceDesc = {};
            instanceDesc.nextInChain = nullptr;
            mInstance = wgpuCreateInstance(&instanceDesc);
            if (!mInstance)
            {
                printf("Failed to create WebGPU instance\n");
                mInitState = InitState::Failed;
                return;
            }
            mInitState = InitState::InstanceCreated;
            [[fallthrough]];
        }
        case InitState::InstanceCreated:
        {
            WGPUEmscriptenSurfaceSourceCanvasHTMLSelector canvasDesc = {};
            canvasDesc.chain.sType = WGPUSType_EmscriptenSurfaceSourceCanvasHTMLSelector;
            canvasDesc.chain.next = nullptr;
            canvasDesc.selector = MakeStringView("#canvas");
            WGPUSurfaceDescriptor surfaceDesc = {};
            surfaceDesc.nextInChain = &canvasDesc.chain;
            mSurface = wgpuInstanceCreateSurface(mInstance, &surfaceDesc);
            if (!mSurface)
            {
                printf("Failed to create WebGPU surface\n");
                mInitState = InitState::Failed;
                return;
            }
            mInitState = InitState::SurfaceCreated;
            [[fallthrough]];
        }
        case InitState::SurfaceCreated:
        {
            WGPURequestAdapterOptions adapterOptions = {};
            adapterOptions.compatibleSurface = mSurface;
            adapterOptions.powerPreference = WGPUPowerPreference_HighPerformance;
            adapterOptions.forceFallbackAdapter = WGPU_FALSE;
            mAdapterDone = false;
            mPendingAdapter = nullptr;
            WGPURequestAdapterCallbackInfo callbackInfo = {};
            callbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
            callbackInfo.callback = &WebGPUDevice::OnAdapterRequested;
            callbackInfo.userdata1 = this;
            wgpuInstanceRequestAdapter(mInstance, &adapterOptions, callbackInfo);
            mInitState = InitState::RequestingAdapter;
            break;
        }
        case InitState::RequestingAdapter:
        {
            if (!mAdapterDone) break;
            if (mAdapterStatus != WGPURequestAdapterStatus_Success || !mPendingAdapter)
            {
                printf("Failed to get WebGPU adapter\n");
                mInitState = InitState::Failed;
                return;
            }
            mAdapter = mPendingAdapter;
            mPendingAdapter = nullptr;
            mInitState = InitState::AdapterReceived;
            [[fallthrough]];
        }
        case InitState::AdapterReceived:
        {
            WGPUDeviceDescriptor deviceDesc = {};
            deviceDesc.label = MakeStringView("AetherDevice");
            deviceDesc.requiredFeatureCount = 0;
            deviceDesc.requiredFeatures = nullptr;
            deviceDesc.requiredLimits = nullptr;
            deviceDesc.defaultQueue.label = MakeStringView("AetherQueue");
            deviceDesc.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
            deviceDesc.deviceLostCallbackInfo.callback = &OnDeviceLost;
            deviceDesc.uncapturedErrorCallbackInfo.callback = &OnUncapturedError;
            mDeviceDone = false;
            mPendingDevice = nullptr;
            WGPURequestDeviceCallbackInfo callbackInfo = {};
            callbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
            callbackInfo.callback = &WebGPUDevice::OnDeviceRequested;
            callbackInfo.userdata1 = this;
            wgpuAdapterRequestDevice(mAdapter, &deviceDesc, callbackInfo);
            mInitState = InitState::RequestingDevice;
            break;
        }
        case InitState::RequestingDevice:
        {
            if (!mDeviceDone) break;
            if (mDeviceStatus != WGPURequestDeviceStatus_Success || !mPendingDevice)
            {
                printf("Failed to get WebGPU device\n");
                mInitState = InitState::Failed;
                return;
            }
            mDevice = mPendingDevice;
            mPendingDevice = nullptr;
            mQueue = wgpuDeviceGetQueue(mDevice);
            if (!mQueue)
            {
                printf("Failed to get WebGPU queue\n");
                mInitState = InitState::Failed;
                return;
            }
            mInitState = InitState::DeviceReceived;
            [[fallthrough]];
        }
        case InitState::DeviceReceived:
        {
            printf("Aether WebGPU device initialized successfully\n");
            mInitState = InitState::Ready;
            break;
        }
        case InitState::Ready:
        case InitState::Failed:
        case InitState::NotStarted:
            break;
    }
}

void WebGPUDevice::WaitIdle()
{
    if (mInstance)
    {
        wgpuInstanceProcessEvents(mInstance);
    }
}

void WebGPUDevice::Tick()
{
    if (mInitState != InitState::Ready && mInitState != InitState::Failed)
    {
        StepInit();
    }
    if (mInstance)
    {
        wgpuInstanceProcessEvents(mInstance);
    }
}

RefPtr<RHI::RHISwapchain> WebGPUDevice::CreateSwapchain(u32 width, u32 height)
{
    if (!IsReady()) return nullptr;

    RefPtr<WebGPUSwapchain> swapchain = MakeRef<WebGPUSwapchain>();
    swapchain->mWidth = width;
    swapchain->mHeight = height;
    swapchain->mSurface = mSurface;
    swapchain->Configure(mDevice);

    mCurrentSwapchain = swapchain;
    return swapchain;
}

RefPtr<RHI::RHIBuffer> WebGPUDevice::CreateBuffer(const RHI::BufferDesc& desc, const void* initialData)
{
    if (!IsReady()) return nullptr;

    RefPtr<WebGPUBuffer> buffer = MakeRef<WebGPUBuffer>();
    buffer->mBufferSize = desc.Size;
    buffer->mUsageFlags = ConvertBufferUsage(desc.Usage);
    buffer->mUsageFlags = (WGPUBufferUsage)((u32)buffer->mUsageFlags | (u32)WGPUBufferUsage_CopyDst);

    // 注意: 不加 MapWrite。WebGPU 规定 MAP_WRITE 只能与 COPY_SRC 共存, 挂上
    // VERTEX/UNIFORM 即校验失败 (缓冲整体无效 -> 黑屏)。engine 侧统一走
    // Queue::WriteBuffer 上传 (只需 CopyDst), HostVisible/DeviceLocal 在本后端无区别。

    WGPUBufferDescriptor bufferDesc = {};
    bufferDesc.label = MakeStringView("Buffer");
    bufferDesc.usage = buffer->mUsageFlags;
    bufferDesc.size = desc.Size;
    bufferDesc.mappedAtCreation = WGPU_FALSE;

    buffer->mBuffer = wgpuDeviceCreateBuffer(mDevice, &bufferDesc);
    if (!buffer->mBuffer)
    {
        printf("Failed to create WebGPU buffer\n");
        return nullptr;
    }

    if (initialData)
    {
        wgpuQueueWriteBuffer(mQueue, buffer->mBuffer, 0, initialData, desc.Size);
    }

    return buffer;
}

RefPtr<RHI::RHIShader> WebGPUDevice::CreateShader(RHI::ShaderStage stage, const RHI::ShaderModuleDesc& desc)
{
    if (!IsReady()) return nullptr;

    RefPtr<WebGPUShader> shader = MakeRef<WebGPUShader>();
    shader->mStage = stage;
    shader->mEntryPoint = desc.EntryPoint ? desc.EntryPoint : "main";

    WGPUShaderSourceWGSL wgslDesc = {};
    wgslDesc.chain.next = nullptr;
    wgslDesc.chain.sType = WGPUSType_ShaderSourceWGSL;
    wgslDesc.code = MakeStringView(static_cast<const char*>(desc.Code));

    WGPUShaderModuleDescriptor moduleDesc = {};
    moduleDesc.nextInChain = &wgslDesc.chain;
    moduleDesc.label = MakeStringView("ShaderModule");

    shader->mModule = wgpuDeviceCreateShaderModule(mDevice, &moduleDesc);
    if (!shader->mModule)
    {
        printf("Failed to create WebGPU shader module\n");
        return nullptr;
    }

    return shader;
}

RefPtr<RHI::RHIRenderPipeline> WebGPUDevice::CreateRenderPipeline(const RHI::RenderPipelineDesc& desc)
{
    if (!IsReady()) return nullptr;

    const WebGPUShader* vs = static_cast<const WebGPUShader*>(desc.VertexShader);
    const WebGPUShader* fs = static_cast<const WebGPUShader*>(desc.FragmentShader);

    RefPtr<WebGPURenderPipeline> pipeline = MakeRef<WebGPURenderPipeline>();

    Array<WGPUVertexAttribute> attributes;
    WGPUVertexBufferLayout vertexBufferLayout = {};
    if (desc.VertexLayout.Attributes && desc.VertexLayout.AttributeCount > 0)
    {
        vertexBufferLayout.arrayStride = desc.VertexLayout.Stride;
        vertexBufferLayout.stepMode = WGPUVertexStepMode_Vertex;
        attributes.Resize(desc.VertexLayout.AttributeCount);
        for (u32 i = 0; i < desc.VertexLayout.AttributeCount; i++)
        {
            attributes[i].shaderLocation = desc.VertexLayout.Attributes[i].Location;
            attributes[i].offset = desc.VertexLayout.Attributes[i].Offset;
            attributes[i].format = ConvertVertexFormat(desc.VertexLayout.Attributes[i].Format);
        }
        vertexBufferLayout.attributeCount = attributes.Count();
        vertexBufferLayout.attributes = attributes.Data();
    }

    WGPUVertexState vertexState = {};
    vertexState.module = vs->mModule;
    vertexState.entryPoint = MakeStringView(vs->mEntryPoint.CStr());
    vertexState.bufferCount = (vertexBufferLayout.arrayStride > 0) ? 1 : 0;
    vertexState.buffers = &vertexBufferLayout;
    vertexState.constantCount = 0;
    vertexState.constants = nullptr;

    Array<WGPUColorTargetState> colorTargets;
    Array<WGPUBlendState> blendStates;
    WGPUFragmentState fragmentState = {};
    if (fs && desc.ColorTargetCount > 0)
    {
        fragmentState.module = fs->mModule;
        fragmentState.entryPoint = MakeStringView(fs->mEntryPoint.CStr());
        colorTargets.Resize(desc.ColorTargetCount);
        blendStates.Resize(desc.ColorTargetCount);
        for (u32 i = 0; i < desc.ColorTargetCount; i++)
        {
            colorTargets[i].format = ConvertFormat(desc.ColorTargets[i].Format);
            colorTargets[i].writeMask = WGPUColorWriteMask_All;
            if (desc.BlendMode == RHI::BlendMode::Additive)
            {
                blendStates[i].color.operation = WGPUBlendOperation_Add;
                blendStates[i].color.srcFactor = WGPUBlendFactor_SrcAlpha;
                blendStates[i].color.dstFactor = WGPUBlendFactor_One;
                blendStates[i].alpha.operation = WGPUBlendOperation_Add;
                blendStates[i].alpha.srcFactor = WGPUBlendFactor_One;
                blendStates[i].alpha.dstFactor = WGPUBlendFactor_One;
            }
            else
            {
                blendStates[i].color.operation = WGPUBlendOperation_Add;
                blendStates[i].color.srcFactor = WGPUBlendFactor_SrcAlpha;
                blendStates[i].color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
                blendStates[i].alpha.operation = WGPUBlendOperation_Add;
                blendStates[i].alpha.srcFactor = WGPUBlendFactor_One;
                blendStates[i].alpha.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
            }
            colorTargets[i].blend = &blendStates[i];
        }
        fragmentState.targetCount = colorTargets.Count();
        fragmentState.targets = colorTargets.Data();
        fragmentState.constantCount = 0;
        fragmentState.constants = nullptr;
    }

    WGPUPrimitiveState primitiveState = {};
    primitiveState.topology = ConvertPrimitiveTopology(desc.PrimitiveTopology);
    primitiveState.stripIndexFormat = WGPUIndexFormat_Undefined;
    primitiveState.frontFace = ConvertFrontFace(desc.FrontFace);
    primitiveState.cullMode = ConvertCullMode(desc.CullMode);
    primitiveState.unclippedDepth = WGPU_FALSE;

    WGPUDepthStencilState depthStencil = {};
    const WGPUDepthStencilState* depthStencilPtr = nullptr;
    if (desc.DepthStencil)
    {
        depthStencil.format = ConvertFormat(desc.DepthStencil->Format);
        depthStencil.depthWriteEnabled = desc.DepthStencil->DepthWriteEnabled ? WGPUOptionalBool_True : WGPUOptionalBool_False;
        depthStencil.depthCompare = ConvertCompareFunction(desc.DepthStencil->DepthCompare);
    }
    else
    {
        // WebGPU 校验要求: 挂了深度附件的 render pass 中所有管线都必须声明
        // depthStencilState。2D 精灵管线等效为 "不写深度 + 恒通过"。
        depthStencil.format = WGPUTextureFormat_Depth32Float;
        depthStencil.depthWriteEnabled = WGPUOptionalBool_False;
        depthStencil.depthCompare = WGPUCompareFunction_Always;
    }
    {
        depthStencil.stencilFront.compare = WGPUCompareFunction_Always;
        depthStencil.stencilBack.compare = WGPUCompareFunction_Always;
        depthStencil.stencilFront.failOp = WGPUStencilOperation_Keep;
        depthStencil.stencilFront.depthFailOp = WGPUStencilOperation_Keep;
        depthStencil.stencilFront.passOp = WGPUStencilOperation_Keep;
        depthStencil.stencilBack.failOp = WGPUStencilOperation_Keep;
        depthStencil.stencilBack.depthFailOp = WGPUStencilOperation_Keep;
        depthStencil.stencilBack.passOp = WGPUStencilOperation_Keep;
        depthStencil.depthBias = 0;
        depthStencil.depthBiasSlopeScale = 0.0f;
        depthStencil.depthBiasClamp = 0.0f;
        depthStencil.stencilReadMask = 0xFFFFFFFF;
        depthStencil.stencilWriteMask = 0xFFFFFFFF;
        depthStencilPtr = &depthStencil;
    }

    WGPUMultisampleState multisample = {};
    multisample.count = 1;
    multisample.mask = 0xFFFFFFFF;
    multisample.alphaToCoverageEnabled = WGPU_FALSE;

    Array<WGPUBindGroupLayout> bindGroupLayouts;
    bindGroupLayouts.Reserve(desc.BindGroupLayouts.Count());
    for (const RHI::BindGroupLayoutDesc& layoutDesc : desc.BindGroupLayouts)
    {
        bindGroupLayouts.Add(GetBindGroupLayout(layoutDesc));
    }

    WGPUPipelineLayoutDescriptor layoutDesc = {};
    layoutDesc.bindGroupLayoutCount = bindGroupLayouts.Count();
    layoutDesc.bindGroupLayouts = bindGroupLayouts.Data();
    layoutDesc.label = MakeStringView("PipelineLayout");
    WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(mDevice, &layoutDesc);

    WGPURenderPipelineDescriptor pipelineDesc = {};
    pipelineDesc.label = MakeStringView("RenderPipeline");
    pipelineDesc.layout = pipelineLayout;
    pipelineDesc.vertex = vertexState;
    pipelineDesc.primitive = primitiveState;
    pipelineDesc.multisample = multisample;
    pipelineDesc.depthStencil = depthStencilPtr;
    pipelineDesc.fragment = (fs) ? &fragmentState : nullptr;

    pipeline->mPipeline = wgpuDeviceCreateRenderPipeline(mDevice, &pipelineDesc);
    wgpuPipelineLayoutRelease(pipelineLayout);

    if (!pipeline->mPipeline)
    {
        printf("Failed to create WebGPU render pipeline\n");
        return nullptr;
    }

    return pipeline;
}

UniquePtr<RHI::RHICommandEncoder> WebGPUDevice::CreateCommandEncoder()
{
    if (!IsReady()) return nullptr;
    return UniquePtr<RHI::RHICommandEncoder>(MakeUnique<WebGPUCommandEncoder>(mDevice, mQueue).Release());
}

WGPUBindGroupLayout WebGPUDevice::GetBindGroupLayout(const RHI::BindGroupLayoutDesc& layoutDesc)
{
    u64 key = 0;
    for (const RHI::BindGroupLayoutEntry& layoutEntry : layoutDesc.Entries)
    {
        u64 h = (u64)layoutEntry.Binding;
        h |= ((u64)layoutEntry.Kind) << 32;
        key ^= h + 0x9e3779b97f4a7c15ull + (key << 6) + (key >> 2);
    }
    if (WGPUBindGroupLayout* cached = mBindGroupLayoutCache.Find(key))
    {
        return *cached;
    }

    Array<WGPUBindGroupLayoutEntry> entries;
    entries.Reserve(layoutDesc.Entries.Count());
    for (const RHI::BindGroupLayoutEntry& layoutEntry : layoutDesc.Entries)
    {
        WGPUBindGroupLayoutEntry entry = {};
        entry.binding = layoutEntry.Binding;
        entry.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
        if (layoutEntry.Kind == RHI::BindGroupEntryKind::Uniform)
        {
            entry.buffer.type = WGPUBufferBindingType_Uniform;
            entry.buffer.hasDynamicOffset = WGPU_FALSE;
            entry.buffer.minBindingSize = 0;
        }
        else if (layoutEntry.Kind == RHI::BindGroupEntryKind::Texture)
        {
            entry.texture.sampleType = WGPUTextureSampleType_Float;
            entry.texture.viewDimension = WGPUTextureViewDimension_2D;
            entry.texture.multisampled = WGPU_FALSE;
        }
        else
        {
            entry.sampler.type = WGPUSamplerBindingType_Filtering;
        }
        entries.Add(entry);
    }

    WGPUBindGroupLayoutDescriptor descriptor = {};
    descriptor.label = MakeStringView("BindGroupLayout");
    descriptor.entryCount = entries.Count();
    descriptor.entries = entries.Data();

    WGPUBindGroupLayout layout = wgpuDeviceCreateBindGroupLayout(mDevice, &descriptor);
    if (!layout)
    {
        printf("Failed to create WebGPU bind group layout\n");
        return nullptr;
    }
    mBindGroupLayoutCache.Add(key, layout);
    return layout;
}

RefPtr<RHI::RHITexture> WebGPUDevice::CreateTexture(const RHI::TextureDesc& desc)
{
    if (!IsReady()) return nullptr;

    RefPtr<WebGPUTexture> texture = MakeRef<WebGPUTexture>();
    texture->mWidth = desc.Width;
    texture->mHeight = desc.Height;

    WGPUTextureDescriptor textureDesc = {};
    textureDesc.label = MakeStringView("Texture");
    textureDesc.usage = ConvertTextureUsage(desc.Usage) | WGPUTextureUsage_TextureBinding;
    textureDesc.dimension = WGPUTextureDimension_2D;
    textureDesc.size = {desc.Width, desc.Height, 1};
    textureDesc.format = ConvertTextureFormat(desc.Format);
    textureDesc.mipLevelCount = 1;
    textureDesc.sampleCount = 1;

    texture->mTexture = wgpuDeviceCreateTexture(mDevice, &textureDesc);
    if (!texture->mTexture)
    {
        printf("Failed to create WebGPU texture\n");
        return nullptr;
    }

    WGPUTextureViewDescriptor viewDesc = {};
    viewDesc.label = MakeStringView("TextureView");
    viewDesc.format = ConvertTextureFormat(desc.Format);
    viewDesc.dimension = WGPUTextureViewDimension_2D;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;

    RefPtr<WebGPUTextureView> view = MakeRef<WebGPUTextureView>();
    view->mView = wgpuTextureCreateView(texture->mTexture, &viewDesc);
    texture->mTextureView = view;

    if (desc.InitialData)
    {
        UpdateTexture(texture.Get(), desc.InitialData);
    }

    return texture;
}

RefPtr<RHI::RHISampler> WebGPUDevice::CreateSampler(const RHI::SamplerDesc& desc)
{
    if (!IsReady()) return nullptr;

    RefPtr<WebGPUSampler> sampler = MakeRef<WebGPUSampler>();
    WGPUSamplerDescriptor samplerDesc = {};
    samplerDesc.label = MakeStringView("Sampler");
    samplerDesc.addressModeU = ConvertAddressMode(desc.AddressModeU);
    samplerDesc.addressModeV = ConvertAddressMode(desc.AddressModeV);
    samplerDesc.addressModeW = WGPUAddressMode_ClampToEdge;
    samplerDesc.magFilter = ConvertTextureFilter(desc.MagFilter);
    samplerDesc.minFilter = ConvertTextureFilter(desc.MinFilter);
    samplerDesc.mipmapFilter = WGPUMipmapFilterMode_Linear;
    samplerDesc.lodMinClamp = 0.0f;
    samplerDesc.lodMaxClamp = 32.0f;
    samplerDesc.compare = WGPUCompareFunction_Undefined;
    samplerDesc.maxAnisotropy = 1;

    sampler->mSampler = wgpuDeviceCreateSampler(mDevice, &samplerDesc);
    if (!sampler->mSampler)
    {
        printf("Failed to create WebGPU sampler\n");
        return nullptr;
    }
    return sampler;
}

RefPtr<RHI::RHIBindGroup> WebGPUDevice::CreateBindGroup(const RHI::BindGroupLayoutDesc& layoutDesc,
                                                        const Array<RHI::BindGroupEntry>& entries)
{
    if (!IsReady()) return nullptr;

    WGPUBindGroupLayout layout = GetBindGroupLayout(layoutDesc);
    if (!layout) return nullptr;

    Array<WGPUBindGroupEntry> wgpuEntries;
    wgpuEntries.Reserve(entries.Count());
    for (const RHI::BindGroupEntry& entry : entries)
    {
        WGPUBindGroupEntry wgpuEntry = {};
        wgpuEntry.binding = entry.Binding;
        if (entry.Buffer)
        {
            wgpuEntry.buffer = static_cast<WebGPUBuffer*>(entry.Buffer)->mBuffer;
            wgpuEntry.offset = entry.Offset;
            wgpuEntry.size = entry.Size > 0 ? entry.Size : WGPU_WHOLE_SIZE;
        }
        else if (entry.TextureView)
        {
            wgpuEntry.textureView = static_cast<WebGPUTextureView*>(entry.TextureView)->mView;
        }
        else if (entry.Sampler)
        {
            wgpuEntry.sampler = static_cast<WebGPUSampler*>(entry.Sampler)->mSampler;
        }
        wgpuEntries.Add(wgpuEntry);
    }

    RefPtr<WebGPUBindGroup> bindGroup = MakeRef<WebGPUBindGroup>();
    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.label = MakeStringView("BindGroup");
    bindGroupDesc.layout = layout;
    bindGroupDesc.entryCount = wgpuEntries.Count();
    bindGroupDesc.entries = wgpuEntries.Data();

    bindGroup->mBindGroup = wgpuDeviceCreateBindGroup(mDevice, &bindGroupDesc);
    if (!bindGroup->mBindGroup)
    {
        printf("Failed to create WebGPU bind group\n");
        return nullptr;
    }
    return bindGroup;
}

void WebGPUDevice::UpdateBuffer(RHI::RHIBuffer* buffer, u64 offset, const void* data, u64 size)
{
    if (!buffer || !data || !size || !mQueue) return;
    WebGPUBuffer* gpuBuffer = static_cast<WebGPUBuffer*>(buffer);
    wgpuQueueWriteBuffer(mQueue, gpuBuffer->mBuffer, offset, data, size);
}

void WebGPUDevice::UpdateTexture(RHI::RHITexture* texture, const void* data)
{
    if (!texture || !data || !mQueue) return;
    WebGPUTexture* gpuTexture = static_cast<WebGPUTexture*>(texture);

    WGPUTexelCopyTextureInfo dest = {};
    dest.texture = gpuTexture->mTexture;
    dest.mipLevel = 0;
    dest.origin = {0, 0, 0};
    dest.aspect = WGPUTextureAspect_All;

    WGPUTexelCopyBufferLayout layout = {};
    layout.offset = 0;
    layout.bytesPerRow = gpuTexture->mWidth * 4;
    layout.rowsPerImage = gpuTexture->mHeight;

    WGPUExtent3D size = {gpuTexture->mWidth, gpuTexture->mHeight, 1};
    wgpuQueueWriteTexture(mQueue, &dest, data, (size_t)gpuTexture->mWidth * gpuTexture->mHeight * 4, &layout, &size);
}

}

namespace Aether::RHI {

UniquePtr<RHIDevice> CreateWebGPUDevice()
{
    return UniquePtr<RHIDevice>(new WebGPU::WebGPUDevice());
}

}
