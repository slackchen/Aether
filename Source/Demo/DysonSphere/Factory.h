#pragma once

#include "Core.h"
#include "Math/Color.h"
#include "Math/Vec3.h"
#include "Planet.h"
#include "Container/Array.h"
#include "Container/HashMap.h"

namespace DSP {

using Aether::f32;
using Aether::i32;
using Aether::u8;
using Aether::u32;
using Aether::Math::Color;
using Aether::Math::Vec3;
using Aether::Array;
using Aether::HashMap;

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

const char* ItemName(ItemKind kind);
Color ItemColor(ItemKind kind);
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

const char* BuildingName(BuildingKind kind);
f32 BuildingPowerDemandKW(BuildingKind kind);
// 建筑在建筑图集 (6x6) 中的格子索引 = (int)kind - 1

struct RecipeInput {
    ItemKind Item = ItemKind::None;
    u32 Count = 0;
};

struct Recipe {
    ItemKind Output = ItemKind::None;
    u32 OutputCount = 1;
    f32 CraftTime = 1.0f;
    Array<RecipeInput> Inputs;
};

const Recipe* GetRecipeFor(ItemKind item);

struct BeltItem {
    ItemKind Kind = ItemKind::None;
    f32 Progress = 0.0f; // [0, 1] 沿带位置
};

struct Building {
    u32 Id = 0;
    BuildingKind Kind = BuildingKind::None;
    u32 TileKey = 0;        // 所在瓦片
    Vec3 WorldPos{0.0f, 0.0f, 0.0f};
    f32 Rotation = 0.0f;     // 朝向弧度 (切平面)
    i32 Dir = 0;             // 朝向格点 0=+u 1=-u 2=+v 3=-v (传送带/分拣器使用)
    i32 StackLevel = 1;

    // 生产
    ItemKind CurrentRecipe = ItemKind::None;
    f32 Progress = 0.0f;
    HashMap<ItemKind, u32> InventoryIn;
    HashMap<ItemKind, u32> InventoryOut;

    // 传送带
    Array<BeltItem> BeltItems;
    static constexpr u32 BELT_CAPACITY = 4;

    // 分拣器
    u32 SrcKey = 0;
    u32 DstKey = 0;
    ItemKind SorterFilter = ItemKind::None;
    BeltItem SorterHeldItem;
    f32 SorterArmProgress = 0.0f;

    // 物流站
    struct Channel {
        ItemKind Item = ItemKind::None;
        u32 Count = 0;
        u32 MaxCapacity = 5000;
        bool IsSupply = true;
    };
    Channel Channels[5];
    u32 DroneCount = 10;
    u32 VesselCount = 5;

    // 状态
    bool Powered = true;
    bool Active = true;
    f32 AnimTimer = 0.0f;
};

struct LogisticsShip {
    Vec3 FromPos;
    Vec3 ToPos;
    Vec3 CurrentPos;
    ItemKind CargoKind = ItemKind::None;
    u32 CargoCount = 0;
    bool IsInterstellar = false;
    f32 Progress = 0.0f;
    f32 Speed = 40.0f;
};

class FactorySystem {
public:
    FactorySystem();

    void Update(f32 dt, Planet* planet, f32 powerSatisfaction);

    // 放置 (调用方保证瓦片合法); 自动登记瓦片占用
    u32 PlaceBuilding(BuildingKind kind, const GridPos& gpos, const Vec3& wpos, f32 rot = 0.0f, ItemKind recipe = ItemKind::None);
    // 拆除并释放瓦片占用
    bool RemoveBuilding(u32 id, Planet* planet);
    Building* GetBuilding(u32 id);
    Building* BuildingAtTile(u32 tileKey);
    const Array<Building>& Buildings() const { return mBuildings; }
    Array<Building>& Buildings() { return mBuildings; }

    const Array<LogisticsShip>& ActiveShips() const { return mShips; }

    f32 GetProductionRate(ItemKind item) const;
    f32 GetConsumptionRate(ItemKind item) const;

    u32 LaunchedSolarSails() const { return mLaunchedSailsTotal; }
    u32 LaunchedCarrierRockets() const { return mLaunchedRocketsTotal; }
    void ConsumeSailsForSwarm(u32 count);
    void ConsumeRocketsForSphere(u32 count);

    // 全工厂库存汇总 (用于资源栏)
    u32 TotalStored(ItemKind item) const;

private:
    void UpdateMiners(Building& b, f32 dt, Planet* planet, f32 powerFactor);
    void UpdateSmeltersAssemblers(Building& b, f32 dt, f32 powerFactor);
    void UpdateBelts(Building& b, f32 dt, Planet* planet);
    void UpdateSorters(Building& b, f32 dt, Planet* planet, f32 powerFactor);
    void UpdateMatrixLabs(Building& b, f32 dt, f32 powerFactor);
    void UpdateLaunchers(Building& b, f32 dt, f32 powerFactor);
    void UpdateLogisticsStations(Building& b, f32 dt);
    void UpdateShips(f32 dt);
    bool FeedMachine(Building& target, ItemKind item, u32 count);

    u32 mNextId = 1;
    Array<Building> mBuildings;
    Array<LogisticsShip> mShips;

    HashMap<ItemKind, f32> mStatsProduced;
    HashMap<ItemKind, f32> mStatsConsumed;
    f32 mStatSampleTimer = 0.0f;

    u32 mLaunchedSailsTotal = 0;
    u32 mLaunchedRocketsTotal = 0;
};

} // namespace DSP
