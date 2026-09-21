#pragma once

#include "Core.h"
#include "Factory.h"
#include "Container/Array.h"
#include "Container/String.h"
#include "Container/HashMap.h"

namespace DSP {

using Aether::f32;
using Aether::u32;
using Aether::Array;
using Aether::String;
using Aether::HashMap;

enum class TechId {
    BasicAutomation = 0,
    ElectromagneticMetallurgy,
    ThermalPowerGeneration,
    PlasmaRefining,
    TitaniumSiliconMetallurgy,
    PlanetaryLogistics,
    InterplanetaryLogistics,
    MiniaturizedFusion,
    EMRailSolarSwarm,
    VerticalRocketLaunch,
    DysonSphereStressSystem,
    ArtificialStarAntimatter,
    UniverseCosmology,
    Count
};

struct TechNode {
    TechId Id = TechId::BasicAutomation;
    String Name;
    String Desc;
    Array<TechId> Prerequisites;
    HashMap<ItemKind, u32> RequiredMatrices;
    u32 HashesInvested = 0;
    u32 HashesRequired = 600;
    bool Unlocked = false;
};

class TechTreeManager {
public:
    TechTreeManager();

    void Update(f32 dt, Array<Building>& buildings);

    void SelectResearch(TechId id);
    TechId CurrentResearch() const { return mCurrentTech; }
    const TechNode* GetTech(TechId id) const;
    bool IsUnlocked(TechId id) const;
    bool CanResearch(TechId id) const;

    f32 CurrentProgressRatio() const;
    const Array<TechNode>& AllTechs() const { return mTechs; }

    bool HasNewUnlockEvent(TechId& outId);

private:
    void InitTree();

    Array<TechNode> mTechs;
    TechId mCurrentTech = TechId::BasicAutomation;
    bool mNewUnlock = false;
    TechId mLastUnlocked = TechId::BasicAutomation;
};

}
