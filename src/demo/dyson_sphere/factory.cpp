#include "demo/dyson_sphere/factory.h"
#include <cmath>
#include <algorithm>

namespace dsp {

const char* item_name(ItemKind kind) {
    switch (kind) {
        case ItemKind::IronOre: return "铁矿石";
        case ItemKind::CopperOre: return "铜矿石";
        case ItemKind::Coal: return "煤炭";
        case ItemKind::Stone: return "石材";
        case ItemKind::TitaniumOre: return "钛矿石";
        case ItemKind::SiliconOre: return "硅矿石";
        case ItemKind::CrudeOil: return "原油";
        case ItemKind::IronIngot: return "铁块";
        case ItemKind::CopperIngot: return "铜块";
        case ItemKind::Magnet: return "磁铁";
        case ItemKind::MagneticCoil: return "电磁线圈";
        case ItemKind::CircuitBoard: return "电路板";
        case ItemKind::Gear: return "齿轮";
        case ItemKind::Steel: return "钢材";
        case ItemKind::TitaniumIngot: return "钛块";
        case ItemKind::HighPuritySilicon: return "高纯硅";
        case ItemKind::Processor: return "处理器";
        case ItemKind::Engine: return "电动机";
        case ItemKind::Thruster: return "推进器";
        case ItemKind::Plastic: return "塑料";
        case ItemKind::SulfuricAcid: return "硫酸";
        case ItemKind::Hydrogen: return "氢";
        case ItemKind::Deuterium: return "氘";
        case ItemKind::Antimatter: return "反物质";
        case ItemKind::SolarSail: return "太阳帆";
        case ItemKind::SmallCarrierRocket: return "小型运载火箭";
        case ItemKind::SpaceWarper: return "空间翘曲器";
        case ItemKind::MatrixBlue: return "电磁矩阵";
        case ItemKind::MatrixRed: return "能量矩阵";
        case ItemKind::MatrixYellow: return "结构矩阵";
        case ItemKind::MatrixPurple: return "信息矩阵";
        case ItemKind::MatrixGreen: return "引力矩阵";
        case ItemKind::MatrixWhite: return "宇宙矩阵";
        default: return "无";
    }
}

Color item_color(ItemKind kind) {
    switch (kind) {
        case ItemKind::IronOre: return Color{0.38f, 0.48f, 0.58f, 1.0f};
        case ItemKind::CopperOre: return Color{0.85f, 0.52f, 0.28f, 1.0f};
        case ItemKind::Coal: return Color{0.18f, 0.18f, 0.20f, 1.0f};
        case ItemKind::Stone: return Color{0.65f, 0.65f, 0.65f, 1.0f};
        case ItemKind::TitaniumOre: return Color{0.80f, 0.82f, 0.90f, 1.0f};
        case ItemKind::SiliconOre: return Color{0.60f, 0.70f, 0.75f, 1.0f};
        case ItemKind::CrudeOil: return Color{0.12f, 0.10f, 0.15f, 1.0f};
        case ItemKind::IronIngot: return Color{0.70f, 0.75f, 0.85f, 1.0f};
        case ItemKind::CopperIngot: return Color{0.92f, 0.58f, 0.32f, 1.0f};
        case ItemKind::Magnet: return Color{0.45f, 0.55f, 0.70f, 1.0f};
        case ItemKind::MagneticCoil: return Color{0.25f, 0.80f, 0.95f, 1.0f};
        case ItemKind::CircuitBoard: return Color{0.20f, 0.85f, 0.40f, 1.0f};
        case ItemKind::Gear: return Color{0.80f, 0.75f, 0.65f, 1.0f};
        case ItemKind::Steel: return Color{0.55f, 0.58f, 0.64f, 1.0f};
        case ItemKind::TitaniumIngot: return Color{0.85f, 0.90f, 1.0f, 1.0f};
        case ItemKind::HighPuritySilicon: return Color{0.45f, 0.80f, 0.90f, 1.0f};
        case ItemKind::Processor: return Color{0.20f, 0.95f, 0.85f, 1.0f};
        case ItemKind::Engine: return Color{0.90f, 0.75f, 0.20f, 1.0f};
        case ItemKind::Thruster: return Color{0.95f, 0.45f, 0.20f, 1.0f};
        case ItemKind::Plastic: return Color{0.95f, 0.95f, 0.90f, 1.0f};
        case ItemKind::SulfuricAcid: return Color{0.85f, 0.90f, 0.15f, 1.0f};
        case ItemKind::Hydrogen: return Color{0.90f, 0.30f, 0.40f, 1.0f};
        case ItemKind::Deuterium: return Color{0.30f, 0.90f, 0.70f, 1.0f};
        case ItemKind::Antimatter: return Color{0.95f, 0.95f, 0.95f, 1.0f};
        case ItemKind::SolarSail: return Color{1.0f, 0.85f, 0.25f, 1.0f};
        case ItemKind::SmallCarrierRocket: return Color{0.90f, 0.95f, 1.0f, 1.0f};
        case ItemKind::SpaceWarper: return Color{0.65f, 0.30f, 1.0f, 1.0f};
        case ItemKind::MatrixBlue: return Color{0.15f, 0.60f, 1.0f, 1.0f};
        case ItemKind::MatrixRed: return Color{1.0f, 0.25f, 0.20f, 1.0f};
        case ItemKind::MatrixYellow: return Color{1.0f, 0.85f, 0.15f, 1.0f};
        case ItemKind::MatrixPurple: return Color{0.85f, 0.25f, 1.0f, 1.0f};
        case ItemKind::MatrixGreen: return Color{0.20f, 0.95f, 0.45f, 1.0f};
        case ItemKind::MatrixWhite: return Color{0.98f, 0.98f, 1.0f, 1.0f};
        default: return Color{1.0f, 1.0f, 1.0f, 1.0f};
    }
}

const char* building_name(BuildingKind kind) {
    switch (kind) {
        case BuildingKind::ConveyorBelt: return "传送带";
        case BuildingKind::Sorter: return "分拣器";
        case BuildingKind::MiningMachine: return "采矿机";
        case BuildingKind::OilExtractor: return "抽油机";
        case BuildingKind::ArcSmelter: return "电弧熔炉";
        case BuildingKind::AssemblingMachine: return "组装机";
        case BuildingKind::ChemicalPlant: return "化工厂";
        case BuildingKind::MatrixLab: return "矩阵实验室";
        case BuildingKind::PlanetaryLogisticsStation: return "行星物流站";
        case BuildingKind::InterstellarLogisticsStation: return "星际物流站";
        case BuildingKind::EMRailEjector: return "电磁弹射器";
        case BuildingKind::VerticalLaunchSilo: return "垂直发射井";
        case BuildingKind::RayReceiver: return "射线接收站";
        case BuildingKind::ArtificialStar: return "人造恒星";
        case BuildingKind::TeslaTower: return "电浆中继塔";
        case BuildingKind::WirelessPowerTower: return "无线输电塔";
        case BuildingKind::WindTurbine: return "风力涡轮机";
        case BuildingKind::SolarPanel: return "太阳能板";
        case BuildingKind::ThermalPowerPlant: return "火力发电厂";
        case BuildingKind::FusionPowerPlant: return "微型聚变电站";
        default: return "未知建筑";
    }
}

f32 building_power_demand_kw(BuildingKind kind) {
    switch (kind) {
        case BuildingKind::MiningMachine: return 420.0f;
        case BuildingKind::OilExtractor: return 840.0f;
        case BuildingKind::ArcSmelter: return 360.0f;
        case BuildingKind::AssemblingMachine: return 270.0f;
        case BuildingKind::ChemicalPlant: return 720.0f;
        case BuildingKind::MatrixLab: return 480.0f;
        case BuildingKind::PlanetaryLogisticsStation: return 600.0f;
        case BuildingKind::InterstellarLogisticsStation: return 1200.0f;
        case BuildingKind::EMRailEjector: return 1200.0f;
        case BuildingKind::VerticalLaunchSilo: return 1800.0f;
        case BuildingKind::Sorter: return 18.0f;
        default: return 0.0f;
    }
}

const Recipe* get_recipe_for(ItemKind item) {
    static const std::map<ItemKind, Recipe> kRecipes = {
        {ItemKind::IronIngot, {ItemKind::IronIngot, 1, 1.0f, {{ItemKind::IronOre, 1}}}},
        {ItemKind::CopperIngot, {ItemKind::CopperIngot, 1, 1.0f, {{ItemKind::CopperOre, 1}}}},
        {ItemKind::Magnet, {ItemKind::Magnet, 1, 1.5f, {{ItemKind::IronOre, 1}}}},
        {ItemKind::MagneticCoil, {ItemKind::MagneticCoil, 2, 1.0f, {{ItemKind::Magnet, 2}, {ItemKind::CopperIngot, 1}}}},
        {ItemKind::Gear, {ItemKind::Gear, 1, 1.0f, {{ItemKind::IronIngot, 1}}}},
        {ItemKind::CircuitBoard, {ItemKind::CircuitBoard, 2, 1.0f, {{ItemKind::IronIngot, 2}, {ItemKind::CopperIngot, 1}}}},
        {ItemKind::Steel, {ItemKind::Steel, 1, 3.0f, {{ItemKind::IronIngot, 3}}}},
        {ItemKind::TitaniumIngot, {ItemKind::TitaniumIngot, 1, 2.0f, {{ItemKind::TitaniumOre, 2}}}},
        {ItemKind::HighPuritySilicon, {ItemKind::HighPuritySilicon, 1, 2.0f, {{ItemKind::SiliconOre, 2}}}},
        {ItemKind::Processor, {ItemKind::Processor, 1, 3.0f, {{ItemKind::CircuitBoard, 2}, {ItemKind::HighPuritySilicon, 2}}}},
        {ItemKind::Engine, {ItemKind::Engine, 1, 2.0f, {{ItemKind::IronIngot, 2}, {ItemKind::Gear, 1}, {ItemKind::MagneticCoil, 1}}}},
        {ItemKind::Thruster, {ItemKind::Thruster, 1, 4.0f, {{ItemKind::Steel, 2}, {ItemKind::Processor, 1}}}},
        {ItemKind::SolarSail, {ItemKind::SolarSail, 2, 4.0f, {{ItemKind::HighPuritySilicon, 1}, {ItemKind::CircuitBoard, 1}}}},
        {ItemKind::SmallCarrierRocket, {ItemKind::SmallCarrierRocket, 1, 6.0f, {{ItemKind::TitaniumIngot, 2}, {ItemKind::Processor, 2}, {ItemKind::Thruster, 1}}}},
        {ItemKind::SpaceWarper, {ItemKind::SpaceWarper, 1, 8.0f, {{ItemKind::Processor, 1}, {ItemKind::TitaniumIngot, 1}}}},
        {ItemKind::MatrixBlue, {ItemKind::MatrixBlue, 1, 3.0f, {{ItemKind::MagneticCoil, 1}, {ItemKind::CircuitBoard, 1}}}},
        {ItemKind::MatrixRed, {ItemKind::MatrixRed, 1, 6.0f, {{ItemKind::Hydrogen, 2}, {ItemKind::Coal, 2}}}},
        {ItemKind::MatrixYellow, {ItemKind::MatrixYellow, 1, 8.0f, {{ItemKind::TitaniumIngot, 1}, {ItemKind::HighPuritySilicon, 1}}}},
        {ItemKind::MatrixPurple, {ItemKind::MatrixPurple, 1, 10.0f, {{ItemKind::Processor, 2}, {ItemKind::Plastic, 1}}}},
        {ItemKind::MatrixGreen, {ItemKind::MatrixGreen, 1, 12.0f, {{ItemKind::Deuterium, 2}, {ItemKind::SpaceWarper, 1}}}},
        {ItemKind::MatrixWhite, {ItemKind::MatrixWhite, 1, 15.0f, {{ItemKind::MatrixBlue, 1}, {ItemKind::MatrixRed, 1}, {ItemKind::MatrixYellow, 1}, {ItemKind::MatrixPurple, 1}, {ItemKind::MatrixGreen, 1}, {ItemKind::Antimatter, 1}}}},
    };
    auto it = kRecipes.find(item);
    return it != kRecipes.end() ? &it->second : nullptr;
}

FactorySystem::FactorySystem() = default;

u32 FactorySystem::place_building(BuildingKind kind, const GridPos& gpos, const Vec3& wpos, f32 rot, ItemKind recipe) {
    Building b;
    b.id = next_id_++;
    b.kind = kind;
    b.tile_key = gpos.key();
    b.world_pos = wpos;
    b.rotation = rot;
    b.current_recipe = recipe;

    if (kind == BuildingKind::PlanetaryLogisticsStation || kind == BuildingKind::InterstellarLogisticsStation) {
        b.channels[0] = {ItemKind::IronIngot, 200, 5000, true};
        b.channels[1] = {ItemKind::CopperIngot, 200, 5000, true};
        b.channels[2] = {ItemKind::MagneticCoil, 150, 5000, true};
        b.channels[3] = {ItemKind::CircuitBoard, 150, 5000, true};
        b.channels[4] = {ItemKind::Processor, 80, 5000, false};
    }

    buildings_.push_back(b);
    return b.id;
}

bool FactorySystem::remove_building(u32 id, Planet* planet) {
    for (auto it = buildings_.begin(); it != buildings_.end(); ++it) {
        if (it->id == id) {
            if (planet) planet->grid().tile(it->tile_key).building_id = 0;
            buildings_.erase(it);
            return true;
        }
    }
    return false;
}

Building* FactorySystem::get_building(u32 id) {
    for (auto& b : buildings_) if (b.id == id) return &b;
    return nullptr;
}

Building* FactorySystem::building_at_tile(u32 tile_key) {
    for (auto& b : buildings_) if (b.tile_key == tile_key) return &b;
    return nullptr;
}

f32 FactorySystem::get_production_rate(ItemKind item) const {
    auto it = stats_produced_.find(item);
    return it != stats_produced_.end() ? it->second : 0.0f;
}

f32 FactorySystem::get_consumption_rate(ItemKind item) const {
    auto it = stats_consumed_.find(item);
    return it != stats_consumed_.end() ? it->second : 0.0f;
}

u32 FactorySystem::total_stored(ItemKind item) const {
    u32 sum = 0;
    for (const auto& b : buildings_) {
        auto i = b.inventory_in.find(item);
        if (i != b.inventory_in.end()) sum += i->second;
        auto o = b.inventory_out.find(item);
        if (o != b.inventory_out.end()) sum += o->second;
        if (b.kind == BuildingKind::ConveyorBelt)
            for (const auto& bi : b.belt_items) if (bi.kind == item) sum++;
        if (b.sorter_held_item.kind == item) sum++;
        if (b.kind == BuildingKind::PlanetaryLogisticsStation || b.kind == BuildingKind::InterstellarLogisticsStation)
            for (const auto& ch : b.channels) if (ch.item == item) sum += ch.count;
    }
    return sum;
}

void FactorySystem::consume_sails_for_swarm(u32 count) {
    if (launched_sails_total_ >= count) launched_sails_total_ -= count;
}

void FactorySystem::consume_rockets_for_sphere(u32 count) {
    if (launched_rockets_total_ >= count) launched_rockets_total_ -= count;
}

void FactorySystem::update(f32 dt, Planet* planet, f32 power_satisfaction) {
    f32 power_factor = std::clamp(power_satisfaction, 0.0f, 1.0f);

    stat_sample_timer_ += dt;
    if (stat_sample_timer_ >= 2.0f) {
        stat_sample_timer_ = 0.0f;
        for (auto& [k, v] : stats_produced_) v *= 0.85f;
        for (auto& [k, v] : stats_consumed_) v *= 0.85f;
    }

    for (auto& b : buildings_) {
        b.anim_timer += dt;
        switch (b.kind) {
            case BuildingKind::MiningMachine:
            case BuildingKind::OilExtractor:
                update_miners(b, dt, planet, power_factor);
                break;
            case BuildingKind::ArcSmelter:
            case BuildingKind::AssemblingMachine:
            case BuildingKind::ChemicalPlant:
                update_smelters_assemblers(b, dt, power_factor);
                break;
            case BuildingKind::ConveyorBelt:
                update_belts(b, dt, planet);
                break;
            case BuildingKind::Sorter:
                update_sorters(b, dt, planet, power_factor);
                break;
            case BuildingKind::MatrixLab:
                update_matrix_labs(b, dt, power_factor);
                break;
            case BuildingKind::EMRailEjector:
            case BuildingKind::VerticalLaunchSilo:
                update_launchers(b, dt, power_factor);
                break;
            case BuildingKind::PlanetaryLogisticsStation:
            case BuildingKind::InterstellarLogisticsStation:
                update_logistics_stations(b, dt);
                break;
            default:
                break;
        }
    }

    update_ships(dt);
}

void FactorySystem::update_miners(Building& b, f32 dt, Planet* planet, f32 power_factor) {
    if (power_factor < 0.1f || !planet) return;

    // 采矿机只开采所在瓦片的矿脉 (抽油机开采原油)
    const Tile& t = planet->grid().tile(b.tile_key);
    ResourceKind rk = (ResourceKind)t.resource;
    if (b.kind == BuildingKind::OilExtractor) {
        if (rk != ResourceKind::CrudeOil) return;
    } else {
        if (rk == ResourceKind::None || rk == ResourceKind::CrudeOil) return;
    }
    if (t.resource_amount == 0) return;

    static const ItemKind kMap[] = {
        ItemKind::None, ItemKind::IronOre, ItemKind::CopperOre, ItemKind::Coal,
        ItemKind::Stone, ItemKind::TitaniumOre, ItemKind::SiliconOre, ItemKind::CrudeOil
    };
    ItemKind out_item = kMap[(int)rk];

    b.progress += dt * power_factor * 1.2f;
    if (b.progress >= 1.0f) {
        b.progress -= 1.0f;
        ResourceKind drained;
        if (planet->mine_vein(b.tile_key, 60, drained)) {
            if (b.inventory_out[out_item] < 50) {
                b.inventory_out[out_item]++;
                stats_produced_[out_item] += 30.0f;
            }
        }
    }
}

void FactorySystem::update_smelters_assemblers(Building& b, f32 dt, f32 power_factor) {
    if (power_factor < 0.1f) return;
    const Recipe* r = get_recipe_for(b.current_recipe);
    if (!r) return;

    bool has_inputs = true;
    for (const auto& [in_item, count] : r->inputs) {
        if (b.inventory_in[in_item] < count) { has_inputs = false; break; }
    }

    if (has_inputs && b.inventory_out[r->output] < 100) {
        b.progress += (dt * power_factor) / r->craft_time;
        if (b.progress >= 1.0f) {
            b.progress -= 1.0f;
            for (const auto& [in_item, count] : r->inputs) {
                b.inventory_in[in_item] -= count;
                stats_consumed_[in_item] += (f32)count * 30.0f;
            }
            b.inventory_out[r->output] += r->output_count;
            stats_produced_[r->output] += (f32)r->output_count * 30.0f;
        }
    }
    b.powered = power_factor > 0.1f;
}

void FactorySystem::update_belts(Building& b, f32 dt, Planet* planet) {
    f32 belt_speed = 0.9f; // 每带段行进时间 ~1.1s

    for (auto& it : b.belt_items) it.progress += dt * belt_speed;

    if (!b.belt_items.empty() && b.belt_items.front().progress >= 1.0f) {
        // 找下游: 朝向格点上的建筑
        Building* next = nullptr;
        if (planet) {
            u32 nk = PlanetGrid::neighbor_key(b.tile_key, b.dir);
            next = building_at_tile(nk);
        }
        ItemKind done_kind = b.belt_items.front().kind;
        bool transferred = false;
        if (next) {
            if (next->kind == BuildingKind::ConveyorBelt) {
                if (next->belt_items.size() < Building::kBeltCapacity &&
                    (next->belt_items.empty() || next->belt_items.back().progress > 0.15f)) {
                    next->belt_items.push_back({done_kind, 0.0f});
                    transferred = true;
                }
            } else {
                transferred = feed_machine(*next, done_kind, 1);
            }
        }
        if (transferred) {
            b.belt_items.erase(b.belt_items.begin());
        } else {
            b.belt_items.front().progress = 1.0f; // 阻塞等待
        }
    }
    // 保持队列间距
    for (size_t i = 1; i < b.belt_items.size(); i++) {
        b.belt_items[i].progress = std::min(b.belt_items[i].progress, b.belt_items[i - 1].progress - 0.26f);
    }
}

bool FactorySystem::feed_machine(Building& target, ItemKind item, u32 count) {
    if (target.kind == BuildingKind::ConveyorBelt) {
        if (target.belt_items.size() < Building::kBeltCapacity) {
            target.belt_items.push_back({item, 0.0f});
            return true;
        }
        return false;
    }
    // 物流站进 supply 通道
    if (target.kind == BuildingKind::PlanetaryLogisticsStation ||
        target.kind == BuildingKind::InterstellarLogisticsStation) {
        for (auto& ch : target.channels) {
            if (ch.item == item && ch.count + count <= ch.max_capacity) {
                ch.count += count;
                return true;
            }
        }
        for (auto& ch : target.channels) {
            if (ch.item == ItemKind::None) {
                ch.item = item;
                ch.is_supply = true;
                ch.count = count;
                return true;
            }
        }
        return false;
    }
    // 机器: 只收配方所需原料
    const Recipe* r = get_recipe_for(target.current_recipe);
    if (!r) return false;
    for (const auto& [in_item, c] : r->inputs) {
        if (in_item == item && target.inventory_in[item] + count <= 40) {
            target.inventory_in[item] += count;
            return true;
        }
    }
    return false;
}

void FactorySystem::update_sorters(Building& b, f32 dt, Planet* planet, f32 power_factor) {
    if (power_factor < 0.1f) return;
    Building* src = planet ? building_at_tile(b.src_key) : nullptr;
    Building* dst = planet ? building_at_tile(b.dst_key) : nullptr;

    f32 grab_speed = 2.2f * power_factor;
    if (b.sorter_held_item.kind == ItemKind::None) {
        b.sorter_arm_progress = std::max(0.0f, b.sorter_arm_progress - dt * grab_speed);
        if (b.sorter_arm_progress <= 0.05f && src) {
            if (src->kind == BuildingKind::ConveyorBelt) {
                if (!src->belt_items.empty() &&
                    (b.sorter_filter == ItemKind::None || src->belt_items.front().kind == b.sorter_filter)) {
                    b.sorter_held_item = src->belt_items.front();
                    src->belt_items.erase(src->belt_items.begin());
                }
            } else {
                for (auto& [kind, count] : src->inventory_out) {
                    if (count > 0 && (b.sorter_filter == ItemKind::None || b.sorter_filter == kind)) {
                        count--;
                        b.sorter_held_item = {kind, 0.0f};
                        break;
                    }
                }
            }
        }
    } else {
        b.sorter_arm_progress = std::min(1.0f, b.sorter_arm_progress + dt * grab_speed);
        if (b.sorter_arm_progress >= 0.95f && dst) {
            if (feed_machine(*dst, b.sorter_held_item.kind, 1)) {
                b.sorter_held_item.kind = ItemKind::None;
            }
        }
    }
}

void FactorySystem::update_matrix_labs(Building& b, f32 dt, f32 power_factor) {
    if (power_factor < 0.1f) return;
    const Recipe* r = get_recipe_for(b.current_recipe);
    if (!r) return;

    f32 rate_mult = (f32)b.stack_level;
    bool can_craft = true;
    for (const auto& [in_k, count] : r->inputs) {
        if (b.inventory_in[in_k] < count) { can_craft = false; break; }
    }
    if (can_craft && b.inventory_out[r->output] < 50 * b.stack_level) {
        b.progress += (dt * power_factor * rate_mult) / r->craft_time;
        if (b.progress >= 1.0f) {
            b.progress -= 1.0f;
            for (const auto& [in_k, count] : r->inputs) {
                b.inventory_in[in_k] -= count;
                stats_consumed_[in_k] += (f32)count * 30.0f;
            }
            b.inventory_out[r->output] += r->output_count;
            stats_produced_[r->output] += (f32)r->output_count * rate_mult * 30.0f;
        }
    }
    b.powered = power_factor > 0.1f;
}

void FactorySystem::update_launchers(Building& b, f32 dt, f32 power_factor) {
    if (power_factor < 0.5f) return;
    if (b.kind == BuildingKind::EMRailEjector) {
        b.progress += dt * 0.4f * power_factor;
        if (b.progress >= 1.0f) {
            b.progress -= 1.0f;
            launched_sails_total_++;
            stats_consumed_[ItemKind::SolarSail] += 24.0f;
        }
    } else if (b.kind == BuildingKind::VerticalLaunchSilo) {
        b.progress += dt * 0.2f * power_factor;
        if (b.progress >= 1.0f) {
            b.progress -= 1.0f;
            launched_rockets_total_++;
            stats_consumed_[ItemKind::SmallCarrierRocket] += 12.0f;
        }
    }
}

void FactorySystem::update_logistics_stations(Building& b, f32 dt) {
    (void)dt;
    if (ships_.size() < 16) {
        for (auto& ch : b.channels) {
            if (ch.is_supply && ch.count >= 50) {
                ch.count -= 50;
                LogisticsShip ship;
                ship.from_pos = b.world_pos;
                ship.current_pos = b.world_pos;
                ship.to_pos = {b.world_pos.x + 80.0f, b.world_pos.y + 40.0f, b.world_pos.z - 60.0f};
                ship.cargo_kind = ch.item;
                ship.cargo_count = 50;
                ship.is_interstellar = (b.kind == BuildingKind::InterstellarLogisticsStation);
                ship.progress = 0.0f;
                ship.speed = ship.is_interstellar ? 120.0f : 45.0f;
                ships_.push_back(ship);
                break;
            }
        }
    }
}

void FactorySystem::update_ships(f32 dt) {
    for (auto it = ships_.begin(); it != ships_.end(); ) {
        it->progress += dt * 0.15f;
        f32 t = it->progress;
        if (t < 1.0f) {
            it->current_pos = {
                it->from_pos.x + (it->to_pos.x - it->from_pos.x) * t,
                it->from_pos.y + (it->to_pos.y - it->from_pos.y) * t + sinf(t * 3.14159f) * 35.0f,
                it->from_pos.z + (it->to_pos.z - it->from_pos.z) * t
            };
            ++it;
        } else {
            it = ships_.erase(it);
        }
    }
}

} // namespace dsp
