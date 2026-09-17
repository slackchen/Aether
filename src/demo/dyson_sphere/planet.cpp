#include "demo/dyson_sphere/planet.h"
#include <cmath>
#include <algorithm>

namespace dsp {

static constexpr f32 kPi = 3.14159265358979323846f;
static constexpr f32 kHalfPi = kPi * 0.5f;
static constexpr f32 kTwoPi = kPi * 2.0f;

Planet::Planet(u32 seed, const std::string& name, BiomeType biome, f32 radius, f32 orbit_dist, f32 orbit_speed)
    : seed_(seed), name_(name), biome_(biome), radius_(radius), orbit_dist_(orbit_dist), orbit_speed_(orbit_speed) {
    atmo_height_ = (biome_ == BiomeType::GasGiant) ? 240.0f : 120.0f;
    rotation_speed_ = 0.0f; // 星球固定, 昼夜由恒星公转产生, 保证建筑/角色坐标系稳定
    field_.set_seed(seed_);
    grid_.generate(seed_, (int)biome_, field_);
}

void Planet::update(f32 dt) {
    orbit_angle_ += orbit_speed_ * dt;
    if (orbit_angle_ > kTwoPi) orbit_angle_ -= kTwoPi;
}

Vec3 Planet::orbital_position() const {
    f32 dist = orbit_dist_ * 4000.0f;
    return {
        dist * cosf(orbit_angle_),
        dist * sinf(orbit_angle_) * sinf(axial_tilt_),
        dist * sinf(orbit_angle_) * cosf(axial_tilt_)
    };
}

Vec3 Planet::lat_lon_to_cartesian(f32 lat, f32 lon, f32 alt_offset) const {
    f32 r = radius_ + alt_offset;
    return {
        r * cosf(lat) * sinf(lon),
        r * sinf(lat),
        r * cosf(lat) * cosf(lon)
    };
}

bool Planet::is_buildable_tile(u32 tile_key) const {
    const Tile& t = grid_.tile(tile_key);
    return t.building_id == 0 && terrain_buildable((Terrain)t.terrain);
}

bool Planet::mine_vein(u32 tile_key, u32 amount, ResourceKind& out_kind) {
    Tile& t = grid_.tile(tile_key);
    if (t.resource == 0 || t.resource_amount == 0) return false;
    out_kind = (ResourceKind)t.resource;
    u32 taken = std::min(amount, t.resource_amount);
    t.resource_amount -= taken;
    return true;
}

f32 Planet::atmosphere_density(f32 altitude) const {
    if (altitude < 0.0f) return 1.0f;
    if (altitude >= atmo_height_) return 0.0f;
    return expf(-altitude / 35.0f);
}

f32 Planet::atmospheric_opacity(f32 altitude) const {
    if (altitude <= 0.0f) return 1.0f;
    if (altitude >= atmo_height_) return 0.0f;
    f32 t = 1.0f - (altitude / atmo_height_);
    return t * t;
}

Color Planet::sky_color(f32 altitude, const Vec3& sun_dir, const Vec3& view_up) const {
    f32 density = atmosphere_density(altitude);
    if (density <= 0.001f) return Color{0.0f, 0.0f, 0.0f, 0.0f};

    f32 sun_dot = view_up.x * sun_dir.x + view_up.y * sun_dir.y + view_up.z * sun_dir.z;

    Color day_sky = (biome_ == BiomeType::Mediterranean) ? Color{0.22f, 0.58f, 0.96f, 1.0f} :
                    (biome_ == BiomeType::Desert) ? Color{0.88f, 0.70f, 0.45f, 1.0f} :
                    (biome_ == BiomeType::Ice) ? Color{0.45f, 0.75f, 0.98f, 1.0f} :
                    (biome_ == BiomeType::Volcanic) ? Color{0.85f, 0.30f, 0.15f, 1.0f} :
                    Color{0.40f, 0.80f, 0.75f, 1.0f};
    Color sunset_sky = Color{0.98f, 0.42f, 0.12f, 1.0f};
    Color night_sky = Color{0.015f, 0.02f, 0.06f, 1.0f};

    Color base_sky;
    if (sun_dot > 0.25f) {
        base_sky = day_sky;
    } else if (sun_dot > -0.15f) {
        f32 factor = (sun_dot + 0.15f) / 0.40f;
        base_sky = {
            night_sky.r * (1.0f - factor) + sunset_sky.r * factor,
            night_sky.g * (1.0f - factor) + sunset_sky.g * factor,
            night_sky.b * (1.0f - factor) + sunset_sky.b * factor,
            1.0f
        };
    } else {
        base_sky = night_sky;
    }
    base_sky.a = density;
    return base_sky;
}

Color Planet::horizon_fog_color(f32 altitude, const Vec3& sun_dir) const {
    f32 density = atmosphere_density(altitude);
    Color sky = sky_color(altitude, sun_dir, {0.0f, 1.0f, 0.0f});
    sky.a = density * 0.65f;
    return sky;
}

bool Planet::raycast(const Vec3& ray_origin, const Vec3& ray_dir, Vec3& out_hit, f32& out_dist) const {
    f32 a = ray_dir.x * ray_dir.x + ray_dir.y * ray_dir.y + ray_dir.z * ray_dir.z;
    f32 b = 2.0f * (ray_origin.x * ray_dir.x + ray_origin.y * ray_dir.y + ray_origin.z * ray_dir.z);
    f32 c = ray_origin.x * ray_origin.x + ray_origin.y * ray_origin.y + ray_origin.z * ray_origin.z - radius_ * radius_;
    f32 discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0.0f) return false;
    f32 sqrt_d = sqrtf(discriminant);
    f32 t0 = (-b - sqrt_d) / (2.0f * a);
    f32 t1 = (-b + sqrt_d) / (2.0f * a);
    f32 t = (t0 > 0.0f) ? t0 : ((t1 > 0.0f) ? t1 : -1.0f);
    if (t < 0.0f) return false;
    out_dist = t;
    out_hit = {ray_origin.x + ray_dir.x * t, ray_origin.y + ray_dir.y * t, ray_origin.z + ray_dir.z * t};
    return true;
}

Color Planet::ground_color() const {
    switch (biome_) {
        case BiomeType::Mediterranean: return Color{0.18f, 0.48f, 0.72f, 1.0f};
        case BiomeType::Desert: return Color{0.82f, 0.65f, 0.38f, 1.0f};
        case BiomeType::Ice: return Color{0.78f, 0.88f, 0.98f, 1.0f};
        case BiomeType::Volcanic: return Color{0.25f, 0.12f, 0.10f, 1.0f};
        case BiomeType::GasGiant: return Color{0.35f, 0.72f, 0.82f, 1.0f};
    }
    return Color{0.5f, 0.5f, 0.5f, 1.0f};
}

Color Planet::atmosphere_color() const {
    switch (biome_) {
        case BiomeType::Mediterranean: return Color{0.25f, 0.65f, 0.98f, 0.75f};
        case BiomeType::Desert: return Color{0.92f, 0.72f, 0.45f, 0.60f};
        case BiomeType::Ice: return Color{0.60f, 0.85f, 1.0f, 0.80f};
        case BiomeType::Volcanic: return Color{0.95f, 0.35f, 0.15f, 0.65f};
        case BiomeType::GasGiant: return Color{0.45f, 0.85f, 0.95f, 0.90f};
    }
    return Color{0.3f, 0.6f, 0.9f, 0.5f};
}

Vec3 Planet::world_normal(u32 tile_key) const {
    return PlanetGrid::tile_normal(tile_key);
}

} // namespace dsp
