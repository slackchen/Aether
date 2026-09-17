#pragma once

#include "core/platform.h"
#include "core/math.h"
#include "demo/dyson_sphere/planet.h"
#include "demo/dyson_sphere/dyson_sphere.h"
#include <vector>
#include <memory>
#include <string>

namespace dsp {

using namespace aether;

enum class SpectralType {
    ClassO,       // Blue Giant
    ClassB,       // Blue
    ClassA,       // White
    ClassF,       // Yellow-White
    ClassG,       // Sol (Yellow)
    ClassK,       // Orange
    ClassM,       // Red Dwarf
    NeutronStar,  // Pulsar
    BlackHole,    // Gravitational Lensing
};

const char* spectral_name(SpectralType type);
Color star_color(SpectralType type);

struct Star {
    u32 id = 0;
    std::string name;
    SpectralType type = SpectralType::ClassG;
    f32 luminosity = 1.0f;
    f32 radius = 350.0f;
    Vec3 pos_ly{0.0f, 0.0f, 0.0f}; // Position in light years
    std::vector<std::unique_ptr<Planet>> planets;
    DysonSphereManager dyson_sphere;
};

enum class ViewScale {
    MechaClose = 0,
    FactoryBirdView,
    PlanetGlobe,
    StarSystem,
    GalaxyMap,
};

class DSPCamera {
public:
    DSPCamera();

    void update(f32 dt, const Vec3& target_mecha_pos, const Vec3& target_planet_pos, const Vec3& target_star_pos, f32 mecha_altitude);
    void handle_input(const Vec2& mouse_delta, f32 wheel_delta, bool right_mouse_down, bool middle_mouse_down);

    Vec3 eye() const { return eye_; }
    Vec3 target() const { return target_; }
    Vec3 up() const { return up_; }
    f32 fov_deg() const { return fov_deg_; }
    ViewScale current_scale() const { return scale_; }
    f32 zoom_distance() const { return distance_; }

    Vec3 forward() const {
        Vec3 f = {target_.x - eye_.x, target_.y - eye_.y, target_.z - eye_.z};
        f32 flen = sqrtf(f.x * f.x + f.y * f.y + f.z * f.z);
        return (flen > 1e-5f) ? Vec3{f.x / flen, f.y / flen, f.z / flen} : Vec3{0.0f, 0.0f, -1.0f};
    }

    Vec3 right() const {
        Vec3 f = forward();
        Vec3 r = {
            f.y * up_.z - f.z * up_.y,
            f.z * up_.x - f.x * up_.z,
            f.x * up_.y - f.y * up_.x
        };
        f32 rlen = sqrtf(r.x * r.x + r.y * r.y + r.z * r.z);
        return (rlen > 1e-5f) ? Vec3{r.x / rlen, r.y / rlen, r.z / rlen} : Vec3{1.0f, 0.0f, 0.0f};
    }

    Mat4 view_matrix() const;
    Mat4 projection_matrix(f32 aspect) const;
    Mat4 view_projection(f32 aspect) const;

    // Ray from screen pos
    void screen_to_ray(const Vec2& screen_pos, f32 screen_w, f32 screen_h, Vec3& out_origin, Vec3& out_dir) const;

private:
    void update_scale_transition();

    Vec3 eye_{0.0f, 150.0f, 250.0f};
    Vec3 target_{0.0f, 0.0f, 200.0f};
    Vec3 up_{0.0f, 1.0f, 0.0f};

    f32 distance_ = 45.0f;
    f32 target_distance_ = 45.0f;
    f32 yaw_ = 0.0f;
    f32 pitch_ = 0.55f;
    f32 fov_deg_ = 65.0f;

    Vec3 fwd_tangent_{0.0f, 0.0f, 1.0f};

    ViewScale scale_ = ViewScale::MechaClose;
};

class Universe {
public:
    Universe(u32 seed = 2026);

    void update(f32 dt);

    const std::vector<std::unique_ptr<Star>>& stars() const { return stars_; }
    std::vector<std::unique_ptr<Star>>& stars() { return stars_; }
    Star* current_star() { return stars_.empty() ? nullptr : stars_[current_star_idx_].get(); }
    Planet* current_planet();

    void select_star(size_t idx) {
        if (idx < stars_.size()) {
            current_star_idx_ = idx;
            current_planet_idx_ = 0;
        }
    }
    void select_planet(size_t idx) {
        Star* s = current_star();
        if (s && idx < s->planets.size()) current_planet_idx_ = idx;
    }

    DSPCamera& camera() { return camera_; }
    const DSPCamera& camera() const { return camera_; }

private:
    void generate_star_cluster(u32 seed);

    std::vector<std::unique_ptr<Star>> stars_;
    size_t current_star_idx_ = 0;
    size_t current_planet_idx_ = 0;
    DSPCamera camera_;
};

}
