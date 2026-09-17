#pragma once

#include "core/platform.h"
#include "core/math.h"
#include <vector>

namespace dsp {

using namespace aether;

struct SolarSailParticle {
    Vec3 pos{0.0f, 0.0f, 0.0f};
    Vec3 vel{0.0f, 0.0f, 0.0f};
    f32 orbit_radius = 1200.0f;
    f32 angle = 0.0f;
    f32 inclination = 0.35f;
    f32 life = 1200.0f; // Seconds remaining
    f32 max_life = 1200.0f;
    bool absorbed = false;
};

struct DysonNode {
    u32 id = 0;
    Vec3 pos{0.0f, 0.0f, 0.0f}; // Position on Dyson Sphere shell
    u32 rockets_invested = 0;
    u32 rockets_required = 30;
    bool completed = false;
    f32 energy_output_mw = 96.0f;
};

struct DysonStrut {
    u32 node_a = 0;
    u32 node_b = 0;
    f32 progress = 0.0f;
    bool completed = false;
};

struct DysonShellPanel {
    u32 node_indices[3]; // Triangle cell on geodesic sphere
    u32 sails_absorbed = 0;
    u32 sails_required = 120;
    f32 fill_ratio = 0.0f;
    bool completed = false;
};

class DysonSphereManager {
public:
    DysonSphereManager();

    void update(f32 dt, u32 new_launched_sails, u32 new_launched_rockets);

    // Swarm
    u32 sail_count() const { return static_cast<u32>(sails_.size()); }
    f32 swarm_generation_gw() const { return swarm_gen_gw_; }
    const std::vector<SolarSailParticle>& sails() const { return sails_; }

    // Shell
    u32 node_count() const { return static_cast<u32>(nodes_.size()); }
    u32 completed_nodes() const;
    f32 shell_generation_gw() const { return shell_gen_gw_; }
    f32 total_generation_gw() const { return swarm_gen_gw_ + shell_gen_gw_; }

    const std::vector<DysonNode>& nodes() const { return nodes_; }
    const std::vector<DysonStrut>& struts() const { return struts_; }
    const std::vector<DysonShellPanel>& panels() const { return panels_; }

    f32 sphere_radius() const { return sphere_radius_; }

private:
    void init_geodesic_shell();
    void spawn_sails(u32 count);
    void invest_rockets(u32 count);

    f32 sphere_radius_ = 1400.0f;
    std::vector<SolarSailParticle> sails_;
    std::vector<DysonNode> nodes_;
    std::vector<DysonStrut> struts_;
    std::vector<DysonShellPanel> panels_;

    f32 swarm_gen_gw_ = 0.0f;
    f32 shell_gen_gw_ = 0.0f;
};

}
