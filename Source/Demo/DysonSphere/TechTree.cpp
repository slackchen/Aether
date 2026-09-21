#include "TechTree.h"

namespace DSP {

using namespace Aether;

TechTreeManager::TechTreeManager()
{
    InitTree();
}

void TechTreeManager::InitTree()
{
    mTechs.Clear();

    auto add = [this](TechId id, const String& name, const String& desc,
                      const Array<TechId>& prereqs,
                      const HashMap<ItemKind, u32>& matrices, u32 hashes, bool startUnlocked = false) {
        TechNode t;
        t.Id = id;
        t.Name = name;
        t.Desc = desc;
        t.Prerequisites = prereqs;
        t.RequiredMatrices = matrices;
        t.HashesRequired = hashes;
        t.HashesInvested = startUnlocked ? hashes : 0;
        t.Unlocked = startUnlocked;
        mTechs.Add(t);
    };

    add(TechId::BasicAutomation, "基础自动化", "解锁 传送带 Mk.1 与 分拣器 Mk.1",
        {}, {{ItemKind::MatrixBlue, 20}}, 400, true);

    add(TechId::ElectromagneticMetallurgy, "电磁冶金", "解锁 弧光熔炉 与 磁线圈",
        {TechId::BasicAutomation}, {{ItemKind::MatrixBlue, 40}}, 800, true);

    add(TechId::ThermalPowerGeneration, "火力发电技术", "解锁 火力发电机 与 煤炭燃烧发电",
        {TechId::ElectromagneticMetallurgy}, {{ItemKind::MatrixBlue, 60}}, 1200);

    add(TechId::PlasmaRefining, "等离子精炼", "解锁 能量矩阵(红方块) 与 原油炼油厂",
        {TechId::ThermalPowerGeneration}, {{ItemKind::MatrixBlue, 100}, {ItemKind::MatrixRed, 100}}, 2400);

    add(TechId::TitaniumSiliconMetallurgy, "钛硅提炼技术", "解锁 高纯硅 与 钛合金精炼加工",
        {TechId::PlasmaRefining}, {{ItemKind::MatrixBlue, 150}, {ItemKind::MatrixRed, 150}}, 3600);

    add(TechId::PlanetaryLogistics, "行星物流系统 (PLS)", "解锁 行星物流运输站 与 物流配送无人机",
        {TechId::TitaniumSiliconMetallurgy}, {{ItemKind::MatrixBlue, 200}, {ItemKind::MatrixRed, 200}, {ItemKind::MatrixYellow, 100}}, 4800);

    add(TechId::InterplanetaryLogistics, "星际物流系统 (ILS)", "解锁 星际物流运输站 与 曲率星际运输船",
        {TechId::PlanetaryLogistics}, {{ItemKind::MatrixBlue, 300}, {ItemKind::MatrixRed, 300}, {ItemKind::MatrixYellow, 200}}, 6000);

    add(TechId::MiniaturizedFusion, "微型聚变发电", "解锁 氘核燃料棒 与 微型聚变电站 (15 MW)",
        {TechId::InterplanetaryLogistics}, {{ItemKind::MatrixBlue, 400}, {ItemKind::MatrixRed, 400}, {ItemKind::MatrixYellow, 300}, {ItemKind::MatrixPurple, 200}}, 8000);

    add(TechId::EMRailSolarSwarm, "电磁轨道弹射系统", "向恒星高轨弹射太阳帆，组建在轨戴森云",
        {TechId::MiniaturizedFusion}, {{ItemKind::MatrixBlue, 500}, {ItemKind::MatrixRed, 500}, {ItemKind::MatrixYellow, 400}, {ItemKind::MatrixPurple, 300}}, 10000);

    add(TechId::VerticalRocketLaunch, "垂直发射井与运载火箭", "向深空发射小型运载火箭，组装戴森球测地线骨架",
        {TechId::EMRailSolarSwarm}, {{ItemKind::MatrixBlue, 600}, {ItemKind::MatrixRed, 600}, {ItemKind::MatrixYellow, 500}, {ItemKind::MatrixPurple, 400}, {ItemKind::MatrixGreen, 200}}, 15000);

    add(TechId::DysonSphereStressSystem, "戴森球应力系统", "解锁 戴森壳测地线框架 与 射线接收站",
        {TechId::VerticalRocketLaunch}, {{ItemKind::MatrixBlue, 800}, {ItemKind::MatrixRed, 800}, {ItemKind::MatrixYellow, 600}, {ItemKind::MatrixPurple, 500}, {ItemKind::MatrixGreen, 400}}, 20000);

    add(TechId::ArtificialStarAntimatter, "人造恒星与反物质湮灭", "解锁 人造恒星 (72 MW) 与 反物质燃料棒",
        {TechId::DysonSphereStressSystem}, {{ItemKind::MatrixGreen, 800}, {ItemKind::MatrixWhite, 400}}, 30000);

    add(TechId::UniverseCosmology, "宇宙矩阵与终极认知", "使命达成：彻底掌握戴森球宇宙终极能源体系",
        {TechId::ArtificialStarAntimatter}, {{ItemKind::MatrixWhite, 2000}}, 50000);
}

const TechNode* TechTreeManager::GetTech(TechId id) const
{
    for (const TechNode& t : mTechs)
    {
        if (t.Id == id) return &t;
    }
    return nullptr;
}

bool TechTreeManager::IsUnlocked(TechId id) const
{
    const TechNode* t = GetTech(id);
    return t ? t->Unlocked : false;
}

bool TechTreeManager::CanResearch(TechId id) const
{
    const TechNode* t = GetTech(id);
    if (!t || t->Unlocked) return false;
    for (TechId pre : t->Prerequisites)
    {
        if (!IsUnlocked(pre)) return false;
    }
    return true;
}

void TechTreeManager::SelectResearch(TechId id)
{
    if (CanResearch(id))
    {
        mCurrentTech = id;
    }
}

f32 TechTreeManager::CurrentProgressRatio() const
{
    const TechNode* t = GetTech(mCurrentTech);
    if (!t || t->HashesRequired == 0) return 0.0f;
    return (f32)t->HashesInvested / (f32)t->HashesRequired;
}

bool TechTreeManager::HasNewUnlockEvent(TechId& outId)
{
    if (mNewUnlock)
    {
        mNewUnlock = false;
        outId = mLastUnlocked;
        return true;
    }
    return false;
}

void TechTreeManager::Update(f32 dt, Array<Building>& buildings)
{
    TechNode* cur = nullptr;
    for (TechNode& t : mTechs)
    {
        if (t.Id == mCurrentTech) { cur = &t; break; }
    }
    if (!cur || cur->Unlocked) return;

    // Aggregate matrix research speed from all MatrixLabs
    u32 labsResearching = 0;
    for (Building& b : buildings)
    {
        if (b.Kind == BuildingKind::MatrixLab && b.CurrentRecipe == ItemKind::None)
        {
            labsResearching += b.StackLevel;
        }
    }

    // Passive baseline + lab speed
    f32 speedHashesSec = 25.0f + (f32)labsResearching * 60.0f;
    cur->HashesInvested += (u32)(speedHashesSec * dt);

    if (cur->HashesInvested >= cur->HashesRequired)
    {
        cur->HashesInvested = cur->HashesRequired;
        cur->Unlocked = true;
        mNewUnlock = true;
        mLastUnlocked = cur->Id;

        // Auto select next available tech
        for (TechNode& t : mTechs)
        {
            if (!t.Unlocked && CanResearch(t.Id))
            {
                mCurrentTech = t.Id;
                break;
            }
        }
    }
}

}
