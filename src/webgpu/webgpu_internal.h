#pragma once

#include "core/platform.h"
#include "rhi/rhi.h"
#include <webgpu/webgpu.h>
#include <cstring>
#include <unordered_map>

namespace aether::webgpu {

inline WGPUStringView make_string_view(const char* str) {
    WGPUStringView sv = {};
    sv.data = str;
    sv.length = str ? strlen(str) : 0;
    return sv;
}

WGPUBufferUsage convert_buffer_usage(u32 usage);
WGPUTextureUsage convert_texture_usage(u32 usage);
WGPUTextureFormat convert_format(rhi::Format format);
WGPUTextureFormat convert_texture_format(rhi::TextureFormat format);
WGPUPrimitiveTopology convert_primitive_topology(rhi::PrimitiveTopology topology);
WGPUFrontFace convert_front_face(rhi::FrontFace face);
WGPUCullMode convert_cull_mode(rhi::CullMode mode);
WGPUCompareFunction convert_compare_function(rhi::CompareOp op);
WGPULoadOp convert_load_op(rhi::LoadOp op);
WGPUStoreOp convert_store_op(rhi::StoreOp op);
WGPUVertexFormat convert_vertex_format(rhi::Format format);
WGPUShaderStage convert_shader_stage(rhi::ShaderStage stage);
WGPUFilterMode convert_texture_filter(rhi::TextureFilter filter);
WGPUAddressMode convert_address_mode(rhi::TextureAddressMode mode);

struct WebGPUBuffer : public rhi::RHIBuffer {
    WGPUBuffer buffer = nullptr;
    u64 buffer_size = 0;
    WGPUBufferUsage usage_flags = WGPUBufferUsage_None;
    void* mapped_data = nullptr;
    bool is_mapped = false;

    ~WebGPUBuffer() override;
    void* map() override;
    void unmap() override;
    u64 size() const override { return buffer_size; }
};

struct WebGPUShader : public rhi::RHIShader {
    WGPUShaderModule module = nullptr;
    rhi::ShaderStage stage;
    std::string entry_point;

    ~WebGPUShader() override;
};

struct WebGPUSampler : public rhi::RHISampler {
    WGPUSampler sampler = nullptr;

    ~WebGPUSampler() override;
};

struct WebGPUTextureView : public rhi::RHITextureView {
    WGPUTextureView view = nullptr;
    WGPUTexture texture = nullptr;

    ~WebGPUTextureView() override;
};

struct WebGPUTexture : public rhi::RHITexture {
    WGPUTexture texture = nullptr;
    std::shared_ptr<WebGPUTextureView> texture_view;
    u32 tex_w = 0;
    u32 tex_h = 0;

    WebGPUTexture() = default;
    ~WebGPUTexture() override;
    u32 width() const override { return tex_w; }
    u32 height() const override { return tex_h; }
    rhi::RHITextureView* get_view() override;
};

struct WebGPUBindGroup : public rhi::RHIBindGroup {
    WGPUBindGroup bind_group = nullptr;

    ~WebGPUBindGroup() override;
};

struct WebGPURenderPipeline : public rhi::RHIRenderPipeline {
    WGPURenderPipeline pipeline = nullptr;

    ~WebGPURenderPipeline() override;
};

struct WebGPUSwapchain : public rhi::RHISwapchain {
    WGPUSurface surface = nullptr;
    WGPUTexture current_texture = nullptr;
    WGPUTextureView current_view = nullptr;
    u32 w = 0;
    u32 h = 0;
    rhi::Format color_fmt = rhi::Format::BGRA8Unorm;
    rhi::Format depth_fmt = rhi::Format::Undefined;
    bool configured = false;

    WebGPUSwapchain() = default;
    ~WebGPUSwapchain() override;
    u32 width() const override { return w; }
    u32 height() const override { return h; }
    rhi::RHITextureView* get_current_view() override;
    rhi::Format color_format() const override { return color_fmt; }
    rhi::Format depth_format() const override { return depth_fmt; }
    bool present() override;
    void configure(WGPUDevice device);
};

struct WebGPUCommandEncoder : public rhi::RHICommandEncoder {
    WGPUCommandEncoder encoder = nullptr;
    WGPURenderPassEncoder render_pass = nullptr;
    WGPUDevice device = nullptr;
    WGPUQueue queue = nullptr;
    WGPUCommandBuffer command_buffer = nullptr;

    WebGPUCommandEncoder(WGPUDevice dev, WGPUQueue q);
    ~WebGPUCommandEncoder() override;
    void begin_render_pass(const rhi::RenderPassDesc& desc) override;
    void end_render_pass() override;
    void set_pipeline(rhi::RHIRenderPipeline* pipeline) override;
    void set_bind_group(u32 group_index, rhi::RHIBindGroup* bind_group) override;
    void set_vertex_buffer(u32 slot, rhi::RHIBuffer* buffer, u64 offset) override;
    void set_index_buffer(rhi::RHIBuffer* buffer, u64 offset, rhi::IndexFormat format) override;
    void draw(u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance) override;
    void draw_indexed(u32 index_count, u32 instance_count, u32 first_index, i32 vertex_offset, u32 first_instance) override;
    void finish() override;
    void submit() override;
};

struct WebGPUDevice : public rhi::RHIDevice {
    enum class InitState {
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

    WGPUInstance instance = nullptr;
    WGPUAdapter adapter = nullptr;
    WGPUDevice device = nullptr;
    WGPUQueue queue = nullptr;
    WGPUSurface surface = nullptr;
    std::unordered_map<u64, WGPUBindGroupLayout> bind_group_layout_cache;
    std::shared_ptr<WebGPUSwapchain> current_swapchain;
    InitState init_state = InitState::NotStarted;

    bool adapter_done = false;
    bool device_done = false;
    WGPURequestAdapterStatus adapter_status = WGPURequestAdapterStatus_Success;
    WGPURequestDeviceStatus device_status = WGPURequestDeviceStatus_Success;
    WGPUAdapter pending_adapter = nullptr;
    WGPUDevice pending_device = nullptr;

    ~WebGPUDevice() override;
    rhi::BackendType backend_type() const override { return rhi::BackendType::WebGPU; }
    bool init(void* native_window_handle) override;
    void wait_idle() override;
    void tick() override;

    bool is_ready() const { return init_state == InitState::Ready; }
    bool is_failed() const { return init_state == InitState::Failed; }

    std::shared_ptr<rhi::RHISwapchain> create_swapchain(u32 width, u32 height) override;
    std::shared_ptr<rhi::RHIBuffer> create_buffer(const rhi::BufferDesc& desc, const void* initial_data) override;
    std::shared_ptr<rhi::RHIShader> create_shader(rhi::ShaderStage stage, const rhi::ShaderModuleDesc& desc) override;
    std::shared_ptr<rhi::RHITexture> create_texture(const rhi::TextureDesc& desc) override;
    std::shared_ptr<rhi::RHISampler> create_sampler(const rhi::SamplerDesc& desc) override;
    std::shared_ptr<rhi::RHIRenderPipeline> create_render_pipeline(const rhi::RenderPipelineDesc& desc) override;
    std::shared_ptr<rhi::RHIBindGroup> create_bind_group(const rhi::BindGroupLayoutDesc& layout_desc, const std::vector<rhi::BindGroupEntry>& entries) override;
    void update_buffer(rhi::RHIBuffer* buffer, u64 offset, const void* data, u64 size) override;
    void update_texture(rhi::RHITexture* texture, const void* data) override;
    std::unique_ptr<rhi::RHICommandEncoder> create_command_encoder() override;

private:
    void step_init();
    WGPUBindGroupLayout get_bind_group_layout(const rhi::BindGroupLayoutDesc& desc);
    static void on_adapter_requested(WGPURequestAdapterStatus status, WGPUAdapter adapter,
                                     WGPUStringView message, void* userdata1, void* userdata2);
    static void on_device_requested(WGPURequestDeviceStatus status, WGPUDevice device,
                                    WGPUStringView message, void* userdata1, void* userdata2);
};

}
