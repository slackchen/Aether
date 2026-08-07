#pragma once

#include "core/platform.h"
#include <vector>

namespace aether::rhi {

enum class BackendType {
    WebGPU,
    Vulkan,
    D3D11,
    D3D12,
    Metal,
    Remote
};

enum class BufferUsage {
    Vertex = 1 << 0,
    Index = 1 << 1,
    Uniform = 1 << 2,
    Storage = 1 << 3,
    CopySrc = 1 << 4,
    CopyDst = 1 << 5,
};

enum class BufferMemoryType {
    DeviceLocal,
    HostVisible,
    HostCoherent,
};

enum class ShaderStage {
    Vertex = 1 << 0,
    Fragment = 1 << 1,
    Compute = 1 << 2,
};

enum class PrimitiveTopology {
    PointList,
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip,
};

enum class FrontFace {
    CCW,
    CW,
};

enum class CullMode {
    None,
    Front,
    Back,
};

enum class CompareOp {
    Never,
    Less,
    Equal,
    LessOrEqual,
    Greater,
    NotEqual,
    GreaterOrEqual,
    Always,
};

enum class LoadOp {
    Load,
    Clear,
    DontCare,
};

enum class StoreOp {
    Store,
    DontCare,
};

enum class Format {
    Undefined,
    RGBA8Unorm,
    BGRA8Unorm,
    Depth32Float,
    Depth24PlusStencil8,
    Float32x2,
    Float32x3,
    Float32x4,
};

enum class TextureFormat {
    Undefined,
    RGBA8Unorm,
};

enum class TextureUsage {
    Sampled = 1 << 0,
    RenderAttachment = 1 << 1,
    CopyDst = 1 << 2,
    CopySrc = 1 << 3,
};

enum class TextureFilter {
    Nearest,
    Linear,
};

enum class TextureAddressMode {
    Repeat,
    ClampToEdge,
};

enum class BlendMode {
    Alpha,
    Additive,
};

enum class IndexFormat {
    Uint16,
    Uint32,
};

enum class BindGroupEntryKind {
    Uniform,
    Texture,
    Sampler,
};

struct TextureDesc {
    u32 width = 0;
    u32 height = 0;
    TextureFormat format = TextureFormat::RGBA8Unorm;
    u32 usage = 0;
    const void* initial_data = nullptr;
};

struct SamplerDesc {
    TextureFilter mag_filter = TextureFilter::Linear;
    TextureFilter min_filter = TextureFilter::Linear;
    TextureAddressMode address_mode_u = TextureAddressMode::ClampToEdge;
    TextureAddressMode address_mode_v = TextureAddressMode::ClampToEdge;
};

struct BindGroupLayoutEntry {
    u32 binding = 0;
    BindGroupEntryKind kind = BindGroupEntryKind::Uniform;
};

struct BindGroupLayoutDesc {
    std::vector<BindGroupLayoutEntry> entries;
};

struct BindGroupEntry {
    u32 binding = 0;
    class RHIBuffer* buffer = nullptr;
    class RHITextureView* texture_view = nullptr;
    class RHISampler* sampler = nullptr;
    u64 offset = 0;
    u64 size = 0;
};

struct BufferDesc {
    u64 size = 0;
    u32 usage = 0;
    BufferMemoryType memory_type = BufferMemoryType::DeviceLocal;
};

struct ShaderModuleDesc {
    const void* code = nullptr;
    u64 code_size = 0;
    const char* entry_point = "main";
};

struct VertexAttribute {
    u32 location = 0;
    u32 offset = 0;
    Format format = Format::RGBA8Unorm;
};

struct VertexBufferLayout {
    u32 stride = 0;
    u32 attribute_count = 0;
    const VertexAttribute* attributes = nullptr;
};

struct ColorTargetState {
    Format format = Format::BGRA8Unorm;
};

struct DepthStencilState {
    Format format = Format::Depth32Float;
    bool depth_write_enabled = true;
    CompareOp depth_compare = CompareOp::Less;
};

struct PipelineLayoutDesc {};

struct RenderPipelineDesc {
    const class RHIShader* vertex_shader = nullptr;
    const class RHIShader* fragment_shader = nullptr;
    PrimitiveTopology primitive_topology = PrimitiveTopology::TriangleList;
    VertexBufferLayout vertex_layout;
    u32 color_target_count = 1;
    const ColorTargetState* color_targets = nullptr;
    const DepthStencilState* depth_stencil = nullptr;
    FrontFace front_face = FrontFace::CCW;
    CullMode cull_mode = CullMode::None;
    BlendMode blend_mode = BlendMode::Alpha;
    std::vector<BindGroupLayoutDesc> bind_group_layouts;
};

struct RenderPassColorAttachment {
    class RHITextureView* view = nullptr;
    LoadOp load_op = LoadOp::Clear;
    StoreOp store_op = StoreOp::Store;
    f32 clear_color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
};

struct RenderPassDepthAttachment {
    class RHITextureView* view = nullptr;
    LoadOp load_op = LoadOp::Clear;
    StoreOp store_op = StoreOp::Store;
    f32 clear_depth = 1.0f;
};

struct RenderPassDesc {
    u32 color_attachment_count = 0;
    const RenderPassColorAttachment* color_attachments = nullptr;
    const RenderPassDepthAttachment* depth_attachment = nullptr;
};

}
