#pragma once

#include "core/platform.h"
#include "core/math.h"
#include "demo/dyson_sphere/planet.h"
#include <vector>
#include <string>
#include <map>

namespace dsp {

using namespace aether;

enum class ItemKind {
    None = 0,
    // 原矿
    IronOre, CopperOre, Coal, Stone, TitaniumOre, SiliconOre, CrudeOil,
    // 中间产物
    IronIngot, CopperIngot, Magnet, MagneticCoil, CircuitBoard, Gear, Steel,
    TitaniumIngot, HighPuritySilicon, Processor, Engine, Thruster, Plastic,
    SulfuricAcid, Hydrogen, Deuterium, Antimatter,
    // 巨构部件
    SolarSail, SmallCarrierRocket, SpaceWarper,
    // 科研矩阵
    MatrixBlue, MatrixRed, MatrixYellow, MatrixPurple, MatrixGreen, MatrixWhite,
    Count
};

const char* item_name(ItemKind kind);
Color item_color(ItemKind kind);
// 物品在物品图集 (8x8) 中的格子索引 = (int)kind

enum class BuildingKind {
    None = 0,
    ConveyorBelt, Sorter,
    MiningMachine, OilExtractor, ArcSmelter, AssemblingMachine, ChemicalPlant, MatrixLab,
    PlanetaryLogisticsStation, InterstellarLogisticsStation,
    EMRailEjector, VerticalLaunchSilo, RayReceiver, ArtificialStar,
    TeslaTower, WirelessPowerTower, WindTurbine, SolarPanel, ThermalPowerPlant, FusionPowerPlant,
    Count
};

const char* building_name(BuildingKind kind);
f32 building_power_demand_kw(BuildingKind kind);
// 建筑在建筑图集 (6x6) 中的格子索引 = (int)kind - 1

struct Recipe {
    ItemKind output = ItemKind::None;
    u32 output_count = 1;
    f32 craft_time = 1.0f;
    std::vector<std::pair<ItemKind, u32>> inputs;
};

const Recipe* get_recipe_for(ItemKind item);

struct BeltItem {
    ItemKind kind = ItemKind::None;
    f32 progress = 0.0f; // [0, 1] 沿带位置
};

struct Building {
    u32 id = 0;
    BuildingKind kind = BuildingKind::None;
    u32 tile_key = 0;        // 所在瓦片
    Vec3 world_pos{0.0f, 0.0f, 0.0f};
    f32 rotation = 0.0f;     // 朝向弧度 (切平面)
    i32 dir = 0;             // 朝向格点 0=+u 1=-u 2=+v 3=-v (传送带/分拣器使用)
    i32 stack_level = 1;

    // 生产
    ItemKind current_recipe = ItemKind::None;
    f32 progress = 0.0f;
    std::map<ItemKind, u32> inventory_in;
    std::map<ItemKind, u32> inventory_out;

    // 传送带
    std::vector<BeltItem> belt_items;
    static constexpr size_t kBeltCapacity = 4;

    // 分拣器
    u32 src_key = 0;
    u32 dst_key = 0;
    ItemKind sorter_filter = ItemKind::None;
    BeltItem sorter_held_item;
    f32 sorter_arm_progress = 0.0f;

    // 物流站
    struct Channel {
        ItemKind item = ItemKind::None;
        u32 count = 0;
        u32 max_capacity = 5000;
        bool is_supply = true;
    };
    Channel channels[5];
    u32 drone_count = 10;
    u32 vessel_count = 5;

    // 状态
    bool powered = true;
    bool active = true;
    f32 anim_timer = 0.0f;
};

struct LogisticsShip {
    Vec3 from_pos;
    Vec3 to_pos;
    Vec3 current_pos;
    ItemKind cargo_kind = ItemKind::None;
    u32 cargo_count = 0;
    bool is_interstellar = false;
    f32 progress = 0.0f;
    f32 speed = 40.0f;
};

class FactorySystem {
public:
    FactorySystem();

    void update(f32 dt, Planet* planet, f32 power_satisfaction);

    // 放置 (调用方保证瓦片合法); 自动登记瓦片占用
    u32 place_building(BuildingKind kind, const GridPos& gpos, const Vec3& wpos, f32 rot = 0.0f, ItemKind recipe = ItemKind::None);
    // 拆除并释放瓦片占用
    bool remove_building(u32 id, Planet* planet);
    Building* get_building(u32 id);
    Building* building_at_tile(u32 tile_key);
    const std::vector<Building>& buildings() const { return buildings_; }
    std::vector<Building>& buildings() { return buildings_; }

    const std::vector<LogisticsShip>& active_ships() const { return ships_; }

    f32 get_production_rate(ItemKind item) const;
    f32 get_consumption_rate(ItemKind item) const;

    u32 launched_solar_sails() const { return launched_sails_total_; }
    u32 launched_carrier_rockets() const { return launched_rockets_total_; }
    void consume_sails_for_swarm(u32 count);
    void consume_rockets_for_sphere(u32 count);

    // 全工厂库存汇总 (用于资源栏)
    u32 total_stored(ItemKind item) const;

private:
    void update_miners(Building& b, f32 dt, Planet* planet, f32 power_factor);
    void update_smelters_assemblers(Building& b, f32 dt, f32 power_factor);
    void update_belts(Building& b, f32 dt, Planet* planet);
    void update_sorters(Building& b, f32 dt, Planet* planet, f32 power_factor);
    void update_matrix_labs(Building& b, f32 dt, f32 power_factor);
    void update_launchers(Building& b, f32 dt, f32 power_factor);
    void update_logistics_stations(Building& b, f32 dt);
    void update_ships(f32 dt);
    bool feed_machine(Building& target, ItemKind item, u32 count);

    u32 next_id_ = 1;
    std::vector<Building> buildings_;
    std::vector<LogisticsShip> ships_;

    std::map<ItemKind, f32> stats_produced_;
    std::map<ItemKind, f32> stats_consumed_;
    f32 stat_sample_timer_ = 0.0f;

    u32 launched_sails_total_ = 0;
    u32 launched_rockets_total_ = 0;
};

} // namespace dsp
