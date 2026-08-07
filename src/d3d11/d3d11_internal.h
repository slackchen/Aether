#pragma once

#include "core/platform.h"
#include "rhi/rhi.h"

#ifndef __EMSCRIPTEN__
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <memory>
#include <string>
#include <vector>
#endif

namespace aether::d3d11 {

#ifndef __EMSCRIPTEN__

using Microsoft::WRL::ComPtr;

struct D3D11Buffer : public rhi::RHIBuffer {
    ComPtr<ID3D11Buffer> buffer;
    ComPtr<ID3D11DeviceContext> ctx;
    u64 buffer_size = 0;
    bool dynamic = false;
    bool mapped_ = false;
    bool dirty_ = false;
    std::vector<u8> cpu_data;

    ~D3D11Buffer() override = default;
    void* map() override;
    void unmap() override;
    u64 size() const override { return buffer_size; }
};

struct D3D11Shader : public rhi::RHIShader {
    rhi::ShaderStage stage = rhi::ShaderStage::Vertex;
    ComPtr<ID3D11VertexShader> vs;
    ComPtr<ID3D11PixelShader> ps;
    ComPtr<ID3DBlob> blob;

    ~D3D11Shader() override = default;
};

struct D3D11TextureView : public rhi::RHITextureView {
    ComPtr<ID3D11ShaderResourceView> srv;
    ComPtr<ID3D11RenderTargetView> rtv;

    ~D3D11TextureView() override = default;
};

struct D3D11Texture : public rhi::RHITexture {
    ComPtr<ID3D11Texture2D> texture;
    std::shared_ptr<D3D11TextureView> texture_view;
    u32 tex_w = 0;
    u32 tex_h = 0;

    ~D3D11Texture() override = default;
    u32 width() const override { return tex_w; }
    u32 height() const override { return tex_h; }
    rhi::RHITextureView* get_view() override;
};

struct D3D11Sampler : public rhi::RHISampler {
    ComPtr<ID3D11SamplerState> sampler;

    ~D3D11Sampler() override = default;
};

struct D3D11BindGroup : public rhi::RHIBindGroup {
    ComPtr<ID3D11Buffer> constant_buffer;
    ComPtr<ID3D11ShaderResourceView> srv;
    ComPtr<ID3D11SamplerState> sampler;

    ~D3D11BindGroup() override = default;
};

struct D3D11RenderPipeline : public rhi::RHIRenderPipeline {
    ComPtr<ID3D11VertexShader> vs;
    ComPtr<ID3D11PixelShader> ps;
    ComPtr<ID3D11InputLayout> input_layout;
    ComPtr<ID3D11RasterizerState> rasterizer;
    ComPtr<ID3D11BlendState> blend;
    u32 vertex_stride = 0;
    D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    ~D3D11RenderPipeline() override = default;
};

struct D3D11Swapchain : public rhi::RHISwapchain {
    ComPtr<ID3D11Device> device;
    ComPtr<IDXGISwapChain1> swapchain;
    ComPtr<ID3D11RenderTargetView> rtv;
    ComPtr<ID3D11Texture2D> backbuffer;
    HWND hwnd = nullptr;
    u32 w = 0;
    u32 h = 0;
    u32 rtv_w = 0;
    u32 rtv_h = 0;
    rhi::Format color_fmt = rhi::Format::BGRA8Unorm;
    bool configured = false;

    D3D11Swapchain(ComPtr<ID3D11Device> dev, ComPtr<IDXGISwapChain1> sc, HWND hwnd, u32 width, u32 height);
    ~D3D11Swapchain() override = default;
    u32 width() const override { return w; }
    u32 height() const override { return h; }
    rhi::RHITextureView* get_current_view() override;
    rhi::Format color_format() const override { return color_fmt; }
    rhi::Format depth_format() const override { return rhi::Format::Undefined; }
    bool present() override;
    void toggle_fullscreen();
};

struct D3D11CommandEncoder : public rhi::RHICommandEncoder {
    ComPtr<ID3D11DeviceContext> ctx;
    u32 vertex_stride = 0;
    f32 viewport_w = 1.0f;
    f32 viewport_h = 1.0f;

    explicit D3D11CommandEncoder(ComPtr<ID3D11DeviceContext> context, f32 width, f32 height);
    ~D3D11CommandEncoder() override = default;
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

struct D3D11Device : public rhi::RHIDevice {
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<IDXGISwapChain1> swapchain;
    ComPtr<ID3D11InfoQueue> info_queue;
    HWND hwnd = nullptr;
    bool init_ok = false;
    std::shared_ptr<D3D11Swapchain> current_swapchain;

    ~D3D11Device() override = default;
    rhi::BackendType backend_type() const override { return rhi::BackendType::D3D11; }
    bool init(void* native_window_handle) override;
    void wait_idle() override;
    void tick() override;
    bool is_ready() const override { return init_ok; }
    bool is_failed() const override { return !init_ok; }

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
    void toggle_fullscreen() override;
};

#endif

}
