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

#include "Core.h"
#include "Math/Mat4.h"
#include "Math/Vec3.h"
#include "Math/Color.h"
#include "RHI.h"
#include "Container/Array.h"
#include "Container/RefPtr.h"
#include "Terrain.h"

namespace DSP {

using Aether::f32;
using Aether::u32;
using Aether::u64;
using Aether::Math::Color;
using Aether::Math::Mat4;
using Aether::Math::Vec3;
using Aether::Array;
using Aether::RefPtr;

class World3D {
public:
    // 所有 3D 几何共用的顶点格式: pos3 + normal3 + color4
    struct Vertex {
        Vec3 Position{0.0f, 0.0f, 0.0f};
        Vec3 Normal{0.0f, 1.0f, 0.0f};
        Color Color{1.0f, 1.0f, 1.0f, 1.0f};
    };

    bool Init(Aether::RHI::RHIDevice* device, Aether::RHI::Format colorFormat, Aether::RHI::Format depthFormat);
    void Shutdown();
    bool IsReady() const { return mDevice != nullptr && mUniformBuffer != nullptr; }

    // --- 静态网格 (生成时构建一次) ---
    // 球形地形: 立方体球六面, 高度/颜色来自 TerrainField
    void BuildTerrainSphere(const TerrainField& field, f32 radius, u32 resPerFace);
    // 水面: 海平面球壳 (半径 = radius + TerrainField::SEA_LEVEL)
    void BuildWaterSphere(f32 radius, u32 resPerFace);
    // 大气壳
    void BuildAtmosphere(f32 radius, u32 resPerFace);
    // 平面地形调试网格: 中心 center, 切平面基 east/north/up, 边长 extent (XZ 平面, +up 高度)
    void BuildTerrainPlanar(const TerrainField& field, const Vec3& center,
                            const Vec3& east, const Vec3& north, const Vec3& up,
                            f32 extent, u32 res);
    void BuildWaterPlanar(const Vec3& center, const Vec3& east, const Vec3& north, const Vec3& up,
                          f32 extent, f32 altitude);

    // --- 每帧全局量 ---
    void SetGlobals(const Mat4& vp, const Vec3& eye, const Vec3& sunDir, f32 time);

    void DrawTerrain(Aether::RHI::RHICommandEncoder* enc);
    void DrawWater(Aether::RHI::RHICommandEncoder* enc);
    void DrawAtmosphere(Aether::RHI::RHICommandEncoder* enc);

    // --- 动态几何 (戴森球线架 / 帆板 / 太阳帆), 世界坐标, 每帧上传 ---
    void UploadLines(const Vertex* v, u32 count);   // LineList
    void UploadTris(const Vertex* v, u32 count);    // TriangleList
    void DrawLines(Aether::RHI::RHICommandEncoder* enc);
    void DrawTris(Aether::RHI::RHICommandEncoder* enc);

private:
    bool CreatePipelines(Aether::RHI::Format colorFormat, Aether::RHI::Format depthFormat);
    bool UploadMesh(RefPtr<Aether::RHI::RHIBuffer>& vb, RefPtr<Aether::RHI::RHIBuffer>& ib,
                    const Array<Vertex>& verts, const Array<u32>& indices);
    bool EnsureDynamic(RefPtr<Aether::RHI::RHIBuffer>& buf, u64 vertCap);

    Aether::RHI::RHIDevice* mDevice = nullptr;
    Aether::RHI::Format mDepthFormat = Aether::RHI::Format::Undefined;
    RefPtr<Aether::RHI::RHIBuffer> mUniformBuffer;
    RefPtr<Aether::RHI::RHIBindGroup> mBindGroup;

    RefPtr<Aether::RHI::RHIRenderPipeline> mTerrainPipeline;
    RefPtr<Aether::RHI::RHIRenderPipeline> mWaterPipeline;
    RefPtr<Aether::RHI::RHIRenderPipeline> mAtmospherePipeline;
    RefPtr<Aether::RHI::RHIRenderPipeline> mLinesPipeline;
    RefPtr<Aether::RHI::RHIRenderPipeline> mTrisPipeline;

    RefPtr<Aether::RHI::RHIBuffer> mTerrainVB, mTerrainIB;
    u32 mTerrainIdxCount = 0;
    RefPtr<Aether::RHI::RHIBuffer> mWaterVB, mWaterIB;
    u32 mWaterIdxCount = 0;
    RefPtr<Aether::RHI::RHIBuffer> mAtmoVB, mAtmoIB;
    u32 mAtmoIdxCount = 0;

    RefPtr<Aether::RHI::RHIBuffer> mLineVB;
    u32 mLineVertCount = 0;
    RefPtr<Aether::RHI::RHIBuffer> mTriVB;
    u32 mTriVertCount = 0;
};

} // namespace DSP
