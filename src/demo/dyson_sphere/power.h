#pragma once

#include "core/platform.h"
#include "core/math.h"
#include "demo/dyson_sphere/factory.h"
#include <vector>

namespace dsp {

using namespace aether;

struct PowerNode {
    u32 building_id = 0;
    BuildingKind kind = BuildingKind::None;
    Vec3 pos{0.0f, 0.0f, 0.0f};
    f32 supply_radius = 12.0f;
    f32 connect_radius = 50.0f;
    f32 gen_capacity_kw = 0.0f;
    f32 demand_kw = 0.0f;
};

class PowerGrid {
public:
    PowerGrid();

    void update(f32 dt, const std::vector<Building>& buildings, const Vec3& mecha_pos,
                f32& mecha_energy_mj, f32 mecha_max_energy_mj, const Vec3& sun_dir, f32 time);

    f32 total_generation_kw() const { return total_gen_kw_; }
    f32 total_demand_kw() const { return total_demand_kw_; }
    f32 satisfaction_ratio() const { return satisfaction_; }
    f32 accumulator_charge_mj() const { return accum_charge_mj_; }
    f32 accumulator_max_mj() const { return accum_max_mj_; }
    bool is_mecha_charging() const { return mecha_charging_; }

    const std::vector<PowerNode>& nodes() const { return nodes_; }

private:
    std::vector<PowerNode> nodes_;
    f32 total_gen_kw_ = 0.0f;
    f32 total_demand_kw_ = 0.0f;
    f32 satisfaction_ = 1.0f;
    f32 accum_charge_mj_ = 200.0f;
    f32 accum_max_mj_ = 500.0f;
    bool mecha_charging_ = false;
};

}
