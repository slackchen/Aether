#pragma once

// ============================================================================
// dsp_art — 程序化美术资产
// 启动时一次性生成全部贴图图集: 星球地形 / 建筑 / 物品图标 / 矿脉晶体 /
// 机甲 / 特效 / 星云 / 行星云层 / 镜头光晕, 供渲染层直接取 UV 使用。
// ============================================================================

#include "Core.h"
#include "Math/Vec2.h"
#include "RHI.h"
#include "Container/RefPtr.h"

namespace DSPArt {

using Aether::f32;
using Aether::i32;
using Aether::u32;
using Aether::Math::Vec2;
using Aether::RefPtr;

bool Init(Aether::RHI::RHIDevice* device);
void Shutdown();

// 图集与单元格 UV (idx 为枚举值)
const RefPtr<Aether::RHI::RHITexture>& TexTerrain();   // 4x4 cells @128px, idx = Terrain
const RefPtr<Aether::RHI::RHITexture>& TexBuildings(); // 6x6 cells @128px, idx = BuildingKind
const RefPtr<Aether::RHI::RHITexture>& TexItems();     // 8x8 cells @32px,  idx = ItemKind
const RefPtr<Aether::RHI::RHITexture>& TexVeins();     // 8x1 cells @64px,  idx = ResourceKind-1
const RefPtr<Aether::RHI::RHITexture>& TexMecha();     // 单张 128x128
const RefPtr<Aether::RHI::RHITexture>& TexFx();        // 4x2 cells @64px: 0太阳帆 1火箭 2无人机 3货船 4能量点 5火花
const RefPtr<Aether::RHI::RHITexture>& TexNebula();    // 512x512 云气 (染色+叠加混合)
const RefPtr<Aether::RHI::RHITexture>& TexClouds();    // 512x256 行星云层
const RefPtr<Aether::RHI::RHITexture>& TexFlare();     // 256x64 镜头光晕横纹
const RefPtr<Aether::RHI::RHITexture>& TexStar4();     // 128x128 四芒星光斑

void CellUv(int cols, int rows, int idx, Vec2& uv0, Vec2& uv1);

} // namespace DSPArt
