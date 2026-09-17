#pragma once

// ============================================================================
// World3D — 真 3D 网格渲染 (深度缓冲)
//
// 替代旧的 "2D 精灵手工投影" 星球渲染: 地形/水面/大气/戴森球全部以
// 真实三角形网格绘制, 由硬件深度测试解决遮挡, 不再需要 painter's
// algorithm 排层与近平面/地平线剔除等 hack。
//
// 与 SpriteBatch 共用同一个 render pass (先画 3D, 再画 2D 精灵层)。
// ============================================================================

#include "core/platform.h"
#include "core/math.h"
#include "rhi/rhi.h"
#include "demo/dyson_sphere/terrain.h"
#include <memory>
#include <vector>

namespace dsp {

using namespace aether;

class World3D {
public:
    // 所有 3D 几何共用的顶点格式: pos3 + normal3 + color4
    struct Vertex {
        Vec3 pos{0.0f, 0.0f, 0.0f};
        Vec3 normal{0.0f, 1.0f, 0.0f};
        Color color{1.0f, 1.0f, 1.0f, 1.0f};
    };

    bool init(rhi::RHIDevice* device, rhi::Format color_format, rhi::Format depth_format);
    void shutdown();
    bool ready() const { return device_ != nullptr && uniform_buffer_ != nullptr; }

    // --- 静态网格 (生成时构建一次) ---
    // 球形地形: 立方体球六面, 高度/颜色来自 TerrainField
    void build_terrain_sphere(const TerrainField& field, f32 radius, u32 res_per_face);
    // 水面: 海平面球壳 (半径 = radius + TerrainField::kSeaLevel)
    void build_water_sphere(f32 radius, u32 res_per_face);
    // 大气壳
    void build_atmosphere(f32 radius, u32 res_per_face);
    // 平面地形调试网格: 中心 center, 切平面基 east/north/up, 边长 extent (XZ 平面, +up 高度)
    void build_terrain_planar(const TerrainField& field, const Vec3& center,
                              const Vec3& east, const Vec3& north, const Vec3& up,
                              f32 extent, u32 res);
    void build_water_planar(const Vec3& center, const Vec3& east, const Vec3& north, const Vec3& up,
                            f32 extent, f32 altitude);

    // --- 每帧全局量 ---
    void set_globals(const Mat4& vp, const Vec3& eye, const Vec3& sun_dir, f32 time);

    void draw_terrain(rhi::RHICommandEncoder* enc);
    void draw_water(rhi::RHICommandEncoder* enc);
    void draw_atmosphere(rhi::RHICommandEncoder* enc);

    // --- 动态几何 (戴森球线架 / 帆板 / 太阳帆), 世界坐标, 每帧上传 ---
    void upload_lines(const Vertex* v, u32 count);   // LineList
    void upload_tris(const Vertex* v, u32 count);    // TriangleList
    void draw_lines(rhi::RHICommandEncoder* enc);
    void draw_tris(rhi::RHICommandEncoder* enc);

private:
    bool create_pipelines(rhi::Format color_format, rhi::Format depth_format);
    bool upload_mesh(std::shared_ptr<rhi::RHIBuffer>& vb, std::shared_ptr<rhi::RHIBuffer>& ib,
                     const std::vector<Vertex>& verts, const std::vector<u32>& indices);
    bool ensure_dynamic(std::shared_ptr<rhi::RHIBuffer>& buf, u64 vert_cap);

    rhi::RHIDevice* device_ = nullptr;
    rhi::Format depth_format_ = rhi::Format::Undefined;
    std::shared_ptr<rhi::RHIBuffer> uniform_buffer_;
    std::shared_ptr<rhi::RHIBindGroup> bind_group_;

    std::shared_ptr<rhi::RHIRenderPipeline> terrain_pipeline_;
    std::shared_ptr<rhi::RHIRenderPipeline> water_pipeline_;
    std::shared_ptr<rhi::RHIRenderPipeline> atmosphere_pipeline_;
    std::shared_ptr<rhi::RHIRenderPipeline> lines_pipeline_;
    std::shared_ptr<rhi::RHIRenderPipeline> tris_pipeline_;

    std::shared_ptr<rhi::RHIBuffer> terrain_vb_, terrain_ib_;
    u32 terrain_idx_count_ = 0;
    std::shared_ptr<rhi::RHIBuffer> water_vb_, water_ib_;
    u32 water_idx_count_ = 0;
    std::shared_ptr<rhi::RHIBuffer> atmo_vb_, atmo_ib_;
    u32 atmo_idx_count_ = 0;

    std::shared_ptr<rhi::RHIBuffer> line_vb_;
    u32 line_vert_count_ = 0;
    std::shared_ptr<rhi::RHIBuffer> tri_vb_;
    u32 tri_vert_count_ = 0;
};

} // namespace dsp
