#pragma once

#include "core/platform.h"
#include "demo/dyson_sphere/factory.h"
#include <vector>
#include <string>
#include <map>

namespace dsp {

using namespace aether;

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
    TechId id = TechId::BasicAutomation;
    std::string name;
    std::string desc;
    std::vector<TechId> prerequisites;
    std::map<ItemKind, u32> required_matrices;
    u32 hashes_invested = 0;
    u32 hashes_required = 600;
    bool unlocked = false;
};

class TechTreeManager {
public:
    TechTreeManager();

    void update(f32 dt, std::vector<Building>& buildings);

    void select_research(TechId id);
    TechId current_research() const { return current_tech_; }
    const TechNode* get_tech(TechId id) const;
    bool is_unlocked(TechId id) const;
    bool can_research(TechId id) const;

    f32 current_progress_ratio() const;
    const std::vector<TechNode>& all_techs() const { return techs_; }

    bool has_new_unlock_event(TechId& out_id);

private:
    void init_tree();

    std::vector<TechNode> techs_;
    TechId current_tech_ = TechId::BasicAutomation;
    bool new_unlock_ = false;
    TechId last_unlocked_ = TechId::BasicAutomation;
};

}
