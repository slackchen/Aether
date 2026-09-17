#include "demo/dyson_sphere/blueprint.h"
#include <algorithm>

namespace dsp {

BlueprintManager::BlueprintManager() {
    init_default_presets();
}

void BlueprintManager::init_default_presets() {
    // 1) 四联熔炼列: 4 台电弧熔炉 + 进料带 + 出料带
    {
        Blueprint bp;
        bp.name = "四联熔炼列";
        bp.desc = "采矿进料 -> 4台电弧熔炉 -> 汇流出料";
        for (int i = 0; i < 4; i++) {
            bp.items.push_back({BuildingKind::ConveyorBelt, i, 0, 0});                 // 进料带 →
            bp.items.push_back({BuildingKind::ArcSmelter, i, 1, 0, ItemKind::IronIngot});
            bp.items.push_back({BuildingKind::ConveyorBelt, i, 2, 0});                 // 出料带 →
        }
        presets_.push_back(bp);
    }
    // 2) 线圈工坊: 熔炉产磁铁, 组装机产电磁线圈
    {
        Blueprint bp;
        bp.name = "线圈工坊";
        bp.desc = "磁铁与电磁线圈并行产线";
        for (int i = 0; i < 3; i++) {
            bp.items.push_back({BuildingKind::ConveyorBelt, i, 0, 0});
            bp.items.push_back({BuildingKind::ArcSmelter, i, 1, 0, ItemKind::Magnet});
        }
        for (int i = 0; i < 3; i++) {
            bp.items.push_back({BuildingKind::AssemblingMachine, i, 2, 0, ItemKind::MagneticCoil});
            bp.items.push_back({BuildingKind::ConveyorBelt, i, 3, 0});
        }
        presets_.push_back(bp);
    }
    // 3) 太阳能田 3x3
    {
        Blueprint bp;
        bp.name = "太阳能田";
        bp.desc = "3x3 光伏阵列 + 中央无线输电塔";
        for (int dx = -1; dx <= 1; dx++)
            for (int dy = -1; dy <= 1; dy++) {
                if (dx == 0 && dy == 0) bp.items.push_back({BuildingKind::WirelessPowerTower, 0, 0, 0});
                else bp.items.push_back({BuildingKind::SolarPanel, dx, dy, 0});
            }
        presets_.push_back(bp);
    }
    // 4) 矩阵研究塔: 实验室 x4 (蓝矩阵)
    {
        Blueprint bp;
        bp.name = "矩阵研究塔";
        bp.desc = "4座蓝矩阵实验室";
        for (int i = 0; i < 4; i++)
            bp.items.push_back({BuildingKind::MatrixLab, i % 2, i / 2, 0, ItemKind::MatrixBlue});
        presets_.push_back(bp);
    }
    // 5) 电力枢纽: 风电 + 电塔
    {
        Blueprint bp;
        bp.name = "电力枢纽";
        bp.desc = "4台风电 + 无线输电塔";
        bp.items.push_back({BuildingKind::WirelessPowerTower, 0, 0, 0});
        bp.items.push_back({BuildingKind::WindTurbine, -2, 0, 0});
        bp.items.push_back({BuildingKind::WindTurbine, 2, 0, 0});
        bp.items.push_back({BuildingKind::WindTurbine, 0, -2, 0});
        bp.items.push_back({BuildingKind::WindTurbine, 0, 2, 0});
        presets_.push_back(bp);
    }
}

u32 BlueprintManager::paste_blueprint(const Blueprint& bp, u32 origin_key, Planet* planet,
                                      FactorySystem& factory, Mecha& mecha) {
    if (!planet) return 0;
    TileCoord oc = PlanetGrid::coord(origin_key);
    const PlanetGrid& grid = planet->grid();
    constexpr i32 N = (i32)PlanetGrid::kTilesPerFace;

    u32 placed = 0;
    for (const auto& item : bp.items) {
        i32 u = std::clamp(oc.u + item.dx, 0, N - 1);
        i32 v = std::clamp(oc.v + item.dy, 0, N - 1);
        u32 key = PlanetGrid::key(oc.face, (u8)u, (u8)v);
        if (!planet->is_buildable_tile(key)) continue;
        if (item.kind == BuildingKind::MiningMachine && !grid.tile(key).resource) continue;

        GridPos gp = GridPos::from_key(key);
        Vec3 pos = grid.tile_center(planet->radius(), key);
        f32 rot = (f32)item.dir * (3.14159265f * 0.5f);
        u32 id = factory.place_building(item.kind, gp, pos, rot, item.recipe);
        if (id != 0) {
            planet->grid().tile(key).building_id = (u16)id;
            Building* b = factory.get_building(id);
            if (b) b->dir = item.dir;
            if (item.kind == BuildingKind::Sorter && b) {
                b->src_key = PlanetGrid::neighbor_key(key, 1);
                b->dst_key = PlanetGrid::neighbor_key(key, 0);
            }
            mecha.dispatch_drone(pos, id);
            placed++;
        }
    }
    return placed;
}

} // namespace dsp
