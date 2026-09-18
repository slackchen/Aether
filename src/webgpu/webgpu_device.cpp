#include "webgpu/webgpu_internal.h"
#include <cstdio>
#include <cstring>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace aether::webgpu {

WGPUBufferUsage convert_buffer_usage(u32 usage) {
    WGPUBufferUsage flags = WGPUBufferUsage_None;
    if (usage & (u32)rhi::BufferUsage::Vertex) flags |= WGPUBufferUsage_Vertex;
    if (usage & (u32)rhi::BufferUsage::Index) flags |= WGPUBufferUsage_Index;
    if (usage & (u32)rhi::BufferUsage::Uniform) flags |= WGPUBufferUsage_Uniform;
    if (usage & (u32)rhi::BufferUsage::Storage) flags |= WGPUBufferUsage_Storage;
    if (usage & (u32)rhi::BufferUsage::CopySrc) flags |= WGPUBufferUsage_CopySrc;
    if (usage & (u32)rhi::BufferUsage::CopyDst) flags |= WGPUBufferUsage_CopyDst;
    return flags;
}

WGPUTextureFormat convert_format(rhi::Format format) {
    switch (format) {
        case rhi::Format::RGBA8Unorm: return WGPUTextureFormat_RGBA8Unorm;
        case rhi::Format::BGRA8Unorm: return WGPUTextureFormat_BGRA8Unorm;
        case rhi::Format::Depth32Float: return WGPUTextureFormat_Depth32Float;
        case rhi::Format::Depth24PlusStencil8: return WGPUTextureFormat_Depth24PlusStencil8;
        default: return WGPUTextureFormat_Undefined;
    }
}

WGPUTextureUsage convert_texture_usage(u32 usage) {
    WGPUTextureUsage flags = WGPUTextureUsage_None;
    if (usage & (u32)rhi::TextureUsage::Sampled) flags |= WGPUTextureUsage_TextureBinding;
    if (usage & (u32)rhi::TextureUsage::RenderAttachment) flags |= WGPUTextureUsage_RenderAttachment;
    if (usage & (u32)rhi::TextureUsage::CopyDst) flags |= WGPUTextureUsage_CopyDst;
    if (usage & (u32)rhi::TextureUsage::CopySrc) flags |= WGPUTextureUsage_CopySrc;
    return flags;
}

WGPUTextureFormat convert_texture_format(rhi::TextureFormat format) {
    switch (format) {
        case rhi::TextureFormat::RGBA8Unorm: return WGPUTextureFormat_RGBA8Unorm;
        default: return WGPUTextureFormat_Undefined;
    }
}

WGPUFilterMode convert_texture_filter(rhi::TextureFilter filter) {
    return filter == rhi::TextureFilter::Nearest ? WGPUFilterMode_Nearest : WGPUFilterMode_Linear;
}

WGPUAddressMode convert_address_mode(rhi::TextureAddressMode mode) {
    return mode == rhi::TextureAddressMode::Repeat ? WGPUAddressMode_Repeat : WGPUAddressMode_ClampToEdge;
}

WGPUPrimitiveTopology convert_primitive_topology(rhi::PrimitiveTopology topology) {
    switch (topology) {
        case rhi::PrimitiveTopology::PointList: return WGPUPrimitiveTopology_PointList;
        case rhi::PrimitiveTopology::LineList: return WGPUPrimitiveTopology_LineList;
        case rhi::PrimitiveTopology::LineStrip: return WGPUPrimitiveTopology_LineStrip;
        case rhi::PrimitiveTopology::TriangleList: return WGPUPrimitiveTopology_TriangleList;
        case rhi::PrimitiveTopology::TriangleStrip: return WGPUPrimitiveTopology_TriangleStrip;
        default: return WGPUPrimitiveTopology_TriangleList;
    }
}

WGPUFrontFace convert_front_face(rhi::FrontFace face) {
    return face == rhi::FrontFace::CW ? WGPUFrontFace_CW : WGPUFrontFace_CCW;
}

WGPUCullMode convert_cull_mode(rhi::CullMode mode) {
    switch (mode) {
        case rhi::CullMode::None: return WGPUCullMode_None;
        case rhi::CullMode::Front: return WGPUCullMode_Front;
        case rhi::CullMode::Back: return WGPUCullMode_Back;
        default: return WGPUCullMode_None;
    }
}

WGPUCompareFunction convert_compare_function(rhi::CompareOp op) {
    switch (op) {
        case rhi::CompareOp::Never: return WGPUCompareFunction_Never;
        case rhi::CompareOp::Less: return WGPUCompareFunction_Less;
        case rhi::CompareOp::Equal: return WGPUCompareFunction_Equal;
        case rhi::CompareOp::LessOrEqual: return WGPUCompareFunction_LessEqual;
        case rhi::CompareOp::Greater: return WGPUCompareFunction_Greater;
        case rhi::CompareOp::NotEqual: return WGPUCompareFunction_NotEqual;
        case rhi::CompareOp::GreaterOrEqual: return WGPUCompareFunction_GreaterEqual;
        case rhi::CompareOp::Always: return WGPUCompareFunction_Always;
        default: return WGPUCompareFunction_Always;
    }
}

WGPULoadOp convert_load_op(rhi::LoadOp op) {
    return op == rhi::LoadOp::Clear ? WGPULoadOp_Clear : (op == rhi::LoadOp::Load ? WGPULoadOp_Load : WGPULoadOp_Clear);
}

WGPUStoreOp convert_store_op(rhi::StoreOp op) {
    return op == rhi::StoreOp::DontCare ? WGPUStoreOp_Discard : WGPUStoreOp_Store;
}

WGPUVertexFormat convert_vertex_format(rhi::Format format) {
    switch (format) {
        case rhi::Format::Float32x2: return WGPUVertexFormat_Float32x2;
        case rhi::Format::Float32x3: return WGPUVertexFormat_Float32x3;
        case rhi::Format::Float32x4: return WGPUVertexFormat_Float32x4;
        case rhi::Format::RGBA8Unorm: return WGPUVertexFormat_Float32x4;
        default: return WGPUVertexFormat_Float32x3;
    }
}

WGPUShaderStage convert_shader_stage(rhi::ShaderStage stage) {
    WGPUShaderStage flags = WGPUShaderStage_None;
    if ((u32)stage & (u32)rhi::ShaderStage::Vertex) flags |= WGPUShaderStage_Vertex;
    if ((u32)stage & (u32)rhi::ShaderStage::Fragment) flags |= WGPUShaderStage_Fragment;
    if ((u32)stage & (u32)rhi::ShaderStage::Compute) flags |= WGPUShaderStage_Compute;
    return flags;
}

WebGPUDevice::~WebGPUDevice() {
    for (auto& [key, layout] : bind_group_layout_cache) {
        if (layout) wgpuBindGroupLayoutRelease(layout);
    }
    bind_group_layout_cache.clear();
    if (queue) wgpuQueueRelease(queue);
    if (device) wgpuDeviceRelease(device);
    if (adapter) wgpuAdapterRelease(adapter);
    if (surface) wgpuSurfaceRelease(surface);
    if (instance) wgpuInstanceRelease(instance);
}

void WebGPUDevice::on_adapter_requested(WGPURequestAdapterStatus status, WGPUAdapter adapter,
                                        WGPUStringView message, void* userdata1, void* userdata2) {
    (void)message; (void)userdata2;
    auto* dev = static_cast<WebGPUDevice*>(userdata1);
    dev->adapter_status = status;
    dev->pending_adapter = adapter;
    dev->adapter_done = true;
    if (status != WGPURequestAdapterStatus_Success) {
        printf("WebGPU adapter request failed\n");
    } else {
        printf("WebGPU adapter acquired\n");
    }
}

static void on_device_lost(WGPUDevice const*, WGPUDeviceLostReason reason, WGPUStringView message,
                           void*, void*) {
    printf("WebGPU DEVICE LOST (reason %d): %.*s\n", (int)reason,
           (int)message.length, message.data ? message.data : "");
}

static void on_uncaptured_error(WGPUDevice const*, WGPUErrorType type, WGPUStringView message,
                                void*, void*) {
    printf("WebGPU ERROR (type %d): %.*s\n", (int)type,
           (int)message.length, message.data ? message.data : "");
}

void WebGPUDevice::on_device_requested(WGPURequestDeviceStatus status, WGPUDevice device,
                                       WGPUStringView message, void* userdata1, void* userdata2) {
    (void)message; (void)userdata2;
    auto* dev = static_cast<WebGPUDevice*>(userdata1);
    dev->device_status = status;
    dev->pending_device = device;
    dev->device_done = true;
    if (status != WGPURequestDeviceStatus_Success) {
        printf("WebGPU device request failed: %.*s\n", (int)message.length,
               message.data ? message.data : "");
    }
}

bool WebGPUDevice::init(void* native_window_handle) {
    (void)native_window_handle;
    init_state = InitState::CreatingInstance;
    step_init();
    return !is_failed();
}

void WebGPUDevice::step_init() {
    switch (init_state) {
        case InitState::CreatingInstance: {
            WGPUInstanceDescriptor inst_desc = {};
            inst_desc.nextInChain = nullptr;
            instance = wgpuCreateInstance(&inst_desc);
            if (!instance) {
                printf("Failed to create WebGPU instance\n");
                init_state = InitState::Failed;
                return;
            }
            init_state = InitState::InstanceCreated;
            [[fallthrough]];
        }
        case InitState::InstanceCreated: {
            WGPUEmscriptenSurfaceSourceCanvasHTMLSelector canvas_desc = {};
            canvas_desc.chain.sType = WGPUSType_EmscriptenSurfaceSourceCanvasHTMLSelector;
            canvas_desc.chain.next = nullptr;
            canvas_desc.selector = make_string_view("#canvas");
            WGPUSurfaceDescriptor surf_desc = {};
            surf_desc.nextInChain = &canvas_desc.chain;
            surface = wgpuInstanceCreateSurface(instance, &surf_desc);
            if (!surface) {
                printf("Failed to create WebGPU surface\n");
                init_state = InitState::Failed;
                return;
            }
            init_state = InitState::SurfaceCreated;
            [[fallthrough]];
        }
        case InitState::SurfaceCreated: {
            WGPURequestAdapterOptions adapter_opts = {};
            adapter_opts.compatibleSurface = surface;
            adapter_opts.powerPreference = WGPUPowerPreference_HighPerformance;
            adapter_opts.forceFallbackAdapter = WGPU_FALSE;
            adapter_done = false;
            pending_adapter = nullptr;
            WGPURequestAdapterCallbackInfo cb_info = {};
            cb_info.mode = WGPUCallbackMode_AllowProcessEvents;
            cb_info.callback = &WebGPUDevice::on_adapter_requested;
            cb_info.userdata1 = this;
            wgpuInstanceRequestAdapter(instance, &adapter_opts, cb_info);
            init_state = InitState::RequestingAdapter;
            break;
        }
        case InitState::RequestingAdapter: {
            if (!adapter_done) break;
            if (adapter_status != WGPURequestAdapterStatus_Success || !pending_adapter) {
                printf("Failed to get WebGPU adapter\n");
                init_state = InitState::Failed;
                return;
            }
            adapter = pending_adapter;
            pending_adapter = nullptr;
            init_state = InitState::AdapterReceived;
            [[fallthrough]];
        }
        case InitState::AdapterReceived: {
            WGPUDeviceDescriptor dev_desc = {};
            dev_desc.label = make_string_view("AetherDevice");
            dev_desc.requiredFeatureCount = 0;
            dev_desc.requiredFeatures = nullptr;
            dev_desc.requiredLimits = nullptr;
            dev_desc.defaultQueue.label = make_string_view("AetherQueue");
            dev_desc.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
            dev_desc.deviceLostCallbackInfo.callback = &on_device_lost;
            dev_desc.uncapturedErrorCallbackInfo.callback = &on_uncaptured_error;
            device_done = false;
            pending_device = nullptr;
            WGPURequestDeviceCallbackInfo cb_info = {};
            cb_info.mode = WGPUCallbackMode_AllowProcessEvents;
            cb_info.callback = &WebGPUDevice::on_device_requested;
            cb_info.userdata1 = this;
            wgpuAdapterRequestDevice(adapter, &dev_desc, cb_info);
            init_state = InitState::RequestingDevice;
            break;
        }
        case InitState::RequestingDevice: {
            if (!device_done) break;
            if (device_status != WGPURequestDeviceStatus_Success || !pending_device) {
                printf("Failed to get WebGPU device\n");
                init_state = InitState::Failed;
                return;
            }
            device = pending_device;
            pending_device = nullptr;
            queue = wgpuDeviceGetQueue(device);
            if (!queue) {
                printf("Failed to get WebGPU queue\n");
                init_state = InitState::Failed;
                return;
            }
            init_state = InitState::DeviceReceived;
            [[fallthrough]];
        }
        case InitState::DeviceReceived: {
            printf("Aether WebGPU device initialized successfully\n");
            init_state = InitState::Ready;
            break;
        }
        case InitState::Ready:
        case InitState::Failed:
        case InitState::NotStarted:
            break;
    }
}

void WebGPUDevice::wait_idle() {
    if (instance) {
        wgpuInstanceProcessEvents(instance);
    }
}

void WebGPUDevice::tick() {
    if (init_state != InitState::Ready && init_state != InitState::Failed) {
        step_init();
    }
    if (instance) {
        wgpuInstanceProcessEvents(instance);
    }
}

std::shared_ptr<rhi::RHISwapchain> WebGPUDevice::create_swapchain(u32 width, u32 height) {
    if (!is_ready()) return nullptr;

    auto sc = std::make_shared<WebGPUSwapchain>();
    sc->w = width;
    sc->h = height;
    sc->surface = surface;
    sc->configure(device);

    current_swapchain = sc;
    return std::static_pointer_cast<rhi::RHISwapchain>(sc);
}

std::shared_ptr<rhi::RHIBuffer> WebGPUDevice::create_buffer(const rhi::BufferDesc& desc, const void* initial_data) {
    if (!is_ready()) return nullptr;

    auto buf = std::make_shared<WebGPUBuffer>();
    buf->buffer_size = desc.size;
    buf->usage_flags = convert_buffer_usage(desc.usage);
    buf->usage_flags = (WGPUBufferUsage)((u32)buf->usage_flags | (u32)WGPUBufferUsage_CopyDst);

    // 注意: 不加 MapWrite。WebGPU 规定 MAP_WRITE 只能与 COPY_SRC 共存, 挂上
    // VERTEX/UNIFORM 即校验失败 (缓冲整体无效 -> 黑屏)。engine 侧统一走
    // Queue::WriteBuffer 上传 (只需 CopyDst), HostVisible/DeviceLocal 在本后端无区别。

    WGPUBufferDescriptor buf_desc = {};
    buf_desc.label = make_string_view("Buffer");
    buf_desc.usage = buf->usage_flags;
    buf_desc.size = desc.size;
    buf_desc.mappedAtCreation = WGPU_FALSE;

    buf->buffer = wgpuDeviceCreateBuffer(device, &buf_desc);
    if (!buf->buffer) {
        printf("Failed to create WebGPU buffer\n");
        return nullptr;
    }

    if (initial_data) {
        wgpuQueueWriteBuffer(queue, buf->buffer, 0, initial_data, desc.size);
    }

    return std::static_pointer_cast<rhi::RHIBuffer>(buf);
}

std::shared_ptr<rhi::RHIShader> WebGPUDevice::create_shader(rhi::ShaderStage stage, const rhi::ShaderModuleDesc& desc) {
    if (!is_ready()) return nullptr;

    auto shader = std::make_shared<WebGPUShader>();
    shader->stage = stage;
    shader->entry_point = desc.entry_point ? desc.entry_point : "main";

    WGPUShaderSourceWGSL wgsl_desc = {};
    wgsl_desc.chain.next = nullptr;
    wgsl_desc.chain.sType = WGPUSType_ShaderSourceWGSL;
    wgsl_desc.code = make_string_view(static_cast<const char*>(desc.code));

    WGPUShaderModuleDescriptor mod_desc = {};
    mod_desc.nextInChain = &wgsl_desc.chain;
    mod_desc.label = make_string_view("ShaderModule");

    shader->module = wgpuDeviceCreateShaderModule(device, &mod_desc);
    if (!shader->module) {
        printf("Failed to create WebGPU shader module\n");
        return nullptr;
    }

    return std::static_pointer_cast<rhi::RHIShader>(shader);
}

std::shared_ptr<rhi::RHIRenderPipeline> WebGPUDevice::create_render_pipeline(const rhi::RenderPipelineDesc& desc) {
    if (!is_ready()) return nullptr;

    auto* vs = static_cast<const WebGPUShader*>(desc.vertex_shader);
    auto* fs = static_cast<const WebGPUShader*>(desc.fragment_shader);

    auto pipeline = std::make_shared<WebGPURenderPipeline>();

    std::vector<WGPUVertexAttribute> attributes;
    WGPUVertexBufferLayout vb_layout = {};
    if (desc.vertex_layout.attributes && desc.vertex_layout.attribute_count > 0) {
        vb_layout.arrayStride = desc.vertex_layout.stride;
        vb_layout.stepMode = WGPUVertexStepMode_Vertex;
        attributes.resize(desc.vertex_layout.attribute_count);
        for (u32 i = 0; i < desc.vertex_layout.attribute_count; i++) {
            attributes[i].shaderLocation = desc.vertex_layout.attributes[i].location;
            attributes[i].offset = desc.vertex_layout.attributes[i].offset;
            attributes[i].format = convert_vertex_format(desc.vertex_layout.attributes[i].format);
        }
        vb_layout.attributeCount = attributes.size();
        vb_layout.attributes = attributes.data();
    }

    WGPUVertexState vertex_state = {};
    vertex_state.module = vs->module;
    vertex_state.entryPoint = make_string_view(vs->entry_point.c_str());
    vertex_state.bufferCount = (vb_layout.arrayStride > 0) ? 1 : 0;
    vertex_state.buffers = &vb_layout;
    vertex_state.constantCount = 0;
    vertex_state.constants = nullptr;

    std::vector<WGPUColorTargetState> color_targets;
    std::vector<WGPUBlendState> blend_states;
    WGPUFragmentState fragment_state = {};
    if (fs && desc.color_target_count > 0) {
        fragment_state.module = fs->module;
        fragment_state.entryPoint = make_string_view(fs->entry_point.c_str());
        color_targets.resize(desc.color_target_count);
        blend_states.resize(desc.color_target_count);
        for (u32 i = 0; i < desc.color_target_count; i++) {
            color_targets[i].format = convert_format(desc.color_targets[i].format);
            color_targets[i].writeMask = WGPUColorWriteMask_All;
            if (desc.blend_mode == rhi::BlendMode::Additive) {
                blend_states[i].color.operation = WGPUBlendOperation_Add;
                blend_states[i].color.srcFactor = WGPUBlendFactor_SrcAlpha;
                blend_states[i].color.dstFactor = WGPUBlendFactor_One;
                blend_states[i].alpha.operation = WGPUBlendOperation_Add;
                blend_states[i].alpha.srcFactor = WGPUBlendFactor_One;
                blend_states[i].alpha.dstFactor = WGPUBlendFactor_One;
            } else {
                blend_states[i].color.operation = WGPUBlendOperation_Add;
                blend_states[i].color.srcFactor = WGPUBlendFactor_SrcAlpha;
                blend_states[i].color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
                blend_states[i].alpha.operation = WGPUBlendOperation_Add;
                blend_states[i].alpha.srcFactor = WGPUBlendFactor_One;
                blend_states[i].alpha.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
            }
            color_targets[i].blend = &blend_states[i];
        }
        fragment_state.targetCount = color_targets.size();
        fragment_state.targets = color_targets.data();
        fragment_state.constantCount = 0;
        fragment_state.constants = nullptr;
    }

    WGPUPrimitiveState primitive_state = {};
    primitive_state.topology = convert_primitive_topology(desc.primitive_topology);
    primitive_state.stripIndexFormat = WGPUIndexFormat_Undefined;
    primitive_state.frontFace = convert_front_face(desc.front_face);
    primitive_state.cullMode = convert_cull_mode(desc.cull_mode);
    primitive_state.unclippedDepth = WGPU_FALSE;

    WGPUDepthStencilState depth_stencil = {};
    const WGPUDepthStencilState* ds_ptr = nullptr;
    if (desc.depth_stencil) {
        depth_stencil.format = convert_format(desc.depth_stencil->format);
        depth_stencil.depthWriteEnabled = desc.depth_stencil->depth_write_enabled ? WGPUOptionalBool_True : WGPUOptionalBool_False;
        depth_stencil.depthCompare = convert_compare_function(desc.depth_stencil->depth_compare);
    } else {
        // WebGPU 校验要求: 挂了深度附件的 render pass 中所有管线都必须声明
        // depthStencilState。2D 精灵管线等效为 "不写深度 + 恒通过"。
        depth_stencil.format = WGPUTextureFormat_Depth32Float;
        depth_stencil.depthWriteEnabled = WGPUOptionalBool_False;
        depth_stencil.depthCompare = WGPUCompareFunction_Always;
    }
    {
        depth_stencil.stencilFront.compare = WGPUCompareFunction_Always;
        depth_stencil.stencilBack.compare = WGPUCompareFunction_Always;
        depth_stencil.stencilFront.failOp = WGPUStencilOperation_Keep;
        depth_stencil.stencilFront.depthFailOp = WGPUStencilOperation_Keep;
        depth_stencil.stencilFront.passOp = WGPUStencilOperation_Keep;
        depth_stencil.stencilBack.failOp = WGPUStencilOperation_Keep;
        depth_stencil.stencilBack.depthFailOp = WGPUStencilOperation_Keep;
        depth_stencil.stencilBack.passOp = WGPUStencilOperation_Keep;
        depth_stencil.depthBias = 0;
        depth_stencil.depthBiasSlopeScale = 0.0f;
        depth_stencil.depthBiasClamp = 0.0f;
        depth_stencil.stencilReadMask = 0xFFFFFFFF;
        depth_stencil.stencilWriteMask = 0xFFFFFFFF;
        ds_ptr = &depth_stencil;
    }

    WGPUMultisampleState multisample = {};
    multisample.count = 1;
    multisample.mask = 0xFFFFFFFF;
    multisample.alphaToCoverageEnabled = WGPU_FALSE;

    std::vector<WGPUBindGroupLayout> bind_group_layouts;
    bind_group_layouts.reserve(desc.bind_group_layouts.size());
    for (const auto& bgl : desc.bind_group_layouts) {
        bind_group_layouts.push_back(get_bind_group_layout(bgl));
    }

    WGPUPipelineLayoutDescriptor layout_desc = {};
    layout_desc.bindGroupLayoutCount = bind_group_layouts.size();
    layout_desc.bindGroupLayouts = bind_group_layouts.data();
    layout_desc.label = make_string_view("PipelineLayout");
    WGPUPipelineLayout pipeline_layout = wgpuDeviceCreatePipelineLayout(device, &layout_desc);

    WGPURenderPipelineDescriptor rp_desc = {};
    rp_desc.label = make_string_view("RenderPipeline");
    rp_desc.layout = pipeline_layout;
    rp_desc.vertex = vertex_state;
    rp_desc.primitive = primitive_state;
    rp_desc.multisample = multisample;
    rp_desc.depthStencil = ds_ptr;
    rp_desc.fragment = (fs) ? &fragment_state : nullptr;

    pipeline->pipeline = wgpuDeviceCreateRenderPipeline(device, &rp_desc);
    wgpuPipelineLayoutRelease(pipeline_layout);

    if (!pipeline->pipeline) {
        printf("Failed to create WebGPU render pipeline\n");
        return nullptr;
    }

    return std::static_pointer_cast<rhi::RHIRenderPipeline>(pipeline);
}

std::unique_ptr<rhi::RHICommandEncoder> WebGPUDevice::create_command_encoder() {
    if (!is_ready()) return nullptr;
    return std::make_unique<WebGPUCommandEncoder>(device, queue);
}

WGPUBindGroupLayout WebGPUDevice::get_bind_group_layout(const rhi::BindGroupLayoutDesc& desc) {
    u64 key = 0;
    for (const auto& e : desc.entries) {
        u64 h = (u64)e.binding;
        h |= ((u64)e.kind) << 32;
        key ^= h + 0x9e3779b97f4a7c15ull + (key << 6) + (key >> 2);
    }
    auto it = bind_group_layout_cache.find(key);
    if (it != bind_group_layout_cache.end()) return it->second;

    std::vector<WGPUBindGroupLayoutEntry> entries;
    entries.reserve(desc.entries.size());
    for (const auto& e : desc.entries) {
        WGPUBindGroupLayoutEntry entry = {};
        entry.binding = e.binding;
        entry.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
        if (e.kind == rhi::BindGroupEntryKind::Uniform) {
            entry.buffer.type = WGPUBufferBindingType_Uniform;
            entry.buffer.hasDynamicOffset = WGPU_FALSE;
            entry.buffer.minBindingSize = 0;
        } else if (e.kind == rhi::BindGroupEntryKind::Texture) {
            entry.texture.sampleType = WGPUTextureSampleType_Float;
            entry.texture.viewDimension = WGPUTextureViewDimension_2D;
            entry.texture.multisampled = WGPU_FALSE;
        } else {
            entry.sampler.type = WGPUSamplerBindingType_Filtering;
        }
        entries.push_back(entry);
    }

    WGPUBindGroupLayoutDescriptor layout_desc = {};
    layout_desc.label = make_string_view("BindGroupLayout");
    layout_desc.entryCount = entries.size();
    layout_desc.entries = entries.data();

    WGPUBindGroupLayout layout = wgpuDeviceCreateBindGroupLayout(device, &layout_desc);
    if (!layout) {
        printf("Failed to create WebGPU bind group layout\n");
        return nullptr;
    }
    bind_group_layout_cache[key] = layout;
    return layout;
}

std::shared_ptr<rhi::RHITexture> WebGPUDevice::create_texture(const rhi::TextureDesc& desc) {
    if (!is_ready()) return nullptr;

    auto tex = std::make_shared<WebGPUTexture>();
    tex->tex_w = desc.width;
    tex->tex_h = desc.height;

    WGPUTextureDescriptor tex_desc = {};
    tex_desc.label = make_string_view("Texture");
    tex_desc.usage = convert_texture_usage(desc.usage) | WGPUTextureUsage_TextureBinding;
    tex_desc.dimension = WGPUTextureDimension_2D;
    tex_desc.size = {desc.width, desc.height, 1};
    tex_desc.format = convert_texture_format(desc.format);
    tex_desc.mipLevelCount = 1;
    tex_desc.sampleCount = 1;

    tex->texture = wgpuDeviceCreateTexture(device, &tex_desc);
    if (!tex->texture) {
        printf("Failed to create WebGPU texture\n");
        return nullptr;
    }

    WGPUTextureViewDescriptor view_desc = {};
    view_desc.label = make_string_view("TextureView");
    view_desc.format = convert_texture_format(desc.format);
    view_desc.dimension = WGPUTextureViewDimension_2D;
    view_desc.baseMipLevel = 0;
    view_desc.mipLevelCount = 1;
    view_desc.baseArrayLayer = 0;
    view_desc.arrayLayerCount = 1;

    auto view = std::make_shared<WebGPUTextureView>();
    view->view = wgpuTextureCreateView(tex->texture, &view_desc);
    tex->texture_view = view;

    if (desc.initial_data) {
        update_texture(tex.get(), desc.initial_data);
    }

    return std::static_pointer_cast<rhi::RHITexture>(tex);
}

std::shared_ptr<rhi::RHISampler> WebGPUDevice::create_sampler(const rhi::SamplerDesc& desc) {
    if (!is_ready()) return nullptr;

    auto sampler = std::make_shared<WebGPUSampler>();
    WGPUSamplerDescriptor s_desc = {};
    s_desc.label = make_string_view("Sampler");
    s_desc.addressModeU = convert_address_mode(desc.address_mode_u);
    s_desc.addressModeV = convert_address_mode(desc.address_mode_v);
    s_desc.addressModeW = WGPUAddressMode_ClampToEdge;
    s_desc.magFilter = convert_texture_filter(desc.mag_filter);
    s_desc.minFilter = convert_texture_filter(desc.min_filter);
    s_desc.mipmapFilter = WGPUMipmapFilterMode_Linear;
    s_desc.lodMinClamp = 0.0f;
    s_desc.lodMaxClamp = 32.0f;
    s_desc.compare = WGPUCompareFunction_Undefined;
    s_desc.maxAnisotropy = 1;

    sampler->sampler = wgpuDeviceCreateSampler(device, &s_desc);
    if (!sampler->sampler) {
        printf("Failed to create WebGPU sampler\n");
        return nullptr;
    }
    return std::static_pointer_cast<rhi::RHISampler>(sampler);
}

std::shared_ptr<rhi::RHIBindGroup> WebGPUDevice::create_bind_group(const rhi::BindGroupLayoutDesc& layout_desc,
                                                                   const std::vector<rhi::BindGroupEntry>& entries) {
    if (!is_ready()) return nullptr;

    WGPUBindGroupLayout layout = get_bind_group_layout(layout_desc);
    if (!layout) return nullptr;

    std::vector<WGPUBindGroupEntry> wgpu_entries;
    wgpu_entries.reserve(entries.size());
    for (const auto& e : entries) {
        WGPUBindGroupEntry entry = {};
        entry.binding = e.binding;
        if (e.buffer) {
            entry.buffer = static_cast<WebGPUBuffer*>(e.buffer)->buffer;
            entry.offset = e.offset;
            entry.size = e.size > 0 ? e.size : WGPU_WHOLE_SIZE;
        } else if (e.texture_view) {
            entry.textureView = static_cast<WebGPUTextureView*>(e.texture_view)->view;
        } else if (e.sampler) {
            entry.sampler = static_cast<WebGPUSampler*>(e.sampler)->sampler;
        }
        wgpu_entries.push_back(entry);
    }

    auto bg = std::make_shared<WebGPUBindGroup>();
    WGPUBindGroupDescriptor bg_desc = {};
    bg_desc.label = make_string_view("BindGroup");
    bg_desc.layout = layout;
    bg_desc.entryCount = wgpu_entries.size();
    bg_desc.entries = wgpu_entries.data();

    bg->bind_group = wgpuDeviceCreateBindGroup(device, &bg_desc);
    if (!bg->bind_group) {
        printf("Failed to create WebGPU bind group\n");
        return nullptr;
    }
    return std::static_pointer_cast<rhi::RHIBindGroup>(bg);
}

void WebGPUDevice::update_buffer(rhi::RHIBuffer* buffer, u64 offset, const void* data, u64 size) {
    if (!buffer || !data || !size || !queue) return;
    auto* buf = static_cast<WebGPUBuffer*>(buffer);
    wgpuQueueWriteBuffer(queue, buf->buffer, offset, data, size);
}

void WebGPUDevice::update_texture(rhi::RHITexture* texture, const void* data) {
    if (!texture || !data || !queue) return;
    auto* tex = static_cast<WebGPUTexture*>(texture);

    WGPUTexelCopyTextureInfo dest = {};
    dest.texture = tex->texture;
    dest.mipLevel = 0;
    dest.origin = {0, 0, 0};
    dest.aspect = WGPUTextureAspect_All;

    WGPUTexelCopyBufferLayout layout = {};
    layout.offset = 0;
    layout.bytesPerRow = tex->tex_w * 4;
    layout.rowsPerImage = tex->tex_h;

    WGPUExtent3D size = {tex->tex_w, tex->tex_h, 1};
    wgpuQueueWriteTexture(queue, &dest, data, (size_t)tex->tex_w * tex->tex_h * 4, &layout, &size);
}

}

namespace aether::rhi {

std::unique_ptr<RHIDevice> create_webgpu_device() {
    return std::make_unique<webgpu::WebGPUDevice>();
}

}
