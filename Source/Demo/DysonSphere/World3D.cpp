#include "World3D.h"
#include "Container/Function.h"
#include "Math/Math.h"

#include <cmath>
#include <cstring>
#include <cstdio>

namespace DSP {

using namespace Aether;
using namespace Aether::Math;

// ---------------------------------------------------------------------------
// Shader 源码 (HLSL / WGSL 双份, 与 engine/shaders.cpp 同一约定:
// b0 = 全局常量, 顶点语义 ATTRIBn)
//
// 顶点布局: ATTRIB0 pos(float3) ATTRIB1 normal(float3) ATTRIB2 color(float4)
// ---------------------------------------------------------------------------
namespace {

#ifdef __EMSCRIPTEN__

const char* LIT_VS = R"(
struct Globals {
    vp: mat4x4f,
    sun: vec4f,
    eye: vec4f,
};
@group(0) @binding(0) var<uniform> u_globals: Globals;

struct VSIn {
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
    @location(2) color: vec4f,
};
struct VSOut {
    @builtin(position) position: vec4f,
    @location(0) normal: vec3f,
    @location(1) world: vec3f,
    @location(2) color: vec4f,
};

@vertex
fn vs_main(input: VSIn) -> VSOut {
    var o: VSOut;
    o.position = u_globals.vp * vec4f(input.position, 1.0);
    o.normal = input.normal;
    o.world = input.position;
    o.color = input.color;
    return o;
}
)";

const char* TERRAIN_FS = R"(
struct Globals {
    vp: mat4x4f,
    sun: vec4f,
    eye: vec4f,
};
@group(0) @binding(0) var<uniform> u_globals: Globals;

struct FSIn {
    @builtin(position) frag: vec4f,
    @location(0) normal: vec3f,
    @location(1) world: vec3f,
    @location(2) color: vec4f,
};

@fragment
fn fs_main(input: FSIn) -> @location(0) vec4f {
    let n = normalize(input.normal);
    let nl = max(0.0, dot(n, u_globals.sun.xyz));
    let v = normalize(u_globals.eye.xyz - input.world);
    let fres = pow(1.0 - clamp(dot(n, v), 0.0, 1.0), 3.0);
    var col = input.color.rgb * (0.30 + 0.78 * nl);
    col += vec3f(0.05, 0.08, 0.13) * fres;
    return vec4f(col, 1.0);
}
)";

const char* WATER_FS = R"(
struct Globals {
    vp: mat4x4f,
    sun: vec4f,
    eye: vec4f,
};
@group(0) @binding(0) var<uniform> u_globals: Globals;

struct FSIn {
    @builtin(position) frag: vec4f,
    @location(0) normal: vec3f,
    @location(1) world: vec3f,
    @location(2) color: vec4f,
};

@fragment
fn fs_main(input: FSIn) -> @location(0) vec4f {
    let n = normalize(input.normal);
    let nl = max(0.0, dot(n, u_globals.sun.xyz));
    let v = normalize(u_globals.eye.xyz - input.world);
    let h = normalize(u_globals.sun.xyz + v);
    let spec = pow(max(0.0, dot(n, h)), 90.0);
    let col = vec3f(0.07, 0.22, 0.38) * (0.35 + 0.75 * nl) + vec3f(1.0, 0.95, 0.8) * spec * 0.9;
    return vec4f(col, 0.80);
}
)";

const char* ATMOSPHERE_FS = R"(
struct Globals {
    vp: mat4x4f,
    sun: vec4f,
    eye: vec4f,
};
@group(0) @binding(0) var<uniform> u_globals: Globals;

struct FSIn {
    @builtin(position) frag: vec4f,
    @location(0) normal: vec3f,
    @location(1) world: vec3f,
    @location(2) color: vec4f,
};

@fragment
fn fs_main(input: FSIn) -> @location(0) vec4f {
    let n = normalize(input.normal);
    let v = normalize(u_globals.eye.xyz - input.world);
    let rim = pow(1.0 - clamp(abs(dot(n, v)), 0.0, 1.0), 2.0);
    let day = clamp(dot(n, u_globals.sun.xyz), 0.0, 1.0);
    var col = vec3f(0.30, 0.55, 1.0) * rim * (0.12 + 1.15 * day);
    let twil = pow(clamp(1.0 - abs(dot(n, u_globals.sun.xyz)), 0.0, 1.0), 3.0) * clamp(day * 2.0, 0.0, 1.0);
    col += vec3f(1.0, 0.45, 0.15) * rim * twil * 0.8;
    return vec4f(col, 1.0);
}
)";

const char* FLAT_VS = R"(
struct Globals {
    vp: mat4x4f,
    sun: vec4f,
    eye: vec4f,
};
@group(0) @binding(0) var<uniform> u_globals: Globals;

struct VSIn {
    @location(0) position: vec3f,
    @location(2) color: vec4f,
};
struct VSOut {
    @builtin(position) position: vec4f,
    @location(0) color: vec4f,
};

@vertex
fn vs_main(input: VSIn) -> VSOut {
    var o: VSOut;
    o.position = u_globals.vp * vec4f(input.position, 1.0);
    o.color = input.color;
    return o;
}
)";

const char* FLAT_FS = R"(
struct FSIn {
    @builtin(position) frag: vec4f,
    @location(0) color: vec4f,
};
@fragment
fn fs_main(input: FSIn) -> @location(0) vec4f {
    return input.color;
}
)";

#else

const char* LIT_VS = R"(
cbuffer Globals : register(b0) {
    float4x4 u_vp;
    float4 u_sun;   // xyz = 方向, w = time
    float4 u_eye;   // xyz = 相机位置
};

struct VSIn {
    float3 position : ATTRIB0;
    float3 normal : ATTRIB1;
    float4 color : ATTRIB2;
};

struct VSOut {
    float4 position : SV_Position;
    float3 normal : TEXCOORD0;
    float3 world : TEXCOORD1;
    float4 color : COLOR;
};

VSOut vs_main(VSIn input) {
    VSOut o;
    o.position = mul(u_vp, float4(input.position, 1.0));
    o.normal = input.normal;
    o.world = input.position;
    o.color = input.color;
    return o;
}
)";

const char* TERRAIN_FS = R"(
cbuffer Globals : register(b0) {
    float4x4 u_vp;
    float4 u_sun;
    float4 u_eye;
};

struct VSOut {
    float4 position : SV_Position;
    float3 normal : TEXCOORD0;
    float3 world : TEXCOORD1;
    float4 color : COLOR;
};

float4 fs_main(VSOut input) : SV_Target {
    float3 n = normalize(input.normal);
    float nl = max(0.0, dot(n, u_sun.xyz));
    float3 v = normalize(u_eye.xyz - input.world);
    float fres = pow(1.0 - saturate(dot(n, v)), 3.0);
    float3 col = input.color.rgb * (0.30 + 0.78 * nl);
    col += float3(0.05, 0.08, 0.13) * fres;
    return float4(col, 1.0);
}
)";

const char* WATER_FS = R"(
cbuffer Globals : register(b0) {
    float4x4 u_vp;
    float4 u_sun;
    float4 u_eye;
};

struct VSOut {
    float4 position : SV_Position;
    float3 normal : TEXCOORD0;
    float3 world : TEXCOORD1;
    float4 color : COLOR;
};

float4 fs_main(VSOut input) : SV_Target {
    float3 n = normalize(input.normal);
    float nl = max(0.0, dot(n, u_sun.xyz));
    float3 v = normalize(u_eye.xyz - input.world);
    float3 h = normalize(u_sun.xyz + v);
    float spec = pow(max(0.0, dot(n, h)), 90.0);
    float3 col = float3(0.07, 0.22, 0.38) * (0.35 + 0.75 * nl) + float3(1.0, 0.95, 0.8) * spec * 0.9;
    return float4(col, 0.80);
}
)";

const char* ATMOSPHERE_FS = R"(
cbuffer Globals : register(b0) {
    float4x4 u_vp;
    float4 u_sun;
    float4 u_eye;
};

struct VSOut {
    float4 position : SV_Position;
    float3 normal : TEXCOORD0;
    float3 world : TEXCOORD1;
    float4 color : COLOR;
};

float4 fs_main(VSOut input) : SV_Target {
    float3 n = normalize(input.normal);
    float3 v = normalize(u_eye.xyz - input.world);
    float rim = pow(1.0 - saturate(abs(dot(n, v))), 2.0);
    float day = saturate(dot(n, u_sun.xyz));
    float3 col = float3(0.30, 0.55, 1.0) * rim * (0.12 + 1.15 * day);
    float twil = pow(saturate(1.0 - abs(dot(n, u_sun.xyz))), 3.0) * saturate(day * 2.0);
    col += float3(1.0, 0.45, 0.15) * rim * twil * 0.8;
    return float4(col, 1.0);
}
)";

const char* FLAT_VS = R"(
cbuffer Globals : register(b0) {
    float4x4 u_vp;
    float4 u_sun;
    float4 u_eye;
};

struct VSIn {
    float3 position : ATTRIB0;
    float4 color : ATTRIB2;
};

struct VSOut {
    float4 position : SV_Position;
    float4 color : COLOR;
};

VSOut vs_main(VSIn input) {
    VSOut o;
    o.position = mul(u_vp, float4(input.position, 1.0));
    o.color = input.color;
    return o;
}
)";

const char* FLAT_FS = R"(
struct VSOut {
    float4 position : SV_Position;
    float4 color : COLOR;
};

float4 fs_main(VSOut input) : SV_Target {
    return input.color;
}
)";

#endif

// 顶点布局: pos3 + normal3 + color4, stride 40
RHI::VertexAttribute ATTRS[3] = {
    {0, 0, RHI::Format::Float32x3},
    {1, 12, RHI::Format::Float32x3},
    {2, 24, RHI::Format::Float32x4},
};
RHI::VertexBufferLayout VERTEX_LAYOUT = {40, 3, ATTRS};

// 通用球面网格: (face, u, v) → 顶点
struct SphereSample {
    Vec3 Position;
    Vec3 Normal;
    Color Color;
};

void BuildSphereGrid(u32 res, Array<World3D::Vertex>& verts,
                     Array<u32>& indices,
                     const Function<SphereSample(const Vec3& dir)>& sample)
{
    verts.Clear();
    indices.Clear();
    for (u32 face = 0; face < 6; face++)
    {
        u32 base = verts.Count();
        for (u32 j = 0; j <= res; j++)
        {
            for (u32 i = 0; i <= res; i++)
            {
                Vec3 dir = TerrainField::CubeFacePoint((i32)face, (f32)i / res, (f32)j / res);
                SphereSample s = sample(dir);
                verts.Add({s.Position, s.Normal, s.Color});
            }
        }
        for (u32 j = 0; j < res; j++)
        {
            for (u32 i = 0; i < res; i++)
            {
                u32 v00 = base + j * (res + 1) + i;
                u32 v10 = v00 + 1;
                u32 v01 = v00 + (res + 1);
                u32 v11 = v01 + 1;
                indices.Add(v00);
                indices.Add(v10);
                indices.Add(v11);
                indices.Add(v00);
                indices.Add(v11);
                indices.Add(v01);
            }
        }
    }
}

} // namespace

// ---------------------------------------------------------------------------
// 初始化
// ---------------------------------------------------------------------------
bool World3D::Init(RHI::RHIDevice* device, RHI::Format colorFormat, RHI::Format depthFormat)
{
    if (!device) return false;
    mDevice = device;
    mDepthFormat = depthFormat;
    if (mDepthFormat == RHI::Format::Undefined)
    {
        mDepthFormat = RHI::Format::Depth32Float;
    }

    mUniformBuffer = mDevice->CreateBuffer({96, (u32)RHI::BufferUsage::Uniform,
                                            RHI::BufferMemoryType::HostVisible});
    if (!mUniformBuffer) return false;

    RHI::BindGroupLayoutDesc bgl;
    bgl.Entries.Add({0, RHI::BindGroupEntryKind::Uniform});
    Array<RHI::BindGroupEntry> entries;
    entries.Add({0, mUniformBuffer.Get(), nullptr, nullptr, 0, 0});
    mBindGroup = mDevice->CreateBindGroup(bgl, entries);
    if (!mBindGroup) return false;

    return CreatePipelines(colorFormat, mDepthFormat);
}

bool World3D::CreatePipelines(RHI::Format colorFormat, RHI::Format depthFormat)
{
    auto makeShader = [&](RHI::ShaderStage stage, const char* src) {
        RHI::ShaderModuleDesc desc;
        desc.Code = src;
        desc.CodeSize = (u64)strlen(src);
        desc.EntryPoint = stage == RHI::ShaderStage::Vertex ? "vs_main" : "fs_main";
        return mDevice->CreateShader(stage, desc);
    };

    RefPtr<RHI::RHIShader> litVs = makeShader(RHI::ShaderStage::Vertex, LIT_VS);
    RefPtr<RHI::RHIShader> flatVs = makeShader(RHI::ShaderStage::Vertex, FLAT_VS);
    RefPtr<RHI::RHIShader> terrainFs = makeShader(RHI::ShaderStage::Fragment, TERRAIN_FS);
    RefPtr<RHI::RHIShader> waterFs = makeShader(RHI::ShaderStage::Fragment, WATER_FS);
    RefPtr<RHI::RHIShader> atmoFs = makeShader(RHI::ShaderStage::Fragment, ATMOSPHERE_FS);
    RefPtr<RHI::RHIShader> flatFs = makeShader(RHI::ShaderStage::Fragment, FLAT_FS);
    if (!litVs || !flatVs || !terrainFs || !waterFs || !atmoFs || !flatFs)
    {
        printf("World3D: shader compilation failed\n");
        return false;
    }

    RHI::ColorTargetState colorTarget;
    colorTarget.Format = colorFormat;

    RHI::DepthStencilState depthWrite{depthFormat, true, RHI::CompareOp::Less};
    RHI::DepthStencilState depthTest{depthFormat, false, RHI::CompareOp::LessOrEqual};

    auto makePipeline = [&](const RHI::RHIShader* vs, const RHI::RHIShader* fs,
                            RHI::PrimitiveTopology topo, RHI::CullMode cull,
                            const RHI::DepthStencilState& ds, RHI::BlendMode blend)
            -> RefPtr<RHI::RHIRenderPipeline> {
        RHI::RenderPipelineDesc pd;
        pd.VertexShader = vs;
        pd.FragmentShader = fs;
        pd.PrimitiveTopology = topo;
        pd.VertexLayout = VERTEX_LAYOUT;
        pd.ColorTargetCount = 1;
        pd.ColorTargets = &colorTarget;
        pd.DepthStencil = &ds;
        pd.FrontFace = RHI::FrontFace::CCW;
        pd.CullMode = cull;
        pd.BlendMode = blend;
        RHI::BindGroupLayoutDesc bgl;
        bgl.Entries.Add({0, RHI::BindGroupEntryKind::Uniform});
        pd.BindGroupLayouts.Add(bgl);
        RefPtr<RHI::RHIRenderPipeline> p = mDevice->CreateRenderPipeline(pd);
        if (!p) printf("World3D: pipeline creation failed\n");
        return p;
    };

    mTerrainPipeline = makePipeline(litVs.Get(), terrainFs.Get(), RHI::PrimitiveTopology::TriangleList,
                                    RHI::CullMode::Back, depthWrite, RHI::BlendMode::Alpha);
    mWaterPipeline = makePipeline(litVs.Get(), waterFs.Get(), RHI::PrimitiveTopology::TriangleList,
                                  RHI::CullMode::Back, depthTest, RHI::BlendMode::Alpha);
    mAtmospherePipeline = makePipeline(litVs.Get(), atmoFs.Get(), RHI::PrimitiveTopology::TriangleList,
                                       RHI::CullMode::Front, depthTest, RHI::BlendMode::Additive);
    mLinesPipeline = makePipeline(flatVs.Get(), flatFs.Get(), RHI::PrimitiveTopology::LineList,
                                  RHI::CullMode::None, depthTest, RHI::BlendMode::Additive);
    mTrisPipeline = makePipeline(flatVs.Get(), flatFs.Get(), RHI::PrimitiveTopology::TriangleList,
                                 RHI::CullMode::None, depthTest, RHI::BlendMode::Additive);
    return mTerrainPipeline && mWaterPipeline && mAtmospherePipeline && mLinesPipeline && mTrisPipeline;
}

void World3D::Shutdown()
{
    mTerrainVB.Reset(); mTerrainIB.Reset(); mTerrainIdxCount = 0;
    mWaterVB.Reset(); mWaterIB.Reset(); mWaterIdxCount = 0;
    mAtmoVB.Reset(); mAtmoIB.Reset(); mAtmoIdxCount = 0;
    mLineVB.Reset(); mLineVertCount = 0;
    mTriVB.Reset(); mTriVertCount = 0;
    mTerrainPipeline.Reset(); mWaterPipeline.Reset(); mAtmospherePipeline.Reset();
    mLinesPipeline.Reset(); mTrisPipeline.Reset();
    mBindGroup.Reset();
    mUniformBuffer.Reset();
    mDevice = nullptr;
}

// ---------------------------------------------------------------------------
// 网格构建
// ---------------------------------------------------------------------------
bool World3D::UploadMesh(RefPtr<RHI::RHIBuffer>& vb, RefPtr<RHI::RHIBuffer>& ib,
                         const Array<Vertex>& verts, const Array<u32>& indices)
{
    vb.Reset();
    ib.Reset();
    vb = mDevice->CreateBuffer({(u64)verts.Count() * sizeof(Vertex), (u32)RHI::BufferUsage::Vertex,
                                RHI::BufferMemoryType::DeviceLocal}, verts.Data());
    ib = mDevice->CreateBuffer({(u64)indices.Count() * sizeof(u32), (u32)RHI::BufferUsage::Index,
                                RHI::BufferMemoryType::DeviceLocal}, indices.Data());
    return vb != nullptr && ib != nullptr;
}

void World3D::BuildTerrainSphere(const TerrainField& field, f32 radius, u32 resPerFace)
{
    if (!mDevice) return;
    Array<Vertex> verts;
    Array<u32> indices;
    BuildSphereGrid(resPerFace, verts, indices, [&](const Vec3& dir) -> SphereSample {
        f32 h = field.SphereHeight(dir);
        Vec3 n = field.SphereNormal(dir, radius);
        f32 slope = Math::Clamp((1.0f - (n.x * dir.x + n.y * dir.y + n.z * dir.z)) * 7.0f, 0.0f, 1.0f);
        Color c = TerrainField::TerrainColor(field.SphereNormalized(dir), slope, field.Detail3(dir));
        Vec3 pos{dir.x * (radius + h), dir.y * (radius + h), dir.z * (radius + h)};
        return {pos, n, c};
    });
    if (UploadMesh(mTerrainVB, mTerrainIB, verts, indices))
    {
        mTerrainIdxCount = indices.Count();
        printf("World3D: terrain sphere %u verts / %u tris\n",
               verts.Count(), mTerrainIdxCount / 3);
    }
}

void World3D::BuildWaterSphere(f32 radius, u32 resPerFace)
{
    if (!mDevice) return;
    Array<Vertex> verts;
    Array<u32> indices;
    Color water{0.07f, 0.22f, 0.38f, 0.80f};
    BuildSphereGrid(resPerFace, verts, indices, [&](const Vec3& dir) -> SphereSample {
        Vec3 pos{dir.x * radius, dir.y * radius, dir.z * radius};
        return {pos, dir, water};
    });
    if (UploadMesh(mWaterVB, mWaterIB, verts, indices))
    {
        mWaterIdxCount = indices.Count();
    }
}

void World3D::BuildAtmosphere(f32 radius, u32 resPerFace)
{
    if (!mDevice) return;
    Array<Vertex> verts;
    Array<u32> indices;
    Color white{1.0f, 1.0f, 1.0f, 1.0f};
    BuildSphereGrid(resPerFace, verts, indices, [&](const Vec3& dir) -> SphereSample {
        Vec3 pos{dir.x * radius, dir.y * radius, dir.z * radius};
        return {pos, dir, white};
    });
    if (UploadMesh(mAtmoVB, mAtmoIB, verts, indices))
    {
        mAtmoIdxCount = indices.Count();
    }
}

void World3D::BuildTerrainPlanar(const TerrainField& field, const Vec3& center,
                                 const Vec3& east, const Vec3& north, const Vec3& up,
                                 f32 extent, u32 res)
{
    if (!mDevice) return;
    Array<Vertex> verts;
    verts.Reserve((u64)(res + 1) * (res + 1));
    Array<u32> indices;
    indices.Reserve((u64)res * res * 6);
    f32 half = extent * 0.5f;
    for (u32 j = 0; j <= res; j++)
    {
        for (u32 i = 0; i <= res; i++)
        {
            f32 lx = -half + extent * (f32)i / res;
            f32 lz = -half + extent * (f32)j / res;
            // 平面 (x, z) 采样坐标: center 沿 east/north 展开
            f32 x = center.x + east.x * lx + north.x * lz;
            f32 z = center.z + east.z * lx + north.z * lz;
            f32 h = field.Height(x, z);
            Vec3 pn = field.PlanarNormal(x, z, extent / res * 0.75f);
            Vec3 n = {east.x * pn.x + up.x * pn.y + north.x * pn.z,
                      east.y * pn.x + up.y * pn.y + north.y * pn.z,
                      east.z * pn.x + up.z * pn.y + north.z * pn.z};
            f32 nl = sqrtf(n.x * n.x + n.y * n.y + n.z * n.z);
            n = {n.x / nl, n.y / nl, n.z / nl};
            f32 slope = Math::Clamp((1.0f - pn.y) * 3.0f, 0.0f, 1.0f);
            Color c = TerrainField::TerrainColor(field.PlanarNormalized(x, z), slope, field.Detail2(x, z));
            Vec3 pos{center.x + east.x * lx + north.x * lz + up.x * h,
                     center.y + east.y * lx + north.y * lz + up.y * h,
                     center.z + east.z * lx + north.z * lz + up.z * h};
            verts.Add({pos, n, c});
        }
    }
    for (u32 j = 0; j < res; j++)
    {
        for (u32 i = 0; i < res; i++)
        {
            u32 v00 = j * (res + 1) + i;
            u32 v10 = v00 + 1;
            u32 v01 = v00 + (res + 1);
            u32 v11 = v01 + 1;
            indices.Add(v00); indices.Add(v10); indices.Add(v11);
            indices.Add(v00); indices.Add(v11); indices.Add(v01);
        }
    }
    if (UploadMesh(mTerrainVB, mTerrainIB, verts, indices))
    {
        mTerrainIdxCount = indices.Count();
        printf("World3D: terrain planar %u verts / %u tris\n", verts.Count(), mTerrainIdxCount / 3);
    }
}

void World3D::BuildWaterPlanar(const Vec3& center, const Vec3& east, const Vec3& north, const Vec3& up,
                               f32 extent, f32 altitude)
{
    if (!mDevice) return;
    f32 half = extent * 0.5f;
    Color water{0.07f, 0.22f, 0.38f, 0.80f};
    auto p = [&](f32 lx, f32 lz) {
        return Vec3{center.x + east.x * lx + north.x * lz + up.x * altitude,
                    center.y + east.y * lx + north.y * lz + up.y * altitude,
                    center.z + east.z * lx + north.z * lz + up.z * altitude};
    };
    Array<Vertex> verts = {
        {p(-half, -half), up, water},
        {p(+half, -half), up, water},
        {p(+half, +half), up, water},
        {p(-half, +half), up, water},
    };
    Array<u32> indices = {0, 1, 2, 0, 2, 3};
    if (UploadMesh(mWaterVB, mWaterIB, verts, indices))
    {
        mWaterIdxCount = indices.Count();
    }
}

// ---------------------------------------------------------------------------
// 动态几何
// ---------------------------------------------------------------------------
bool World3D::EnsureDynamic(RefPtr<RHI::RHIBuffer>& buf, u64 vertCap)
{
    if (buf && buf->Size() >= vertCap * sizeof(Vertex)) return true;
    buf.Reset();
    buf = mDevice->CreateBuffer({vertCap * sizeof(Vertex), (u32)RHI::BufferUsage::Vertex,
                                 RHI::BufferMemoryType::HostVisible});
    return buf != nullptr;
}

void World3D::UploadLines(const Vertex* v, u32 count)
{
    mLineVertCount = count;
    if (count == 0) return;
    if (!EnsureDynamic(mLineVB, count)) { mLineVertCount = 0; return; }
    mDevice->UpdateBuffer(mLineVB.Get(), 0, v, (u64)count * sizeof(Vertex));
}

void World3D::UploadTris(const Vertex* v, u32 count)
{
    mTriVertCount = count;
    if (count == 0) return;
    if (!EnsureDynamic(mTriVB, count)) { mTriVertCount = 0; return; }
    mDevice->UpdateBuffer(mTriVB.Get(), 0, v, (u64)count * sizeof(Vertex));
}

// ---------------------------------------------------------------------------
// 每帧绘制
// ---------------------------------------------------------------------------
void World3D::SetGlobals(const Mat4& vp, const Vec3& eye, const Vec3& sunDir, f32 time)
{
    if (!mUniformBuffer) return;
    f32 data[24];
    memcpy(data, vp.m, sizeof(vp.m));
    data[16] = sunDir.x; data[17] = sunDir.y; data[18] = sunDir.z; data[19] = time;
    data[20] = eye.x; data[21] = eye.y; data[22] = eye.z; data[23] = 0.0f;
    mDevice->UpdateBuffer(mUniformBuffer.Get(), 0, data, sizeof(data));
}

void World3D::DrawTerrain(RHI::RHICommandEncoder* enc)
{
    if (!mTerrainVB || mTerrainIdxCount == 0) return;
    enc->SetPipeline(mTerrainPipeline.Get());
    enc->SetBindGroup(0, mBindGroup.Get());
    enc->SetVertexBuffer(0, mTerrainVB.Get());
    enc->SetIndexBuffer(mTerrainIB.Get());
    enc->DrawIndexed(mTerrainIdxCount);
}

void World3D::DrawWater(RHI::RHICommandEncoder* enc)
{
    if (!mWaterVB || mWaterIdxCount == 0) return;
    enc->SetPipeline(mWaterPipeline.Get());
    enc->SetBindGroup(0, mBindGroup.Get());
    enc->SetVertexBuffer(0, mWaterVB.Get());
    enc->SetIndexBuffer(mWaterIB.Get());
    enc->DrawIndexed(mWaterIdxCount);
}

void World3D::DrawAtmosphere(RHI::RHICommandEncoder* enc)
{
    if (!mAtmoVB || mAtmoIdxCount == 0) return;
    enc->SetPipeline(mAtmospherePipeline.Get());
    enc->SetBindGroup(0, mBindGroup.Get());
    enc->SetVertexBuffer(0, mAtmoVB.Get());
    enc->SetIndexBuffer(mAtmoIB.Get());
    enc->DrawIndexed(mAtmoIdxCount);
}

void World3D::DrawLines(RHI::RHICommandEncoder* enc)
{
    if (!mLineVB || mLineVertCount == 0) return;
    enc->SetPipeline(mLinesPipeline.Get());
    enc->SetBindGroup(0, mBindGroup.Get());
    enc->SetVertexBuffer(0, mLineVB.Get());
    enc->Draw(mLineVertCount);
}

void World3D::DrawTris(RHI::RHICommandEncoder* enc)
{
    if (!mTriVB || mTriVertCount == 0) return;
    enc->SetPipeline(mTrisPipeline.Get());
    enc->SetBindGroup(0, mBindGroup.Get());
    enc->SetVertexBuffer(0, mTriVB.Get());
    enc->Draw(mTriVertCount);
}

} // namespace DSP
