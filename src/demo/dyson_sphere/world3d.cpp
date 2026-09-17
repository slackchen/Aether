#include "demo/dyson_sphere/world3d.h"
#include <cmath>
#include <cstring>
#include <functional>
#include <algorithm>

namespace dsp {

using aether::rhi::RHIDevice;
using aether::rhi::RHICommandEncoder;

// ---------------------------------------------------------------------------
// Shader 源码 (HLSL / WGSL 双份, 与 engine/shaders.cpp 同一约定:
// b0 = 全局常量, 顶点语义 ATTRIBn)
//
// 顶点布局: ATTRIB0 pos(float3) ATTRIB1 normal(float3) ATTRIB2 color(float4)
// ---------------------------------------------------------------------------
namespace {

#ifdef __EMSCRIPTEN__

const char* kLitVS = R"(
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

const char* kTerrainFS = R"(
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

const char* kWaterFS = R"(
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

const char* kAtmosphereFS = R"(
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

const char* kFlatVS = R"(
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

const char* kFlatFS = R"(
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

const char* kLitVS = R"(
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

const char* kTerrainFS = R"(
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

const char* kWaterFS = R"(
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

const char* kAtmosphereFS = R"(
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

const char* kFlatVS = R"(
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

const char* kFlatFS = R"(
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
rhi::VertexAttribute kAttrs[3] = {
    {0, 0, rhi::Format::Float32x3},
    {1, 12, rhi::Format::Float32x3},
    {2, 24, rhi::Format::Float32x4},
};
rhi::VertexBufferLayout kVertexLayout = {40, 3, kAttrs};

// 通用球面网格: (face, u, v) → 顶点
struct SphereSample {
    Vec3 pos;
    Vec3 normal;
    Color color;
};

void build_sphere_grid(u32 res, std::vector<World3D::Vertex>& verts,
                       std::vector<u32>& indices,
                       const std::function<SphereSample(const Vec3& dir)>& sample) {
    verts.clear();
    indices.clear();
    for (u32 face = 0; face < 6; face++) {
        u32 base = (u32)verts.size();
        for (u32 j = 0; j <= res; j++) {
            for (u32 i = 0; i <= res; i++) {
                Vec3 dir = TerrainField::cube_face_point((i32)face, (f32)i / res, (f32)j / res);
                SphereSample s = sample(dir);
                verts.push_back({s.pos, s.normal, s.color});
            }
        }
        for (u32 j = 0; j < res; j++) {
            for (u32 i = 0; i < res; i++) {
                u32 v00 = base + j * (res + 1) + i;
                u32 v10 = v00 + 1;
                u32 v01 = v00 + (res + 1);
                u32 v11 = v01 + 1;
                indices.push_back(v00);
                indices.push_back(v10);
                indices.push_back(v11);
                indices.push_back(v00);
                indices.push_back(v11);
                indices.push_back(v01);
            }
        }
    }
}

} // namespace

// ---------------------------------------------------------------------------
// 初始化
// ---------------------------------------------------------------------------
bool World3D::init(RHIDevice* device, rhi::Format color_format, rhi::Format depth_format) {
    if (!device) return false;
    device_ = device;
    depth_format_ = depth_format;
    if (depth_format_ == rhi::Format::Undefined) {
        depth_format_ = rhi::Format::Depth32Float;
    }

    uniform_buffer_ = device_->create_buffer({96, (u32)rhi::BufferUsage::Uniform,
                                              rhi::BufferMemoryType::HostVisible});
    if (!uniform_buffer_) return false;

    rhi::BindGroupLayoutDesc bgl;
    bgl.entries.push_back({0, rhi::BindGroupEntryKind::Uniform});
    std::vector<rhi::BindGroupEntry> entries;
    entries.push_back({0, uniform_buffer_.get(), nullptr, nullptr, 0, 0});
    bind_group_ = device_->create_bind_group(bgl, entries);
    if (!bind_group_) return false;

    return create_pipelines(color_format, depth_format_);
}

bool World3D::create_pipelines(rhi::Format color_format, rhi::Format depth_format) {
    struct ShaderPair { const char* vs; const char* fs; };
    auto make_shader = [&](rhi::ShaderStage stage, const char* src) {
        rhi::ShaderModuleDesc desc;
        desc.code = src;
        desc.code_size = strlen(src);
        desc.entry_point = stage == rhi::ShaderStage::Vertex ? "vs_main" : "fs_main";
        return device_->create_shader(stage, desc);
    };

    auto lit_vs = make_shader(rhi::ShaderStage::Vertex, kLitVS);
    auto flat_vs = make_shader(rhi::ShaderStage::Vertex, kFlatVS);
    auto terrain_fs = make_shader(rhi::ShaderStage::Fragment, kTerrainFS);
    auto water_fs = make_shader(rhi::ShaderStage::Fragment, kWaterFS);
    auto atmo_fs = make_shader(rhi::ShaderStage::Fragment, kAtmosphereFS);
    auto flat_fs = make_shader(rhi::ShaderStage::Fragment, kFlatFS);
    if (!lit_vs || !flat_vs || !terrain_fs || !water_fs || !atmo_fs || !flat_fs) {
        printf("World3D: shader compilation failed\n");
        return false;
    }

    rhi::ColorTargetState color_target;
    color_target.format = color_format;

    rhi::DepthStencilState depth_write{depth_format, true, rhi::CompareOp::Less};
    rhi::DepthStencilState depth_test{depth_format, false, rhi::CompareOp::LessOrEqual};

    auto make_pipeline = [&](const rhi::RHIShader* vs, const rhi::RHIShader* fs,
                             rhi::PrimitiveTopology topo, rhi::CullMode cull,
                             const rhi::DepthStencilState& ds, rhi::BlendMode blend)
            -> std::shared_ptr<rhi::RHIRenderPipeline> {
        rhi::RenderPipelineDesc pd;
        pd.vertex_shader = vs;
        pd.fragment_shader = fs;
        pd.primitive_topology = topo;
        pd.vertex_layout = kVertexLayout;
        pd.color_target_count = 1;
        pd.color_targets = &color_target;
        pd.depth_stencil = &ds;
        pd.front_face = rhi::FrontFace::CCW;
        pd.cull_mode = cull;
        pd.blend_mode = blend;
        rhi::BindGroupLayoutDesc bgl;
        bgl.entries.push_back({0, rhi::BindGroupEntryKind::Uniform});
        pd.bind_group_layouts.push_back(bgl);
        auto p = device_->create_render_pipeline(pd);
        if (!p) printf("World3D: pipeline creation failed\n");
        return p;
    };

    terrain_pipeline_ = make_pipeline(lit_vs.get(), terrain_fs.get(), rhi::PrimitiveTopology::TriangleList,
                                      rhi::CullMode::Back, depth_write, rhi::BlendMode::Alpha);
    water_pipeline_ = make_pipeline(lit_vs.get(), water_fs.get(), rhi::PrimitiveTopology::TriangleList,
                                    rhi::CullMode::Back, depth_test, rhi::BlendMode::Alpha);
    atmosphere_pipeline_ = make_pipeline(lit_vs.get(), atmo_fs.get(), rhi::PrimitiveTopology::TriangleList,
                                         rhi::CullMode::Front, depth_test, rhi::BlendMode::Additive);
    lines_pipeline_ = make_pipeline(flat_vs.get(), flat_fs.get(), rhi::PrimitiveTopology::LineList,
                                    rhi::CullMode::None, depth_test, rhi::BlendMode::Additive);
    tris_pipeline_ = make_pipeline(flat_vs.get(), flat_fs.get(), rhi::PrimitiveTopology::TriangleList,
                                   rhi::CullMode::None, depth_test, rhi::BlendMode::Additive);
    return terrain_pipeline_ && water_pipeline_ && atmosphere_pipeline_ && lines_pipeline_ && tris_pipeline_;
}

void World3D::shutdown() {
    terrain_vb_.reset(); terrain_ib_.reset(); terrain_idx_count_ = 0;
    water_vb_.reset(); water_ib_.reset(); water_idx_count_ = 0;
    atmo_vb_.reset(); atmo_ib_.reset(); atmo_idx_count_ = 0;
    line_vb_.reset(); line_vert_count_ = 0;
    tri_vb_.reset(); tri_vert_count_ = 0;
    terrain_pipeline_.reset(); water_pipeline_.reset(); atmosphere_pipeline_.reset();
    lines_pipeline_.reset(); tris_pipeline_.reset();
    bind_group_.reset();
    uniform_buffer_.reset();
    device_ = nullptr;
}

// ---------------------------------------------------------------------------
// 网格构建
// ---------------------------------------------------------------------------
bool World3D::upload_mesh(std::shared_ptr<rhi::RHIBuffer>& vb, std::shared_ptr<rhi::RHIBuffer>& ib,
                          const std::vector<Vertex>& verts, const std::vector<u32>& indices) {
    vb.reset();
    ib.reset();
    vb = device_->create_buffer({(u64)verts.size() * sizeof(Vertex), (u32)rhi::BufferUsage::Vertex,
                                 rhi::BufferMemoryType::DeviceLocal}, verts.data());
    ib = device_->create_buffer({(u64)indices.size() * sizeof(u32), (u32)rhi::BufferUsage::Index,
                                 rhi::BufferMemoryType::DeviceLocal}, indices.data());
    return vb != nullptr && ib != nullptr;
}

void World3D::build_terrain_sphere(const TerrainField& field, f32 radius, u32 res_per_face) {
    if (!device_) return;
    std::vector<Vertex> verts;
    std::vector<u32> indices;
    build_sphere_grid(res_per_face, verts, indices, [&](const Vec3& dir) -> SphereSample {
        f32 h = field.sphere_height(dir);
        Vec3 n = field.sphere_normal(dir, radius);
        f32 slope = std::clamp((1.0f - (n.x * dir.x + n.y * dir.y + n.z * dir.z)) * 7.0f, 0.0f, 1.0f);
        Color c = TerrainField::terrain_color(field.sphere_normalized(dir), slope, field.detail3(dir));
        Vec3 pos{dir.x * (radius + h), dir.y * (radius + h), dir.z * (radius + h)};
        return {pos, n, c};
    });
    if (upload_mesh(terrain_vb_, terrain_ib_, verts, indices)) {
        terrain_idx_count_ = (u32)indices.size();
        printf("World3D: terrain sphere %u verts / %u tris\n",
               (u32)verts.size(), terrain_idx_count_ / 3);
    }
}

void World3D::build_water_sphere(f32 radius, u32 res_per_face) {
    if (!device_) return;
    std::vector<Vertex> verts;
    std::vector<u32> indices;
    Color water{0.07f, 0.22f, 0.38f, 0.80f};
    build_sphere_grid(res_per_face, verts, indices, [&](const Vec3& dir) -> SphereSample {
        Vec3 pos{dir.x * radius, dir.y * radius, dir.z * radius};
        return {pos, dir, water};
    });
    if (upload_mesh(water_vb_, water_ib_, verts, indices)) {
        water_idx_count_ = (u32)indices.size();
    }
}

void World3D::build_atmosphere(f32 radius, u32 res_per_face) {
    if (!device_) return;
    std::vector<Vertex> verts;
    std::vector<u32> indices;
    Color white{1.0f, 1.0f, 1.0f, 1.0f};
    build_sphere_grid(res_per_face, verts, indices, [&](const Vec3& dir) -> SphereSample {
        Vec3 pos{dir.x * radius, dir.y * radius, dir.z * radius};
        return {pos, dir, white};
    });
    if (upload_mesh(atmo_vb_, atmo_ib_, verts, indices)) {
        atmo_idx_count_ = (u32)indices.size();
    }
}

void World3D::build_terrain_planar(const TerrainField& field, const Vec3& center,
                                   const Vec3& east, const Vec3& north, const Vec3& up,
                                   f32 extent, u32 res) {
    if (!device_) return;
    std::vector<Vertex> verts;
    verts.reserve((size_t)(res + 1) * (res + 1));
    std::vector<u32> indices;
    indices.reserve((size_t)res * res * 6);
    f32 half = extent * 0.5f;
    for (u32 j = 0; j <= res; j++) {
        for (u32 i = 0; i <= res; i++) {
            f32 lx = -half + extent * (f32)i / res;
            f32 lz = -half + extent * (f32)j / res;
            // 平面 (x, z) 采样坐标: center 沿 east/north 展开
            f32 x = center.x + east.x * lx + north.x * lz;
            f32 z = center.z + east.z * lx + north.z * lz;
            f32 h = field.height(x, z);
            Vec3 pn = field.planar_normal(x, z, extent / res * 0.75f);
            Vec3 n = {east.x * pn.x + up.x * pn.y + north.x * pn.z,
                      east.y * pn.x + up.y * pn.y + north.y * pn.z,
                      east.z * pn.x + up.z * pn.y + north.z * pn.z};
            f32 nl = sqrtf(n.x * n.x + n.y * n.y + n.z * n.z);
            n = {n.x / nl, n.y / nl, n.z / nl};
            f32 slope = std::clamp((1.0f - pn.y) * 3.0f, 0.0f, 1.0f);
            Color c = TerrainField::terrain_color(field.planar_normalized(x, z), slope, field.detail2(x, z));
            Vec3 pos{center.x + east.x * lx + north.x * lz + up.x * h,
                     center.y + east.y * lx + north.y * lz + up.y * h,
                     center.z + east.z * lx + north.z * lz + up.z * h};
            verts.push_back({pos, n, c});
        }
    }
    for (u32 j = 0; j < res; j++) {
        for (u32 i = 0; i < res; i++) {
            u32 v00 = j * (res + 1) + i;
            u32 v10 = v00 + 1;
            u32 v01 = v00 + (res + 1);
            u32 v11 = v01 + 1;
            indices.push_back(v00); indices.push_back(v10); indices.push_back(v11);
            indices.push_back(v00); indices.push_back(v11); indices.push_back(v01);
        }
    }
    if (upload_mesh(terrain_vb_, terrain_ib_, verts, indices)) {
        terrain_idx_count_ = (u32)indices.size();
        printf("World3D: terrain planar %u verts / %u tris\n", (u32)verts.size(), terrain_idx_count_ / 3);
    }
}

void World3D::build_water_planar(const Vec3& center, const Vec3& east, const Vec3& north, const Vec3& up,
                                 f32 extent, f32 altitude) {
    if (!device_) return;
    f32 half = extent * 0.5f;
    Color water{0.07f, 0.22f, 0.38f, 0.80f};
    auto p = [&](f32 lx, f32 lz) {
        return Vec3{center.x + east.x * lx + north.x * lz + up.x * altitude,
                    center.y + east.y * lx + north.y * lz + up.y * altitude,
                    center.z + east.z * lx + north.z * lz + up.z * altitude};
    };
    std::vector<Vertex> verts = {
        {p(-half, -half), up, water},
        {p(+half, -half), up, water},
        {p(+half, +half), up, water},
        {p(-half, +half), up, water},
    };
    std::vector<u32> indices = {0, 1, 2, 0, 2, 3};
    if (upload_mesh(water_vb_, water_ib_, verts, indices)) {
        water_idx_count_ = (u32)indices.size();
    }
}

// ---------------------------------------------------------------------------
// 动态几何
// ---------------------------------------------------------------------------
bool World3D::ensure_dynamic(std::shared_ptr<rhi::RHIBuffer>& buf, u64 vert_cap) {
    if (buf && buf->size() >= vert_cap * sizeof(Vertex)) return true;
    buf.reset();
    buf = device_->create_buffer({vert_cap * sizeof(Vertex), (u32)rhi::BufferUsage::Vertex,
                                  rhi::BufferMemoryType::HostVisible});
    return buf != nullptr;
}

void World3D::upload_lines(const Vertex* v, u32 count) {
    line_vert_count_ = count;
    if (count == 0) return;
    if (!ensure_dynamic(line_vb_, count)) { line_vert_count_ = 0; return; }
    device_->update_buffer(line_vb_.get(), 0, v, (u64)count * sizeof(Vertex));
}

void World3D::upload_tris(const Vertex* v, u32 count) {
    tri_vert_count_ = count;
    if (count == 0) return;
    if (!ensure_dynamic(tri_vb_, count)) { tri_vert_count_ = 0; return; }
    device_->update_buffer(tri_vb_.get(), 0, v, (u64)count * sizeof(Vertex));
}

// ---------------------------------------------------------------------------
// 每帧绘制
// ---------------------------------------------------------------------------
void World3D::set_globals(const Mat4& vp, const Vec3& eye, const Vec3& sun_dir, f32 time) {
    if (!uniform_buffer_) return;
    f32 data[24];
    memcpy(data, vp.m, sizeof(vp.m));
    data[16] = sun_dir.x; data[17] = sun_dir.y; data[18] = sun_dir.z; data[19] = time;
    data[20] = eye.x; data[21] = eye.y; data[22] = eye.z; data[23] = 0.0f;
    device_->update_buffer(uniform_buffer_.get(), 0, data, sizeof(data));
}

void World3D::draw_terrain(RHICommandEncoder* enc) {
    if (!terrain_vb_ || terrain_idx_count_ == 0) return;
    enc->set_pipeline(terrain_pipeline_.get());
    enc->set_bind_group(0, bind_group_.get());
    enc->set_vertex_buffer(0, terrain_vb_.get());
    enc->set_index_buffer(terrain_ib_.get());
    enc->draw_indexed(terrain_idx_count_);
}

void World3D::draw_water(RHICommandEncoder* enc) {
    if (!water_vb_ || water_idx_count_ == 0) return;
    enc->set_pipeline(water_pipeline_.get());
    enc->set_bind_group(0, bind_group_.get());
    enc->set_vertex_buffer(0, water_vb_.get());
    enc->set_index_buffer(water_ib_.get());
    enc->draw_indexed(water_idx_count_);
}

void World3D::draw_atmosphere(RHICommandEncoder* enc) {
    if (!atmo_vb_ || atmo_idx_count_ == 0) return;
    enc->set_pipeline(atmosphere_pipeline_.get());
    enc->set_bind_group(0, bind_group_.get());
    enc->set_vertex_buffer(0, atmo_vb_.get());
    enc->set_index_buffer(atmo_ib_.get());
    enc->draw_indexed(atmo_idx_count_);
}

void World3D::draw_lines(RHICommandEncoder* enc) {
    if (!line_vb_ || line_vert_count_ == 0) return;
    enc->set_pipeline(lines_pipeline_.get());
    enc->set_bind_group(0, bind_group_.get());
    enc->set_vertex_buffer(0, line_vb_.get());
    enc->draw(line_vert_count_);
}

void World3D::draw_tris(RHICommandEncoder* enc) {
    if (!tri_vb_ || tri_vert_count_ == 0) return;
    enc->set_pipeline(tris_pipeline_.get());
    enc->set_bind_group(0, bind_group_.get());
    enc->set_vertex_buffer(0, tri_vb_.get());
    enc->draw(tri_vert_count_);
}

} // namespace dsp
