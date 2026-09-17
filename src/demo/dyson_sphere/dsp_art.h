#pragma once

// ============================================================================
// dsp_art — 程序化美术资产
// 启动时一次性生成全部贴图图集: 星球地形 / 建筑 / 物品图标 / 矿脉晶体 /
// 机甲 / 特效 / 星云 / 行星云层 / 镜头光晕, 供渲染层直接取 UV 使用。
// ============================================================================

#include "core/platform.h"
#include "core/math.h"
#include "rhi/rhi.h"
#include <memory>

namespace dsp_art {

using namespace aether;

bool init(rhi::RHIDevice* device);
void shutdown();

// 图集与单元格 UV (idx 为枚举值)
const std::shared_ptr<rhi::RHITexture>& tex_terrain();   // 4x4 cells @128px, idx = Terrain
const std::shared_ptr<rhi::RHITexture>& tex_buildings(); // 6x6 cells @128px, idx = BuildingKind
const std::shared_ptr<rhi::RHITexture>& tex_items();     // 8x8 cells @32px,  idx = ItemKind
const std::shared_ptr<rhi::RHITexture>& tex_veins();     // 8x1 cells @64px,  idx = ResourceKind-1
const std::shared_ptr<rhi::RHITexture>& tex_mecha();     // 单张 128x128
const std::shared_ptr<rhi::RHITexture>& tex_fx();        // 4x2 cells @64px: 0太阳帆 1火箭 2无人机 3货船 4能量点 5火花
const std::shared_ptr<rhi::RHITexture>& tex_nebula();    // 512x512 云气 (染色+叠加混合)
const std::shared_ptr<rhi::RHITexture>& tex_clouds();    // 512x256 行星云层
const std::shared_ptr<rhi::RHITexture>& tex_flare();     // 256x64 镜头光晕横纹
const std::shared_ptr<rhi::RHITexture>& tex_star4();     // 128x128 四芒星光斑

void cell_uv(int cols, int rows, int idx, Vec2& uv0, Vec2& uv1);

} // namespace dsp_art
