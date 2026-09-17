#include "demo/dyson_sphere/power.h"
#include <cmath>
#include <algorithm>

namespace dsp {

static f32 hash1(f32 x) {
    f32 n = sinf(x * 127.1f) * 43758.5453f;
    return n - floorf(n);
}

PowerGrid::PowerGrid() = default;

void PowerGrid::update(f32 dt, const std::vector<Building>& buildings, const Vec3& mecha_pos,
                       f32& mecha_energy_mj, f32 mecha_max_energy_mj, const Vec3& sun_dir, f32 time) {
    nodes_.clear();
    total_gen_kw_ = 0.0f;
    total_demand_kw_ = 0.0f;
    mecha_charging_ = false;

    // 聚合发电与耗电; 太阳能受昼夜光照角约束, 风电受风速扰动
    for (const auto& b : buildings) {
        PowerNode n;
        n.building_id = b.id;
        n.kind = b.kind;
        n.pos = b.world_pos;

        switch (b.kind) {
            case BuildingKind::WindTurbine: {
                f32 gust = 0.55f + 0.45f * hash1(floorf(time * 0.2f) + (f32)b.id * 0.37f);
                n.gen_capacity_kw = 300.0f * gust;
                n.supply_radius = 15.0f;
                n.connect_radius = 50.0f;
                total_gen_kw_ += n.gen_capacity_kw;
                break;
            }
            case BuildingKind::SolarPanel: {
                f32 plen = sqrtf(b.world_pos.x * b.world_pos.x + b.world_pos.y * b.world_pos.y + b.world_pos.z * b.world_pos.z);
                f32 ndotl = 0.0f;
                if (plen > 1.0f) {
                    ndotl = (b.world_pos.x / plen) * sun_dir.x + (b.world_pos.y / plen) * sun_dir.y + (b.world_pos.z / plen) * sun_dir.z;
                }
                f32 sun_factor = std::clamp(ndotl, 0.0f, 1.0f);
                n.gen_capacity_kw = 360.0f * sun_factor * sun_factor;
                n.supply_radius = 12.0f;
                n.connect_radius = 45.0f;
                total_gen_kw_ += n.gen_capacity_kw;
                break;
            }
            case BuildingKind::ThermalPowerPlant:
                n.gen_capacity_kw = 2160.0f;
                n.supply_radius = 16.0f;
                n.connect_radius = 60.0f;
                total_gen_kw_ += n.gen_capacity_kw;
                break;
            case BuildingKind::FusionPowerPlant:
                n.gen_capacity_kw = 15000.0f;
                n.supply_radius = 24.0f;
                n.connect_radius = 80.0f;
                total_gen_kw_ += n.gen_capacity_kw;
                break;
            case BuildingKind::ArtificialStar:
                n.gen_capacity_kw = 72000.0f;
                n.supply_radius = 35.0f;
                n.connect_radius = 100.0f;
                total_gen_kw_ += n.gen_capacity_kw;
                break;
            case BuildingKind::RayReceiver:
                n.gen_capacity_kw = 12500.0f;
                n.supply_radius = 20.0f;
                n.connect_radius = 70.0f;
                total_gen_kw_ += n.gen_capacity_kw;
                break;
            case BuildingKind::TeslaTower:
                n.supply_radius = 12.0f;
                n.connect_radius = 55.0f;
                break;
            case BuildingKind::WirelessPowerTower:
                n.supply_radius = 22.0f;
                n.connect_radius = 80.0f;
                break;
            default: {
                f32 d = building_power_demand_kw(b.kind);
                n.demand_kw = d;
                total_demand_kw_ += d;
                break;
            }
        }
        nodes_.push_back(n);

        // Wireless Mecha Charging check
        if (b.kind == BuildingKind::WirelessPowerTower || b.kind == BuildingKind::TeslaTower) {
            f32 dx = b.world_pos.x - mecha_pos.x;
            f32 dy = b.world_pos.y - mecha_pos.y;
            f32 dz = b.world_pos.z - mecha_pos.z;
            f32 dist = sqrtf(dx * dx + dy * dy + dz * dz);
            if (dist <= n.supply_radius && mecha_energy_mj < mecha_max_energy_mj) {
                f32 charge_rate_mw = (b.kind == BuildingKind::WirelessPowerTower) ? 4.8f : 1.6f;
                mecha_energy_mj = std::min(mecha_max_energy_mj, mecha_energy_mj + charge_rate_mw * dt);
                total_demand_kw_ += charge_rate_mw * 1000.0f;
                mecha_charging_ = true;
            }
        }
    }

    // Default emergency planetary baseline power
    if (total_gen_kw_ < 600.0f) {
        total_gen_kw_ += 600.0f;
    }

    // Calculate Satisfaction
    if (total_demand_kw_ <= 0.0f) {
        satisfaction_ = 1.0f;
    } else {
        f32 balance = total_gen_kw_ - total_demand_kw_;
        if (balance >= 0.0f) {
            satisfaction_ = 1.0f;
            // Charge accumulator
            accum_charge_mj_ = std::min(accum_max_mj_, accum_charge_mj_ + (balance * 0.001f) * dt * 0.2f);
        } else {
            f32 deficit = -balance;
            // Try to discharge accumulator
            if (accum_charge_mj_ > 0.0f) {
                f32 discharge = std::min(accum_charge_mj_, (deficit * 0.001f) * dt);
                accum_charge_mj_ -= discharge;
                f32 eff_gen = total_gen_kw_ + (discharge / dt) * 1000.0f;
                satisfaction_ = std::clamp(eff_gen / total_demand_kw_, 0.2f, 1.0f);
            } else {
                satisfaction_ = std::clamp(total_gen_kw_ / total_demand_kw_, 0.15f, 1.0f);
            }
        }
    }
}

}
