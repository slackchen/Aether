#pragma once

#include "core/platform.h"
#include "core/math.h"
#include "demo/dyson_sphere/planet.h"
#include <algorithm>
#include <vector>
#include <string>

namespace dsp {

using namespace aether;

enum class MechaMode {
    GroundWalk,
    LowFlight,
    SpaceSailing,
    Warping,
};

enum class FuelKind {
    None = 0,
    Coal,
    HydrogenFuelRod,
    DeuteronFuelRod,
    AntimatterFuelRod,
};

struct FuelItem {
    FuelKind kind = FuelKind::None;
    u32 count = 0;
    f32 energy_per_item_mj = 0.0f;
    f32 combustion_efficiency = 1.0f;
};

struct ConstructionDrone {
    Vec3 pos{0.0f, 0.0f, 0.0f};
    Vec3 target_pos{0.0f, 0.0f, 0.0f};
    f32 speed = 25.0f;
    bool active = false;
    bool returning = false;
    u32 target_id = 0;
    f32 beam_timer = 0.0f;
};

class Mecha {
public:
    Mecha();

    void update(f32 dt, Planet* current_planet, bool space_mode);

    // Unified 3D Spherical & Flight Movement (方向始终以相机为准, 球面切向移动)
    void move_3d(const Vec2& input_wasd, const Vec3& cam_right, f32 alt_input, f32 dt, Planet* planet);
    void move_ground(const Vec2& dir, f32 dt, Planet* planet);
    void move_flight(const Vec2& dir, f32 alt_input, f32 dt, Planet* planet);
    void move_space(const Vec3& thrust_dir, f32 dt);
    void toggle_flight();
    void toggle_warp();

    // Energy & Fuel
    void consume_energy(f32 amount_mj);
    void add_fuel(FuelKind kind, u32 count);
    f32 energy_mj() const { return energy_mj_; }
    f32 max_energy_mj() const { return max_energy_mj_; }
    f32 energy_ratio() const { return max_energy_mj_ > 0.0f ? energy_mj_ / max_energy_mj_ : 0.0f; }
    f32 power_draw_mw() const { return current_power_draw_mw_; }
    f32 charge_rate_mw() const { return current_charge_rate_mw_; }

    // Drone Dispatch
    void dispatch_drone(const Vec3& target_pos, u32 target_id);
    const std::vector<ConstructionDrone>& drones() const { return drones_; }
    std::vector<ConstructionDrone>& drones() { return drones_; }

    // State & Transform
    MechaMode mode() const { return mode_; }
    Vec3 position() const { return pos_; }
    void set_position(const Vec3& p) { pos_ = p; }
    void set_energy(f32 mj) { energy_mj_ = std::max(0.0f, mj); }
    Vec3 velocity() const { return vel_; }
    f32 heading() const { return heading_; }
    f32 altitude() const { return altitude_; }
    f32 speed() const;
    bool is_low_energy() const { return energy_ratio() < 0.15f; }

    // Visual FX intensities for one-shot takeoff and landing
    f32 thruster_intensity() const { return thruster_burn_; }
    f32 reentry_intensity() const { return reentry_glow_; }

    // Upgrades
    u32 core_level() const { return core_level_; }
    void upgrade_core();
    void upgrade_drones();
    void upgrade_speed();

    // Fuel slot info
    const FuelItem& fuel_slot() const { return fuel_slot_; }

private:
    void update_energy_and_combustion(f32 dt);
    void update_drones(f32 dt);

    Vec3 pos_{0.0f, 502.0f, 0.0f}; // 3D world position on R=500m planet
    Vec3 vel_{0.0f, 0.0f, 0.0f};
    f32 lat_ = 0.0f;
    f32 lon_ = 0.0f;
    f32 altitude_ = 0.0f; // Above surface in meters
    f32 heading_ = 0.0f; // Angle in radians
    f32 thruster_burn_ = 0.0f; // 0..1
    f32 reentry_glow_ = 0.0f;  // 0..1

    MechaMode mode_ = MechaMode::GroundWalk;
    bool warp_active_ = false;

    f32 energy_mj_ = 100.0f;
    f32 max_energy_mj_ = 100.0f;
    f32 current_power_draw_mw_ = 0.0f;
    f32 current_charge_rate_mw_ = 0.0f;

    FuelItem fuel_slot_;

    // Attributes
    u32 core_level_ = 1;
    u32 max_drones_ = 3;
    f32 drone_speed_ = 45.0f;
    f32 walk_speed_ = 24.0f;
    f32 fly_speed_ = 68.0f;
    f32 sail_speed_ = 320.0f;
    f32 warp_speed_ = 5000.0f;

    std::vector<ConstructionDrone> drones_;
};

}
