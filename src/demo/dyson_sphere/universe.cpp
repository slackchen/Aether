#include "demo/dyson_sphere/universe.h"
#include <cmath>
#include <random>
#include <algorithm>
#include <cstdio>

namespace dsp {

constexpr f32 kPi = 3.14159265358979323846f;
constexpr f32 kTwoPi = kPi * 2.0f;
constexpr f32 kHalfPi = kPi * 0.5f;

const char* spectral_name(SpectralType type) {
    switch (type) {
        case SpectralType::ClassO: return "O型 蓝超巨星";
        case SpectralType::ClassB: return "B型 蓝巨星";
        case SpectralType::ClassA: return "A型 白巨星";
        case SpectralType::ClassF: return "F型 黄白星";
        case SpectralType::ClassG: return "G型 黄矮星 (类太阳)";
        case SpectralType::ClassK: return "K型 橙矮星";
        case SpectralType::ClassM: return "M型 红矮星";
        case SpectralType::NeutronStar: return "中子星 (脉冲星)";
        case SpectralType::BlackHole: return "恒星级天体黑洞";
        default: return "未知恒星";
    }
}

Color star_color(SpectralType type) {
    switch (type) {
        case SpectralType::ClassO: return Color{0.45f, 0.70f, 1.0f, 1.0f};
        case SpectralType::ClassB: return Color{0.65f, 0.85f, 1.0f, 1.0f};
        case SpectralType::ClassA: return Color{0.92f, 0.96f, 1.0f, 1.0f};
        case SpectralType::ClassF: return Color{1.0f, 0.98f, 0.85f, 1.0f};
        case SpectralType::ClassG: return Color{1.0f, 0.88f, 0.35f, 1.0f};
        case SpectralType::ClassK: return Color{1.0f, 0.65f, 0.20f, 1.0f};
        case SpectralType::ClassM: return Color{1.0f, 0.35f, 0.20f, 1.0f};
        case SpectralType::NeutronStar: return Color{0.70f, 0.95f, 1.0f, 1.0f};
        case SpectralType::BlackHole: return Color{0.85f, 0.35f, 0.85f, 1.0f};
        default: return Color{1.0f, 1.0f, 1.0f, 1.0f};
    }
}

DSPCamera::DSPCamera() = default;

void DSPCamera::handle_input(const Vec2& mouse_delta, f32 wheel_delta, bool right_mouse_down, bool middle_mouse_down) {
    (void)middle_mouse_down;
    if (right_mouse_down) {
        yaw_ += mouse_delta.x * 0.005f;
        pitch_ = std::clamp(pitch_ + mouse_delta.y * 0.005f, 0.10f, kHalfPi * 0.90f);
    }

    if (fabsf(wheel_delta) > 0.001f) {
        f32 factor = powf(0.85f, wheel_delta);
        target_distance_ = std::clamp(target_distance_ * factor, 25.0f, 120000.0f);
    }
}

void DSPCamera::update_scale_transition() {
    if (distance_ < 60.0f) {
        scale_ = ViewScale::MechaClose;
    } else if (distance_ < 350.0f) {
        scale_ = ViewScale::FactoryBirdView;
    } else if (distance_ < 2500.0f) {
        scale_ = ViewScale::PlanetGlobe;
    } else if (distance_ < 25000.0f) {
        scale_ = ViewScale::StarSystem;
    } else {
        scale_ = ViewScale::GalaxyMap;
    }
}

void DSPCamera::update(f32 dt, const Vec3& target_mecha_pos, const Vec3& target_planet_pos, const Vec3& target_star_pos, f32 mecha_altitude) {
    // Dynamic distance tracking based on mecha altitude for seamless one-shot takeoff & landing
    f32 alt_dist_offset = (mecha_altitude < 15.0f) ? 0.0f :
                          (mecha_altitude < 80.0f) ? (mecha_altitude - 15.0f) * 0.85f :
                          (65.0f * 0.85f + (mecha_altitude - 80.0f) * 1.8f);

    f32 dynamic_target_dist = target_distance_ + alt_dist_offset;
    distance_ += (dynamic_target_dist - distance_) * std::min(1.0f, dt * 10.0f);
    update_scale_transition();

    fov_deg_ = 65.0f + std::clamp(mecha_altitude / 200.0f, 0.0f, 1.0f) * 8.0f;

    // Target always tracks Mecha smoothly
    target_.x += (target_mecha_pos.x - target_.x) * std::min(1.0f, dt * 14.0f);
    target_.y += (target_mecha_pos.y - target_.y) * std::min(1.0f, dt * 14.0f);
    target_.z += (target_mecha_pos.z - target_.z) * std::min(1.0f, dt * 14.0f);

    // Compute surface normal N at mecha pos
    Vec3 normal = {0.0f, 1.0f, 0.0f};
    f32 nlen = sqrtf(target_mecha_pos.x * target_mecha_pos.x + target_mecha_pos.y * target_mecha_pos.y + target_mecha_pos.z * target_mecha_pos.z);
    if (nlen > 1.0f) {
        normal = {target_mecha_pos.x / nlen, target_mecha_pos.y / nlen, target_mecha_pos.z / nlen};
    }

    // Continuous Orthonormal Planetary Tangent Frame without 180-degree jumps
    // Smoothly project previous fwd_tangent_ onto the new normal plane
    f32 f_dot_n = fwd_tangent_.x * normal.x + fwd_tangent_.y * normal.y + fwd_tangent_.z * normal.z;
    Vec3 proj_fwd = {
        fwd_tangent_.x - normal.x * f_dot_n,
        fwd_tangent_.y - normal.y * f_dot_n,
        fwd_tangent_.z - normal.z * f_dot_n
    };
    f32 pf_len = sqrtf(proj_fwd.x * proj_fwd.x + proj_fwd.y * proj_fwd.y + proj_fwd.z * proj_fwd.z);
    
    if (pf_len > 1e-4f) {
        fwd_tangent_ = {proj_fwd.x / pf_len, proj_fwd.y / pf_len, proj_fwd.z / pf_len};
    } else {
        // Fallback: If mecha teleports exactly 90 degrees directly onto the forward vector
        fwd_tangent_ = {1.0f, 0.0f, 0.0f}; 
    }

    Vec3 north = fwd_tangent_;
    Vec3 east = {
        north.y * normal.z - north.z * normal.y,
        north.z * normal.x - north.x * normal.z,
        north.x * normal.y - north.y * normal.x
    };

    f32 cos_p = cosf(pitch_);
    f32 sin_p = sinf(pitch_);
    f32 cos_y = cosf(yaw_);
    f32 sin_y = sinf(yaw_);

    // Camera sits behind mecha (-north) elevated (+normal), rotating with yaw
    Vec3 ground_back = {-cos_y * north.x - sin_y * east.x,
                        -cos_y * north.y - sin_y * east.y,
                        -cos_y * north.z - sin_y * east.z};

    Vec3 orbit_dir = {
        sin_p * normal.x + cos_p * ground_back.x,
        sin_p * normal.y + cos_p * ground_back.y,
        sin_p * normal.z + cos_p * ground_back.z
    };

    eye_ = {
        target_.x + distance_ * orbit_dir.x,
        target_.y + distance_ * orbit_dir.y,
        target_.z + distance_ * orbit_dir.z
    };

    up_ = normal;
}

Mat4 DSPCamera::view_matrix() const {
    return mat4_look_at(eye_, target_, up_);
}

Mat4 DSPCamera::projection_matrix(f32 aspect) const {
    f32 znear = 0.5f;
    f32 zfar = 250000.0f;
    return mat4_perspective(fov_deg_ * 3.14159f / 180.0f, aspect, znear, zfar);
}

Mat4 DSPCamera::view_projection(f32 aspect) const {
    return mat4_mul(projection_matrix(aspect), view_matrix());
}

void DSPCamera::screen_to_ray(const Vec2& screen_pos, f32 screen_w, f32 screen_h, Vec3& out_origin, Vec3& out_dir) const {
    out_origin = eye_;

    f32 ndc_x = (2.0f * screen_pos.x) / screen_w - 1.0f;
    f32 ndc_y = 1.0f - (2.0f * screen_pos.y) / screen_h;

    f32 tan_half_fov = tanf(fov_deg_ * 0.5f * 3.14159f / 180.0f);
    f32 aspect = screen_w / (screen_h > 0.0f ? screen_h : 1.0f);

    Vec3 fwd = forward();
    Vec3 rgt = right();
    Vec3 cam_up = {
        rgt.y * fwd.z - rgt.z * fwd.y,
        rgt.z * fwd.x - rgt.x * fwd.z,
        rgt.x * fwd.y - rgt.y * fwd.x
    };

    out_dir = {
        fwd.x + rgt.x * ndc_x * tan_half_fov * aspect + cam_up.x * ndc_y * tan_half_fov,
        fwd.y + rgt.y * ndc_x * tan_half_fov * aspect + cam_up.y * ndc_y * tan_half_fov,
        fwd.z + rgt.z * ndc_x * tan_half_fov * aspect + cam_up.z * ndc_y * tan_half_fov
    };
    f32 dlen = sqrtf(out_dir.x * out_dir.x + out_dir.y * out_dir.y + out_dir.z * out_dir.z);
    if (dlen > 1e-5f) { out_dir.x /= dlen; out_dir.y /= dlen; out_dir.z /= dlen; }
}

Universe::Universe(u32 seed) {
    generate_star_cluster(seed);
}

void Universe::generate_star_cluster(u32 seed) {
    stars_.clear();
    std::mt19937 rng(seed);

    // Star 0: Starter Sol System
    {
        auto s = std::make_unique<Star>();
        s->id = 0;
        s->name = "伊卡洛斯主恒星 (母星系)";
        s->type = SpectralType::ClassG;
        s->luminosity = 1.0f;
        s->radius = 350.0f;
        s->pos_ly = {0.0f, 0.0f, 0.0f};

        // Planet 0: Starter Mediterranean
        s->planets.push_back(std::make_unique<Planet>(seed + 1, "伊卡洛斯 I (地中海母星)", BiomeType::Mediterranean, 500.0f, 1.0f, 0.04f));
        // Planet 1: Desert Titanium Rich
        s->planets.push_back(std::make_unique<Planet>(seed + 2, "伊卡洛斯 II (戈壁荒漠)", BiomeType::Desert, 460.0f, 1.8f, 0.025f));
        // Planet 2: Ice Planet
        s->planets.push_back(std::make_unique<Planet>(seed + 3, "伊卡洛斯 III (远日冰川)", BiomeType::Ice, 420.0f, 2.7f, 0.018f));
        // Planet 3: Gas Giant
        s->planets.push_back(std::make_unique<Planet>(seed + 4, "伊卡洛斯 IV (气态巨行星)", BiomeType::GasGiant, 950.0f, 4.5f, 0.010f));

        stars_.push_back(std::move(s));
    }

    // Star 1: Sirius (Class A White)
    {
        auto s = std::make_unique<Star>();
        s->id = 1;
        s->name = "天狼星 (A型白巨星)";
        s->type = SpectralType::ClassA;
        s->luminosity = 1.75f;
        s->radius = 600.0f;
        s->pos_ly = {8.6f, 2.1f, -4.5f};
        s->planets.push_back(std::make_unique<Planet>(seed + 10, "天狼 I (熔岩火山)", BiomeType::Volcanic, 520.0f, 1.2f, 0.05f));
        s->planets.push_back(std::make_unique<Planet>(seed + 11, "天狼 II (干旱沙丘)", BiomeType::Desert, 480.0f, 2.2f, 0.03f));
        stars_.push_back(std::move(s));
    }

    // Star 2: Cygnus X-1 (Black Hole)
    {
        auto s = std::make_unique<Star>();
        s->id = 2;
        s->name = "天鹅座 X-1 (天体黑洞)";
        s->type = SpectralType::BlackHole;
        s->luminosity = 0.05f;
        s->radius = 350.0f;
        s->pos_ly = {-12.4f, 6.8f, 11.2f};
        s->planets.push_back(std::make_unique<Planet>(seed + 20, "天鹅座 I (深渊死星)", BiomeType::Volcanic, 440.0f, 2.5f, 0.08f));
        stars_.push_back(std::move(s));
    }

    // Star 3: Vega (Class O Blue Giant)
    {
        auto s = std::make_unique<Star>();
        s->id = 3;
        s->name = "织女星 (O型蓝巨星)";
        s->type = SpectralType::ClassO;
        s->luminosity = 2.45f;
        s->radius = 800.0f;
        s->pos_ly = {15.4f, -8.3f, 7.2f};
        s->planets.push_back(std::make_unique<Planet>(seed + 30, "织女 I (极寒冻土)", BiomeType::Ice, 540.0f, 2.0f, 0.03f));
        s->planets.push_back(std::make_unique<Planet>(seed + 31, "织女 II (类地海洋)", BiomeType::Mediterranean, 500.0f, 3.2f, 0.02f));
        stars_.push_back(std::move(s));
    }
}

Planet* Universe::current_planet() {
    Star* s = current_star();
    if (!s || s->planets.empty()) return nullptr;
    if (current_planet_idx_ >= s->planets.size()) current_planet_idx_ = 0;
    return s->planets[current_planet_idx_].get();
}

void Universe::update(f32 dt) {
    for (auto& s : stars_) {
        for (auto& p : s->planets) {
            p->update(dt);
        }
        s->dyson_sphere.update(dt, 0, 0);
    }
}

}
