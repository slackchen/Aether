#include "Blueprint.h"
#include "Math/Math.h"

namespace DSP {

using namespace Aether;
using namespace Aether::Math;

BlueprintManager::BlueprintManager()
{
    InitDefaultPresets();
}

void BlueprintManager::InitDefaultPresets()
{
    // 1) 四联熔炼列: 4 台电弧熔炉 + 进料带 + 出料带
    {
        Blueprint bp;
        bp.Name = "四联熔炼列";
        bp.Desc = "采矿进料 -> 4台电弧熔炉 -> 汇流出料";
        for (int i = 0; i < 4; i++)
        {
            bp.Items.Add({BuildingKind::ConveyorBelt, i, 0, 0});                 // 进料带 →
            bp.Items.Add({BuildingKind::ArcSmelter, i, 1, 0, ItemKind::IronIngot});
            bp.Items.Add({BuildingKind::ConveyorBelt, i, 2, 0});                 // 出料带 →
        }
        mPresets.Add(bp);
    }
    // 2) 线圈工坊: 熔炉产磁铁, 组装机产电磁线圈
    {
        Blueprint bp;
        bp.Name = "线圈工坊";
        bp.Desc = "磁铁与电磁线圈并行产线";
        for (int i = 0; i < 3; i++)
        {
            bp.Items.Add({BuildingKind::ConveyorBelt, i, 0, 0});
            bp.Items.Add({BuildingKind::ArcSmelter, i, 1, 0, ItemKind::Magnet});
        }
        for (int i = 0; i < 3; i++)
        {
            bp.Items.Add({BuildingKind::AssemblingMachine, i, 2, 0, ItemKind::MagneticCoil});
            bp.Items.Add({BuildingKind::ConveyorBelt, i, 3, 0});
        }
        mPresets.Add(bp);
    }
    // 3) 太阳能田 3x3
    {
        Blueprint bp;
        bp.Name = "太阳能田";
        bp.Desc = "3x3 光伏阵列 + 中央无线输电塔";
        for (int dx = -1; dx <= 1; dx++)
            for (int dy = -1; dy <= 1; dy++)
            {
                if (dx == 0 && dy == 0) bp.Items.Add({BuildingKind::WirelessPowerTower, 0, 0, 0});
                else bp.Items.Add({BuildingKind::SolarPanel, dx, dy, 0});
            }
        mPresets.Add(bp);
    }
    // 4) 矩阵研究塔: 实验室 x4 (蓝矩阵)
    {
        Blueprint bp;
        bp.Name = "矩阵研究塔";
        bp.Desc = "4座蓝矩阵实验室";
        for (int i = 0; i < 4; i++)
            bp.Items.Add({BuildingKind::MatrixLab, i % 2, i / 2, 0, ItemKind::MatrixBlue});
        mPresets.Add(bp);
    }
    // 5) 电力枢纽: 风电 + 电塔
    {
        Blueprint bp;
        bp.Name = "电力枢纽";
        bp.Desc = "4台风电 + 无线输电塔";
        bp.Items.Add({BuildingKind::WirelessPowerTower, 0, 0, 0});
        bp.Items.Add({BuildingKind::WindTurbine, -2, 0, 0});
        bp.Items.Add({BuildingKind::WindTurbine, 2, 0, 0});
        bp.Items.Add({BuildingKind::WindTurbine, 0, -2, 0});
        bp.Items.Add({BuildingKind::WindTurbine, 0, 2, 0});
        mPresets.Add(bp);
    }
}

u32 BlueprintManager::PasteBlueprint(const Blueprint& bp, u32 originKey, Planet* planet,
                                     FactorySystem& factory, Mecha& mecha)
{
    if (!planet) return 0;
    TileCoord oc = PlanetGrid::Coord(originKey);
    const PlanetGrid& grid = planet->Grid();
    constexpr i32 N = (i32)PlanetGrid::TILES_PER_FACE;

    u32 placed = 0;
    for (const BlueprintItem& item : bp.Items)
    {
        i32 u = Math::Clamp(oc.U + item.Dx, 0, N - 1);
        i32 v = Math::Clamp(oc.V + item.Dy, 0, N - 1);
        u32 key = PlanetGrid::Key(oc.Face, (u8)u, (u8)v);
        if (!planet->IsBuildableTile(key)) continue;
        if (item.Kind == BuildingKind::MiningMachine && !grid.GetTile(key).Resource) continue;

        GridPos gp = GridPos::FromKey(key);
        Vec3 pos = grid.TileCenter(planet->Radius(), key);
        f32 rot = (f32)item.Dir * (Math::PI * 0.5f);
        u32 id = factory.PlaceBuilding(item.Kind, gp, pos, rot, item.Recipe);
        if (id != 0)
        {
            planet->Grid().GetTile(key).BuildingId = (u16)id;
            Building* b = factory.GetBuilding(id);
            if (b) b->Dir = item.Dir;
            if (item.Kind == BuildingKind::Sorter && b)
            {
                b->SrcKey = PlanetGrid::NeighborKey(key, 1);
                b->DstKey = PlanetGrid::NeighborKey(key, 0);
            }
            mecha.DispatchDrone(pos, id);
            placed++;
        }
    }
    return placed;
}

} // namespace DSP
