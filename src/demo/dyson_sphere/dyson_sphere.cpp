#include "demo/dyson_sphere/dyson_sphere.h"
#include <cmath>
#include <random>
#include <algorithm>

namespace dsp {

constexpr f32 kPi = 3.14159265358979323846f;
constexpr f32 kTwoPi = kPi * 2.0f;

DysonSphereManager::DysonSphereManager() {
    init_geodesic_shell();

    // Spawn starting swarm
    spawn_sails(300);
}

void DysonSphereManager::init_geodesic_shell() {
    nodes_.clear();
    struts_.clear();
    panels_.clear();

    // Golden ratio for icosahedron vertices
    f32 phi = (1.0f + sqrtf(5.0f)) * 0.5f;
    f32 inv_len = 1.0f / sqrtf(1.0f + phi * phi);
    f32 a = 1.0f * inv_len * sphere_radius_;
    f32 b = phi * inv_len * sphere_radius_;

    Vec3 raw_verts[12] = {
        {-a,  b, 0.0f}, { a,  b, 0.0f}, {-a, -b, 0.0f}, { a, -b, 0.0f},
        {0.0f, -a,  b}, {0.0f,  a,  b}, {0.0f, -a, -b}, {0.0f,  a, -b},
        { b, 0.0f, -a}, { b, 0.0f,  a}, {-b, 0.0f, -a}, {-b, 0.0f,  a},
    };

    for (u32 i = 0; i < 12; i++) {
        DysonNode n;
        n.id = i;
        n.pos = raw_verts[i];
        n.rockets_invested = (i < 3) ? 30 : 0; // First 3 nodes start completed
        n.completed = (i < 3);
        nodes_.push_back(n);
    }

    const u32 raw_faces[20][3] = {
        {0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11},
        {1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
        {3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9},
        {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}
    };

    for (u32 i = 0; i < 20; i++) {
        DysonShellPanel p;
        p.node_indices[0] = raw_faces[i][0];
        p.node_indices[1] = raw_faces[i][1];
        p.node_indices[2] = raw_faces[i][2];
        p.sails_absorbed = (i == 0) ? 120 : 0;
        p.completed = (i == 0);
        p.fill_ratio = p.completed ? 1.0f : 0.0f;
        panels_.push_back(p);

        // Add 3 struts per face if not duplicate
        auto add_strut = [this](u32 na, u32 nb) {
            for (const auto& s : struts_) {
                if ((s.node_a == na && s.node_b == nb) || (s.node_a == nb && s.node_b == na)) return;
            }
            DysonStrut s;
            s.node_a = na;
            s.node_b = nb;
            s.completed = (na < 3 && nb < 3);
            s.progress = s.completed ? 1.0f : 0.0f;
            struts_.push_back(s);
        };
        add_strut(raw_faces[i][0], raw_faces[i][1]);
        add_strut(raw_faces[i][1], raw_faces[i][2]);
        add_strut(raw_faces[i][2], raw_faces[i][0]);
    }
}

void DysonSphereManager::spawn_sails(u32 count) {
    std::mt19937 rng(1337);
    std::uniform_real_distribution<f32> dist_r(sphere_radius_ * 0.8f, sphere_radius_ * 1.3f);
    std::uniform_real_distribution<f32> dist_ang(0.0f, kTwoPi);
    std::uniform_real_distribution<f32> dist_inc(-0.45f, 0.45f);

    for (u32 i = 0; i < count; i++) {
        SolarSailParticle s;
        s.orbit_radius = dist_r(rng);
        s.angle = dist_ang(rng);
        s.inclination = dist_inc(rng);
        s.life = 1200.0f;
        s.max_life = 1200.0f;
        s.absorbed = false;
        sails_.push_back(s);
    }
}

void DysonSphereManager::invest_rockets(u32 count) {
    u32 rem = count;
    for (auto& n : nodes_) {
        if (!n.completed) {
            u32 needed = n.rockets_required - n.rockets_invested;
            u32 add = std::min(rem, needed);
            n.rockets_invested += add;
            rem -= add;
            if (n.rockets_invested >= n.rockets_required) {
                n.completed = true;
            }
            if (rem == 0) break;
        }
    }
}

u32 DysonSphereManager::completed_nodes() const {
    u32 count = 0;
    for (const auto& n : nodes_) {
        if (n.completed) count++;
    }
    return count;
}

void DysonSphereManager::update(f32 dt, u32 new_launched_sails, u32 new_launched_rockets) {
    if (new_launched_sails > 0) {
        spawn_sails(new_launched_sails);
    }
    if (new_launched_rockets > 0) {
        invest_rockets(new_launched_rockets);
    }

    // Update solar sails
    swarm_gen_gw_ = 0.0f;
    f32 orbit_angular_speed = 0.08f;

    for (auto it = sails_.begin(); it != sails_.end(); ) {
        it->angle += orbit_angular_speed * dt;
        if (it->angle > kTwoPi) it->angle -= kTwoPi;

        f32 cos_a = cosf(it->angle);
        f32 sin_a = sinf(it->angle);
        it->pos = {
            it->orbit_radius * cos_a,
            it->orbit_radius * sin_a * sinf(it->inclination),
            it->orbit_radius * sin_a * cosf(it->inclination)
        };

        it->life -= dt;

        // Try absorption into incomplete shell panels
        if (!it->absorbed) {
            for (auto& p : panels_) {
                if (!p.completed && p.sails_absorbed < p.sails_required) {
                    const auto& na = nodes_[p.node_indices[0]];
                    const auto& nb = nodes_[p.node_indices[1]];
                    const auto& nc = nodes_[p.node_indices[2]];
                    if (na.completed && nb.completed && nc.completed) {
                        p.sails_absorbed++;
                        p.fill_ratio = (f32)p.sails_absorbed / (f32)p.sails_required;
                        if (p.sails_absorbed >= p.sails_required) {
                            p.completed = true;
                        }
                        it->absorbed = true;
                        break;
                    }
                }
            }
        }

        if (it->life <= 0.0f || it->absorbed) {
            it = sails_.erase(it);
        } else {
            swarm_gen_gw_ += 0.000036f; // 36 kW per solar sail
            ++it;
        }
    }

    // Update shell nodes and panels power
    shell_gen_gw_ = 0.0f;
    for (const auto& n : nodes_) {
        if (n.completed) {
            shell_gen_gw_ += n.energy_output_mw * 0.001f; // 96 MW = 0.096 GW
        }
    }
    for (const auto& p : panels_) {
        if (p.completed) {
            shell_gen_gw_ += 0.120f; // 120 MW per complete panel
        } else if (p.fill_ratio > 0.0f) {
            shell_gen_gw_ += 0.120f * p.fill_ratio;
        }
    }
}

}
