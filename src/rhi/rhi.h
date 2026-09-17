#pragma once

#include "core/platform.h"
#include "rhi/rhi_types.h"

namespace aether::rhi {

class RHIBuffer {
public:
    virtual ~RHIBuffer() = default;
    virtual void* map() = 0;
    virtual void unmap() = 0;
    virtual u64 size() const = 0;
};

class RHIShader {
public:
    virtual ~RHIShader() = default;
};

class RHITexture {
public:
    virtual ~RHITexture() = default;
    virtual u32 width() const = 0;
    virtual u32 height() const = 0;
    virtual RHITextureView* get_view() = 0;
};

class RHISampler {
public:
    virtual ~RHISampler() = default;
};

class RHITextureView {
public:
    virtual ~RHITextureView() = default;
};

class RHISwapchain {
public:
    virtual ~RHISwapchain() = default;
    virtual u32 width() const = 0;
    virtual u32 height() const = 0;
    virtual RHITextureView* get_current_view() = 0;
    virtual RHITextureView* get_depth_view() { return nullptr; }
    virtual Format color_format() const = 0;
    virtual Format depth_format() const = 0;
    virtual bool present() = 0;
};

class RHICommandEncoder {
public:
    virtual ~RHICommandEncoder() = default;
    virtual void begin_render_pass(const RenderPassDesc& desc) = 0;
    virtual void end_render_pass() = 0;
    virtual void set_pipeline(class RHIRenderPipeline* pipeline) = 0;
    virtual void set_bind_group(u32 group_index, class RHIBindGroup* bind_group) = 0;
    virtual void set_vertex_buffer(u32 slot, RHIBuffer* buffer, u64 offset = 0) = 0;
    virtual void set_index_buffer(RHIBuffer* buffer, u64 offset = 0, IndexFormat format = IndexFormat::Uint32) = 0;
    virtual void draw(u32 vertex_count, u32 instance_count = 1, u32 first_vertex = 0, u32 first_instance = 0) = 0;
    virtual void draw_indexed(u32 index_count, u32 instance_count = 1, u32 first_index = 0, i32 vertex_offset = 0, u32 first_instance = 0) = 0;
    virtual void finish() = 0;
    virtual void submit() = 0;
};

class RHIBindGroup {
public:
    virtual ~RHIBindGroup() = default;
};

class RHIRenderPipeline {
public:
    virtual ~RHIRenderPipeline() = default;
};

class RHIDevice {
public:
    virtual ~RHIDevice() = default;
    virtual BackendType backend_type() const = 0;
    virtual bool init(void* native_window_handle = nullptr) = 0;
    virtual void wait_idle() = 0;
    virtual void tick() = 0;

    virtual bool is_ready() const { return true; }
    virtual bool is_failed() const { return false; }

    virtual std::shared_ptr<RHISwapchain> create_swapchain(u32 width, u32 height) = 0;
    virtual std::shared_ptr<RHIBuffer> create_buffer(const BufferDesc& desc, const void* initial_data = nullptr) = 0;
    virtual std::shared_ptr<RHIShader> create_shader(ShaderStage stage, const ShaderModuleDesc& desc) = 0;
    virtual std::shared_ptr<RHITexture> create_texture(const TextureDesc& desc) = 0;
    virtual std::shared_ptr<RHISampler> create_sampler(const SamplerDesc& desc) = 0;
    virtual std::shared_ptr<RHIRenderPipeline> create_render_pipeline(const RenderPipelineDesc& desc) = 0;
    virtual std::shared_ptr<RHIBindGroup> create_bind_group(const BindGroupLayoutDesc& layout_desc, const std::vector<BindGroupEntry>& entries) = 0;
    virtual void update_buffer(RHIBuffer* buffer, u64 offset, const void* data, u64 size) = 0;
    virtual void update_texture(RHITexture* texture, const void* data) = 0;
    virtual std::unique_ptr<RHICommandEncoder> create_command_encoder() = 0;
    virtual void toggle_fullscreen() {}
};

std::unique_ptr<RHIDevice> create_webgpu_device();
std::unique_ptr<RHIDevice> create_d3d11_device();

}
