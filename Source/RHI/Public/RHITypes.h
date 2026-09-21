#pragma once

#include "Core.h"
#include "Container/Array.h"

namespace Aether::RHI {

enum class BackendType
{
    WebGPU,
    Vulkan,
    D3D11,
    D3D12,
    Metal,
    Remote
};

enum class BufferUsage
{
    Vertex = 1 << 0,
    Index = 1 << 1,
    Uniform = 1 << 2,
    Storage = 1 << 3,
    CopySrc = 1 << 4,
    CopyDst = 1 << 5,
};

enum class BufferMemoryType
{
    DeviceLocal,
    HostVisible,
    HostCoherent,
};

enum class ShaderStage
{
    Vertex = 1 << 0,
    Fragment = 1 << 1,
    Compute = 1 << 2,
};

enum class PrimitiveTopology
{
    PointList,
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip,
};

enum class FrontFace
{
    CCW,
    CW,
};

enum class CullMode
{
    None,
    Front,
    Back,
};

enum class CompareOp
{
    Never,
    Less,
    Equal,
    LessOrEqual,
    Greater,
    NotEqual,
    GreaterOrEqual,
    Always,
};

enum class LoadOp
{
    Load,
    Clear,
    DontCare,
};

enum class StoreOp
{
    Store,
    DontCare,
};

enum class Format
{
    Undefined,
    RGBA8Unorm,
    BGRA8Unorm,
    Depth32Float,
    Depth24PlusStencil8,
    Float32x2,
    Float32x3,
    Float32x4,
};

enum class TextureFormat
{
    Undefined,
    RGBA8Unorm,
};

enum class TextureUsage
{
    Sampled = 1 << 0,
    RenderAttachment = 1 << 1,
    CopyDst = 1 << 2,
    CopySrc = 1 << 3,
};

enum class TextureFilter
{
    Nearest,
    Linear,
};

enum class TextureAddressMode
{
    Repeat,
    ClampToEdge,
};

enum class BlendMode
{
    Alpha,
    Additive,
};

enum class IndexFormat
{
    Uint16,
    Uint32,
};

enum class BindGroupEntryKind
{
    Uniform,
    Texture,
    Sampler,
};

struct TextureDesc
{
    u32 Width = 0;
    u32 Height = 0;
    TextureFormat Format = TextureFormat::RGBA8Unorm;
    u32 Usage = 0;
    const void* InitialData = nullptr;
};

struct SamplerDesc
{
    TextureFilter MagFilter = TextureFilter::Linear;
    TextureFilter MinFilter = TextureFilter::Linear;
    TextureAddressMode AddressModeU = TextureAddressMode::ClampToEdge;
    TextureAddressMode AddressModeV = TextureAddressMode::ClampToEdge;
};

struct BindGroupLayoutEntry
{
    u32 Binding = 0;
    BindGroupEntryKind Kind = BindGroupEntryKind::Uniform;
};

struct BindGroupLayoutDesc
{
    Array<BindGroupLayoutEntry> Entries;
};

struct BindGroupEntry
{
    u32 Binding = 0;
    class RHIBuffer* Buffer = nullptr;
    class RHITextureView* TextureView = nullptr;
    class RHISampler* Sampler = nullptr;
    u64 Offset = 0;
    u64 Size = 0;
};

struct BufferDesc
{
    u64 Size = 0;
    u32 Usage = 0;
    BufferMemoryType MemoryType = BufferMemoryType::DeviceLocal;
};

struct ShaderModuleDesc
{
    const void* Code = nullptr;
    u64 CodeSize = 0;
    const char* EntryPoint = "main";
};

struct VertexAttribute
{
    u32 Location = 0;
    u32 Offset = 0;
    Format Format = Format::RGBA8Unorm;
};

struct VertexBufferLayout
{
    u32 Stride = 0;
    u32 AttributeCount = 0;
    const VertexAttribute* Attributes = nullptr;
};

struct ColorTargetState
{
    Format Format = Format::BGRA8Unorm;
};

struct DepthStencilState
{
    Format Format = Format::Depth32Float;
    bool DepthWriteEnabled = true;
    CompareOp DepthCompare = CompareOp::Less;
};

struct PipelineLayoutDesc
{
};

struct RenderPipelineDesc
{
    const class RHIShader* VertexShader = nullptr;
    const class RHIShader* FragmentShader = nullptr;
    PrimitiveTopology PrimitiveTopology = PrimitiveTopology::TriangleList;
    VertexBufferLayout VertexLayout;
    u32 ColorTargetCount = 1;
    const ColorTargetState* ColorTargets = nullptr;
    const DepthStencilState* DepthStencil = nullptr;
    FrontFace FrontFace = FrontFace::CCW;
    CullMode CullMode = CullMode::None;
    BlendMode BlendMode = BlendMode::Alpha;
    Array<BindGroupLayoutDesc> BindGroupLayouts;
};

struct RenderPassColorAttachment
{
    class RHITextureView* View = nullptr;
    LoadOp LoadOp = LoadOp::Clear;
    StoreOp StoreOp = StoreOp::Store;
    f32 ClearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
};

struct RenderPassDepthAttachment
{
    class RHITextureView* View = nullptr;
    LoadOp LoadOp = LoadOp::Clear;
    StoreOp StoreOp = StoreOp::Store;
    f32 ClearDepth = 1.0f;
};

struct RenderPassDesc
{
    u32 ColorAttachmentCount = 0;
    const RenderPassColorAttachment* ColorAttachments = nullptr;
    const RenderPassDepthAttachment* DepthAttachment = nullptr;
};

}
