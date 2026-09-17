#include "demo/dyson_sphere/mecha.h"
#include <cmath>
#include <algorithm>

namespace dsp {

constexpr f32 kPi = 3.14159265358979323846f;
constexpr f32 kTwoPi = kPi * 2.0f;
constexpr f32 kHalfPi = kPi * 0.5f;

Mecha::Mecha() {
    fuel_slot_.kind = FuelKind::HydrogenFuelRod;
    fuel_slot_.count = 40;
    fuel_slot_.energy_per_item_mj = 54.0f;
    fuel_slot_.combustion_efficiency = 1.0f;

    energy_mj_ = max_energy_mj_;
    drones_.resize(max_drones_);
    for (auto& d : drones_) {
        d.pos = pos_;
        d.speed = drone_speed_;
    }
}

f32 Mecha::speed() const {
    return sqrtf(vel_.x * vel_.x + vel_.y * vel_.y + vel_.z * vel_.z);
}

void Mecha::consume_energy(f32 amount_mj) {
    energy_mj_ = std::max(0.0f, energy_mj_ - amount_mj);
}

void Mecha::add_fuel(FuelKind kind, u32 count) {
    if (fuel_slot_.kind == kind) {
        fuel_slot_.count += count;
    } else if (fuel_slot_.count == 0 || fuel_slot_.kind == FuelKind::None) {
        fuel_slot_.kind = kind;
        fuel_slot_.count = count;
        switch (kind) {
            case FuelKind::Coal:
                fuel_slot_.energy_per_item_mj = 2.7f;
                fuel_slot_.combustion_efficiency = 0.8f;
                break;
            case FuelKind::HydrogenFuelRod:
                fuel_slot_.energy_per_item_mj = 54.0f;
                fuel_slot_.combustion_efficiency = 1.0f;
                break;
            case FuelKind::DeuteronFuelRod:
                fuel_slot_.energy_per_item_mj = 600.0f;
                fuel_slot_.combustion_efficiency = 1.5f;
                break;
            case FuelKind::AntimatterFuelRod:
                fuel_slot_.energy_per_item_mj = 7200.0f;
                fuel_slot_.combustion_efficiency = 2.0f;
                break;
            default:
                break;
        }
    }
}

void Mecha::toggle_flight() {
    if (mode_ == MechaMode::GroundWalk && energy_mj_ > 5.0f) {
        mode_ = MechaMode::LowFlight;
        altitude_ = 14.0f;
    } else if (mode_ == MechaMode::LowFlight) {
        mode_ = MechaMode::GroundWalk;
        altitude_ = 0.0f;
    }
}

void Mecha::toggle_warp() {
    if (mode_ == MechaMode::SpaceSailing && energy_mj_ > 40.0f) {
        warp_active_ = !warp_active_;
        mode_ = warp_active_ ? MechaMode::Warping : MechaMode::SpaceSailing;
    }
}

void Mecha::move_3d(const Vec2& input_wasd, const Vec3& cam_right, f32 alt_input, f32 dt, Planet* planet) {
    if (!planet) return;

    // Normal at current mecha pos
    Vec3 normal = {0.0f, 1.0f, 0.0f};
    f32 plen = sqrtf(pos_.x * pos_.x + pos_.y * pos_.y + pos_.z * pos_.z);
    if (plen > 1e-4f) {
        normal = {pos_.x / plen, pos_.y / plen, pos_.z / plen};
    }

    // 切向右向量: 相机右向量投影到当前点切平面。
    // 相机 right = view_fwd × up, 天然与 up 正交且始终指向屏幕右侧。
    f32 r_dot = cam_right.x * normal.x + cam_right.y * normal.y + cam_right.z * normal.z;
    Vec3 right_tangent = {cam_right.x - r_dot * normal.x, cam_right.y - r_dot * normal.y, cam_right.z - r_dot * normal.z};
    f32 rt_len = sqrtf(right_tangent.x * right_tangent.x + right_tangent.y * right_tangent.y + right_tangent.z * right_tangent.z);
    if (rt_len > 1e-4f) { right_tangent.x /= rt_len; right_tangent.y /= rt_len; right_tangent.z /= rt_len; }
    else right_tangent = {1.0f, 0.0f, 0.0f};

    // 切向前向量 = normal × right_tangent。
    // 恒等于相机前向的切向投影 (n × (f×n) = f_tangent), 任意半球方向一致, 无需翻转判断。
    Vec3 fwd_tangent = {
        normal.y * right_tangent.z - normal.z * right_tangent.y,
        normal.z * right_tangent.x - normal.x * right_tangent.z,
        normal.x * right_tangent.y - normal.y * right_tangent.x
    };

    // Combined desired 3D velocity
    Vec3 desired_v = {
        input_wasd.y * fwd_tangent.x + input_wasd.x * right_tangent.x,
        input_wasd.y * fwd_tangent.y + input_wasd.x * right_tangent.y,
        input_wasd.y * fwd_tangent.z + input_wasd.x * right_tangent.z
    };
    f32 v_len = sqrtf(desired_v.x * desired_v.x + desired_v.y * desired_v.y + desired_v.z * desired_v.z);

    f32 move_speed = (mode_ == MechaMode::GroundWalk) ? walk_speed_ :
                     (mode_ == MechaMode::LowFlight) ? fly_speed_ : sail_speed_;
    if (is_low_energy()) move_speed *= 0.4f;

    if (v_len > 0.01f) {
        desired_v.x = (desired_v.x / v_len) * move_speed;
        desired_v.y = (desired_v.y / v_len) * move_speed;
        desired_v.z = (desired_v.z / v_len) * move_speed;

        // Update heading on screen (0 = forward, PI/2 = right, PI = back, -PI/2 = left)
        heading_ = atan2f(input_wasd.x, input_wasd.y);

        // Power draw & consumption
        f32 pwr = (mode_ == MechaMode::GroundWalk) ? 0.8f : (mode_ == MechaMode::LowFlight ? 3.5f : 8.0f);
        current_power_draw_mw_ += pwr;
        consume_energy(pwr * dt);
    }

    // Smooth altitude update (One-shot takeoff & landing)
    if (alt_input > 0.0f) {
        thruster_burn_ = std::min(1.0f, thruster_burn_ + dt * 4.0f);
        f32 climb_speed = (mode_ == MechaMode::GroundWalk) ? 14.0f : (altitude_ < 80.0f ? 28.0f : 80.0f);
        altitude_ += climb_speed * dt;
        if (altitude_ > 2.0f && mode_ == MechaMode::GroundWalk) {
            mode_ = MechaMode::LowFlight;
        }
        if (altitude_ > 120.0f && mode_ == MechaMode::LowFlight) {
            mode_ = MechaMode::SpaceSailing;
        }
        consume_energy(4.0f * dt);
    } else if (alt_input < 0.0f) {
        f32 sink_speed = (altitude_ > 80.0f) ? 60.0f : 24.0f;
        altitude_ -= sink_speed * dt;
        if (altitude_ <= 0.0f) {
            altitude_ = 0.0f;
            mode_ = MechaMode::GroundWalk;
        } else if (altitude_ <= 100.0f && mode_ == MechaMode::SpaceSailing) {
            mode_ = MechaMode::LowFlight;
        }
        thruster_burn_ = std::max(0.0f, thruster_burn_ - dt * 3.0f);
    } else {
        thruster_burn_ = std::max(0.0f, thruster_burn_ - dt * 2.0f);
    }

    // Re-entry plasma heating effect when sinking fast into dense atmosphere
    if (alt_input < 0.0f && altitude_ > 15.0f && altitude_ < 120.0f) {
        reentry_glow_ = std::min(1.0f, reentry_glow_ + dt * 3.0f);
    } else {
        reentry_glow_ = std::max(0.0f, reentry_glow_ - dt * 2.0f);
    }

    // Integrate 3D position
    Vec3 new_pos = {
        pos_.x + desired_v.x * dt,
        pos_.y + desired_v.y * dt,
        pos_.z + desired_v.z * dt
    };
    f32 n_calc = sqrtf(new_pos.x * new_pos.x + new_pos.y * new_pos.y + new_pos.z * new_pos.z);
    if (n_calc > 1e-4f) {
        Vec3 u = {new_pos.x / n_calc, new_pos.y / n_calc, new_pos.z / n_calc};
        f32 h_terrain = planet->get_terrain_height_3d(u);
        f32 total_r = planet->radius() + h_terrain + altitude_;
        pos_ = {u.x * total_r, u.y * total_r, u.z * total_r};
    }
}

void Mecha::move_ground(const Vec2& dir, f32 dt, Planet* planet) {
    Vec3 default_right = {1.0f, 0.0f, 0.0f};
    move_3d(dir, default_right, 0.0f, dt, planet);
}

void Mecha::move_flight(const Vec2& dir, f32 alt_input, f32 dt, Planet* planet) {
    Vec3 default_right = {1.0f, 0.0f, 0.0f};
    move_3d(dir, default_right, alt_input, dt, planet);
}

void Mecha::move_space(const Vec3& thrust_dir, f32 dt) {
    (void)thrust_dir;
    (void)dt;
}

void Mecha::upgrade_core() {
    core_level_++;
    max_energy_mj_ += 50.0f;
    energy_mj_ = max_energy_mj_;
}

void Mecha::upgrade_drones() {
    max_drones_ += 2;
    drone_speed_ += 10.0f;
    drones_.resize(max_drones_);
    for (auto& d : drones_) {
        d.pos = pos_;
        d.speed = drone_speed_;
    }
}

void Mecha::upgrade_speed() {
    walk_speed_ += 4.0f;
    fly_speed_ += 8.0f;
    sail_speed_ += 40.0f;
}

void Mecha::dispatch_drone(const Vec3& target_pos, u32 target_id) {
    for (auto& d : drones_) {
        if (!d.active) {
            d.active = true;
            d.returning = false;
            d.pos = pos_;
            d.target_pos = target_pos;
            d.target_id = target_id;
            d.beam_timer = 0.8f;
            break;
        }
    }
}

void Mecha::update(f32 dt, Planet* current_planet, bool space_mode) {
    (void)space_mode;
    update_energy_and_combustion(dt);
    update_drones(dt);

    if (current_planet && mode_ == MechaMode::GroundWalk) {
        f32 plen = sqrtf(pos_.x * pos_.x + pos_.y * pos_.y + pos_.z * pos_.z);
        if (plen > 1e-4f) {
            Vec3 u = {pos_.x / plen, pos_.y / plen, pos_.z / plen};
            f32 h_terrain = current_planet->get_terrain_height_3d(u);
            f32 target_r = current_planet->radius() + h_terrain + altitude_;
            pos_ = {u.x * target_r, u.y * target_r, u.z * target_r};
        }
    }
}

void Mecha::update_energy_and_combustion(f32 dt) {
    current_charge_rate_mw_ = 0.0f;

    // Fuel combustion in chamber
    if (energy_mj_ < max_energy_mj_ && fuel_slot_.count > 0 && fuel_slot_.kind != FuelKind::None) {
        f32 burn_rate_mj_per_sec = 8.0f * fuel_slot_.combustion_efficiency;
        f32 energy_gained = burn_rate_mj_per_sec * dt;
        energy_mj_ = std::min(max_energy_mj_, energy_mj_ + energy_gained);
        current_charge_rate_mw_ = burn_rate_mj_per_sec;

        if (energy_mj_ >= max_energy_mj_) {
            // Reached full charge
        }
    }

    current_power_draw_mw_ = std::max(0.2f, current_power_draw_mw_ * (1.0f - dt * 3.0f));
}

void Mecha::update_drones(f32 dt) {
    for (auto& d : drones_) {
        if (!d.active) continue;

        if (!d.returning) {
            // Fly towards build target
            Vec3 delta = {d.target_pos.x - d.pos.x, d.target_pos.y - d.pos.y, d.target_pos.z - d.pos.z};
            f32 dist = sqrtf(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
            if (dist > 2.0f) {
                d.pos.x += (delta.x / dist) * d.speed * dt;
                d.pos.y += (delta.y / dist) * d.speed * dt;
                d.pos.z += (delta.z / dist) * d.speed * dt;
            } else {
                d.beam_timer -= dt;
                if (d.beam_timer <= 0.0f) {
                    d.returning = true;
                }
            }
        } else {
            // Fly back to mecha
            Vec3 delta = {pos_.x - d.pos.x, pos_.y - d.pos.y, pos_.z - d.pos.z};
            f32 dist = sqrtf(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
            if (dist > 2.0f) {
                d.pos.x += (delta.x / dist) * d.speed * dt;
                d.pos.y += (delta.y / dist) * d.speed * dt;
                d.pos.z += (delta.z / dist) * d.speed * dt;
            } else {
                d.active = false;
                d.returning = false;
            }
        }
    }
}

}
