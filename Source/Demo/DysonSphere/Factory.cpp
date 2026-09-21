#include "Factory.h"
#include "Math/Math.h"

#include <cmath>

namespace DSP {

using namespace Aether;
using namespace Aether::Math;

const char* ItemName(ItemKind kind)
{
    switch (kind)
    {
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

Color ItemColor(ItemKind kind)
{
    switch (kind)
    {
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

const char* BuildingName(BuildingKind kind)
{
    switch (kind)
    {
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

f32 BuildingPowerDemandKW(BuildingKind kind)
{
    switch (kind)
    {
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

const Recipe* GetRecipeFor(ItemKind item)
{
    static const HashMap<ItemKind, Recipe> RECIPES = {
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
    const Recipe* recipe = RECIPES.Find(item);
    return recipe;
}

FactorySystem::FactorySystem() = default;

u32 FactorySystem::PlaceBuilding(BuildingKind kind, const GridPos& gpos, const Vec3& wpos, f32 rot, ItemKind recipe)
{
    Building b;
    b.Id = mNextId++;
    b.Kind = kind;
    b.TileKey = gpos.Key();
    b.WorldPos = wpos;
    b.Rotation = rot;
    b.CurrentRecipe = recipe;

    if (kind == BuildingKind::PlanetaryLogisticsStation || kind == BuildingKind::InterstellarLogisticsStation)
    {
        b.Channels[0] = {ItemKind::IronIngot, 200, 5000, true};
        b.Channels[1] = {ItemKind::CopperIngot, 200, 5000, true};
        b.Channels[2] = {ItemKind::MagneticCoil, 150, 5000, true};
        b.Channels[3] = {ItemKind::CircuitBoard, 150, 5000, true};
        b.Channels[4] = {ItemKind::Processor, 80, 5000, false};
    }

    mBuildings.Add(b);
    return b.Id;
}

bool FactorySystem::RemoveBuilding(u32 id, Planet* planet)
{
    for (u32 i = 0; i < mBuildings.Count(); ++i)
    {
        if (mBuildings[i].Id == id)
        {
            if (planet) planet->Grid().GetTile(mBuildings[i].TileKey).BuildingId = 0;
            mBuildings.RemoveAt(i);
            return true;
        }
    }
    return false;
}

Building* FactorySystem::GetBuilding(u32 id)
{
    for (Building& b : mBuildings) if (b.Id == id) return &b;
    return nullptr;
}

Building* FactorySystem::BuildingAtTile(u32 tileKey)
{
    for (Building& b : mBuildings) if (b.TileKey == tileKey) return &b;
    return nullptr;
}

f32 FactorySystem::GetProductionRate(ItemKind item) const
{
    const f32* rate = mStatsProduced.Find(item);
    return rate ? *rate : 0.0f;
}

f32 FactorySystem::GetConsumptionRate(ItemKind item) const
{
    const f32* rate = mStatsConsumed.Find(item);
    return rate ? *rate : 0.0f;
}

u32 FactorySystem::TotalStored(ItemKind item) const
{
    u32 sum = 0;
    for (const Building& b : mBuildings)
    {
        if (const u32* inCount = b.InventoryIn.Find(item)) sum += *inCount;
        if (const u32* outCount = b.InventoryOut.Find(item)) sum += *outCount;
        if (b.Kind == BuildingKind::ConveyorBelt)
            for (const BeltItem& bi : b.BeltItems) if (bi.Kind == item) sum++;
        if (b.SorterHeldItem.Kind == item) sum++;
        if (b.Kind == BuildingKind::PlanetaryLogisticsStation || b.Kind == BuildingKind::InterstellarLogisticsStation)
            for (const Building::Channel& ch : b.Channels) if (ch.Item == item) sum += ch.Count;
    }
    return sum;
}

void FactorySystem::ConsumeSailsForSwarm(u32 count)
{
    if (mLaunchedSailsTotal >= count) mLaunchedSailsTotal -= count;
}

void FactorySystem::ConsumeRocketsForSphere(u32 count)
{
    if (mLaunchedRocketsTotal >= count) mLaunchedRocketsTotal -= count;
}

void FactorySystem::Update(f32 dt, Planet* planet, f32 powerSatisfaction)
{
    f32 powerFactor = Math::Clamp(powerSatisfaction, 0.0f, 1.0f);

    mStatSampleTimer += dt;
    if (mStatSampleTimer >= 2.0f)
    {
        mStatSampleTimer = 0.0f;
        for (auto& entry : mStatsProduced) entry.Value *= 0.85f;
        for (auto& entry : mStatsConsumed) entry.Value *= 0.85f;
    }

    for (Building& b : mBuildings)
    {
        b.AnimTimer += dt;
        switch (b.Kind)
        {
            case BuildingKind::MiningMachine:
            case BuildingKind::OilExtractor:
                UpdateMiners(b, dt, planet, powerFactor);
                break;
            case BuildingKind::ArcSmelter:
            case BuildingKind::AssemblingMachine:
            case BuildingKind::ChemicalPlant:
                UpdateSmeltersAssemblers(b, dt, powerFactor);
                break;
            case BuildingKind::ConveyorBelt:
                UpdateBelts(b, dt, planet);
                break;
            case BuildingKind::Sorter:
                UpdateSorters(b, dt, planet, powerFactor);
                break;
            case BuildingKind::MatrixLab:
                UpdateMatrixLabs(b, dt, powerFactor);
                break;
            case BuildingKind::EMRailEjector:
            case BuildingKind::VerticalLaunchSilo:
                UpdateLaunchers(b, dt, powerFactor);
                break;
            case BuildingKind::PlanetaryLogisticsStation:
            case BuildingKind::InterstellarLogisticsStation:
                UpdateLogisticsStations(b, dt);
                break;
            default:
                break;
        }
    }

    UpdateShips(dt);
}

void FactorySystem::UpdateMiners(Building& b, f32 dt, Planet* planet, f32 powerFactor)
{
    if (powerFactor < 0.1f || !planet) return;

    // 采矿机只开采所在瓦片的矿脉 (抽油机开采原油)
    const Tile& t = planet->Grid().GetTile(b.TileKey);
    ResourceKind rk = (ResourceKind)t.Resource;
    if (b.Kind == BuildingKind::OilExtractor)
    {
        if (rk != ResourceKind::CrudeOil) return;
    }
    else
    {
        if (rk == ResourceKind::None || rk == ResourceKind::CrudeOil) return;
    }
    if (t.ResourceAmount == 0) return;

    static const ItemKind RESOURCE_ITEMS[] = {
        ItemKind::None, ItemKind::IronOre, ItemKind::CopperOre, ItemKind::Coal,
        ItemKind::Stone, ItemKind::TitaniumOre, ItemKind::SiliconOre, ItemKind::CrudeOil
    };
    ItemKind outItem = RESOURCE_ITEMS[(int)rk];

    b.Progress += dt * powerFactor * 1.2f;
    if (b.Progress >= 1.0f)
    {
        b.Progress -= 1.0f;
        ResourceKind drained;
        if (planet->MineVein(b.TileKey, 60, drained))
        {
            if (b.InventoryOut[outItem] < 50)
            {
                b.InventoryOut[outItem]++;
                mStatsProduced[outItem] += 30.0f;
            }
        }
    }
}

void FactorySystem::UpdateSmeltersAssemblers(Building& b, f32 dt, f32 powerFactor)
{
    if (powerFactor < 0.1f) return;
    const Recipe* r = GetRecipeFor(b.CurrentRecipe);
    if (!r) return;

    bool hasInputs = true;
    for (const RecipeInput& in : r->Inputs)
    {
        if (b.InventoryIn[in.Item] < in.Count) { hasInputs = false; break; }
    }

    if (hasInputs && b.InventoryOut[r->Output] < 100)
    {
        b.Progress += (dt * powerFactor) / r->CraftTime;
        if (b.Progress >= 1.0f)
        {
            b.Progress -= 1.0f;
            for (const RecipeInput& in : r->Inputs)
            {
                b.InventoryIn[in.Item] -= in.Count;
                mStatsConsumed[in.Item] += (f32)in.Count * 30.0f;
            }
            b.InventoryOut[r->Output] += r->OutputCount;
            mStatsProduced[r->Output] += (f32)r->OutputCount * 30.0f;
        }
    }
    b.Powered = powerFactor > 0.1f;
}

void FactorySystem::UpdateBelts(Building& b, f32 dt, Planet* planet)
{
    f32 beltSpeed = 0.9f; // 每带段行进时间 ~1.1s

    for (BeltItem& it : b.BeltItems) it.Progress += dt * beltSpeed;

    if (!b.BeltItems.IsEmpty() && b.BeltItems.First().Progress >= 1.0f)
    {
        // 找下游: 朝向格点上的建筑
        Building* next = nullptr;
        if (planet)
        {
            u32 nk = PlanetGrid::NeighborKey(b.TileKey, b.Dir);
            next = BuildingAtTile(nk);
        }
        ItemKind doneKind = b.BeltItems.First().Kind;
        bool transferred = false;
        if (next)
        {
            if (next->Kind == BuildingKind::ConveyorBelt)
            {
                if (next->BeltItems.Count() < Building::BELT_CAPACITY &&
                    (next->BeltItems.IsEmpty() || next->BeltItems.Last().Progress > 0.15f))
                {
                    next->BeltItems.Add({doneKind, 0.0f});
                    transferred = true;
                }
            }
            else
            {
                transferred = FeedMachine(*next, doneKind, 1);
            }
        }
        if (transferred)
        {
            b.BeltItems.RemoveAt(0);
        }
        else
        {
            b.BeltItems.First().Progress = 1.0f; // 阻塞等待
        }
    }
    // 保持队列间距
    for (u32 i = 1; i < b.BeltItems.Count(); i++)
    {
        b.BeltItems[i].Progress = Math::Min(b.BeltItems[i].Progress, b.BeltItems[i - 1].Progress - 0.26f);
    }
}

bool FactorySystem::FeedMachine(Building& target, ItemKind item, u32 count)
{
    if (target.Kind == BuildingKind::ConveyorBelt)
    {
        if (target.BeltItems.Count() < Building::BELT_CAPACITY)
        {
            target.BeltItems.Add({item, 0.0f});
            return true;
        }
        return false;
    }
    // 物流站进 supply 通道
    if (target.Kind == BuildingKind::PlanetaryLogisticsStation ||
        target.Kind == BuildingKind::InterstellarLogisticsStation)
    {
        for (Building::Channel& ch : target.Channels)
        {
            if (ch.Item == item && ch.Count + count <= ch.MaxCapacity)
            {
                ch.Count += count;
                return true;
            }
        }
        for (Building::Channel& ch : target.Channels)
        {
            if (ch.Item == ItemKind::None)
            {
                ch.Item = item;
                ch.IsSupply = true;
                ch.Count = count;
                return true;
            }
        }
        return false;
    }
    // 机器: 只收配方所需原料
    const Recipe* r = GetRecipeFor(target.CurrentRecipe);
    if (!r) return false;
    for (const RecipeInput& in : r->Inputs)
    {
        if (in.Item == item && target.InventoryIn[item] + count <= 40)
        {
            target.InventoryIn[item] += count;
            return true;
        }
    }
    return false;
}

void FactorySystem::UpdateSorters(Building& b, f32 dt, Planet* planet, f32 powerFactor)
{
    if (powerFactor < 0.1f) return;
    Building* src = planet ? BuildingAtTile(b.SrcKey) : nullptr;
    Building* dst = planet ? BuildingAtTile(b.DstKey) : nullptr;

    f32 grabSpeed = 2.2f * powerFactor;
    if (b.SorterHeldItem.Kind == ItemKind::None)
    {
        b.SorterArmProgress = Math::Max(0.0f, b.SorterArmProgress - dt * grabSpeed);
        if (b.SorterArmProgress <= 0.05f && src)
        {
            if (src->Kind == BuildingKind::ConveyorBelt)
            {
                if (!src->BeltItems.IsEmpty() &&
                    (b.SorterFilter == ItemKind::None || src->BeltItems.First().Kind == b.SorterFilter))
                {
                    b.SorterHeldItem = src->BeltItems.First();
                    src->BeltItems.RemoveAt(0);
                }
            }
            else
            {
                for (auto& entry : src->InventoryOut)
                {
                    if (entry.Value > 0 && (b.SorterFilter == ItemKind::None || b.SorterFilter == entry.Key))
                    {
                        entry.Value--;
                        b.SorterHeldItem = {entry.Key, 0.0f};
                        break;
                    }
                }
            }
        }
    }
    else
    {
        b.SorterArmProgress = Math::Min(1.0f, b.SorterArmProgress + dt * grabSpeed);
        if (b.SorterArmProgress >= 0.95f && dst)
        {
            if (FeedMachine(*dst, b.SorterHeldItem.Kind, 1))
            {
                b.SorterHeldItem.Kind = ItemKind::None;
            }
        }
    }
}

void FactorySystem::UpdateMatrixLabs(Building& b, f32 dt, f32 powerFactor)
{
    if (powerFactor < 0.1f) return;
    const Recipe* r = GetRecipeFor(b.CurrentRecipe);
    if (!r) return;

    f32 rateMult = (f32)b.StackLevel;
    bool canCraft = true;
    for (const RecipeInput& in : r->Inputs)
    {
        if (b.InventoryIn[in.Item] < in.Count) { canCraft = false; break; }
    }
    if (canCraft && b.InventoryOut[r->Output] < 50 * b.StackLevel)
    {
        b.Progress += (dt * powerFactor * rateMult) / r->CraftTime;
        if (b.Progress >= 1.0f)
        {
            b.Progress -= 1.0f;
            for (const RecipeInput& in : r->Inputs)
            {
                b.InventoryIn[in.Item] -= in.Count;
                mStatsConsumed[in.Item] += (f32)in.Count * 30.0f;
            }
            b.InventoryOut[r->Output] += r->OutputCount;
            mStatsProduced[r->Output] += (f32)r->OutputCount * rateMult * 30.0f;
        }
    }
    b.Powered = powerFactor > 0.1f;
}

void FactorySystem::UpdateLaunchers(Building& b, f32 dt, f32 powerFactor)
{
    if (powerFactor < 0.5f) return;
    if (b.Kind == BuildingKind::EMRailEjector)
    {
        b.Progress += dt * 0.4f * powerFactor;
        if (b.Progress >= 1.0f)
        {
            b.Progress -= 1.0f;
            mLaunchedSailsTotal++;
            mStatsConsumed[ItemKind::SolarSail] += 24.0f;
        }
    }
    else if (b.Kind == BuildingKind::VerticalLaunchSilo)
    {
        b.Progress += dt * 0.2f * powerFactor;
        if (b.Progress >= 1.0f)
        {
            b.Progress -= 1.0f;
            mLaunchedRocketsTotal++;
            mStatsConsumed[ItemKind::SmallCarrierRocket] += 12.0f;
        }
    }
}

void FactorySystem::UpdateLogisticsStations(Building& b, f32 dt)
{
    (void)dt;
    if (mShips.Count() < 16)
    {
        for (Building::Channel& ch : b.Channels)
        {
            if (ch.IsSupply && ch.Count >= 50)
            {
                ch.Count -= 50;
                LogisticsShip ship;
                ship.FromPos = b.WorldPos;
                ship.CurrentPos = b.WorldPos;
                ship.ToPos = {b.WorldPos.x + 80.0f, b.WorldPos.y + 40.0f, b.WorldPos.z - 60.0f};
                ship.CargoKind = ch.Item;
                ship.CargoCount = 50;
                ship.IsInterstellar = (b.Kind == BuildingKind::InterstellarLogisticsStation);
                ship.Progress = 0.0f;
                ship.Speed = ship.IsInterstellar ? 120.0f : 45.0f;
                mShips.Add(ship);
                break;
            }
        }
    }
}

void FactorySystem::UpdateShips(f32 dt)
{
    for (u32 i = 0; i < mShips.Count();)
    {
        LogisticsShip& ship = mShips[i];
        ship.Progress += dt * 0.15f;
        f32 t = ship.Progress;
        if (t < 1.0f)
        {
            ship.CurrentPos = {
                ship.FromPos.x + (ship.ToPos.x - ship.FromPos.x) * t,
                ship.FromPos.y + (ship.ToPos.y - ship.FromPos.y) * t + sinf(t * Math::PI) * 35.0f,
                ship.FromPos.z + (ship.ToPos.z - ship.FromPos.z) * t
            };
            ++i;
        }
        else
        {
            mShips.RemoveAt(i);
        }
    }
}

} // namespace DSP
