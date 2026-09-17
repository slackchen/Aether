#include "demo/dyson_sphere/voxel_world.h"
#include <cmath>
#include <algorithm>

namespace dsp {

static constexpr f32 kPi = 3.14159265358979323846f;
static constexpr f32 kTwoPi = kPi * 2.0f;
static constexpr f32 kHalfPi = kPi * 0.5f;

static bool project_pt(const Vec3& p, const Mat4& vp, f32 aspect, Vec2& out_screen, f32& out_depth) {
    f32 x = p.x * vp.m[0] + p.y * vp.m[4] + p.z * vp.m[8] + vp.m[12];
    f32 y = p.x * vp.m[1] + p.y * vp.m[5] + p.z * vp.m[9] + vp.m[13];
    f32 z = p.x * vp.m[2] + p.y * vp.m[6] + p.z * vp.m[10] + vp.m[14];
    f32 w = p.x * vp.m[3] + p.y * vp.m[7] + p.z * vp.m[11] + vp.m[15];

    if (w <= 0.1f) return false;

    f32 inv_w = 1.0f / w;
    f32 ndc_x = x * inv_w;
    f32 ndc_y = y * inv_w;

    if (ndc_x < -2.2f || ndc_x > 2.2f || ndc_y < -2.2f || ndc_y > 2.2f) return false;

    f32 half_h = 360.0f;
    f32 half_w = half_h * aspect;

    out_screen.x = ndc_x * half_w;
    out_screen.y = -ndc_y * half_h;
    out_depth = w;
    return true;
}

static f32 hash3d(f32 x, f32 y, f32 z) {
    f32 n = sinf(x * 127.1f + y * 311.7f + z * 74.7f) * 43758.5453f;
    return n - floorf(n);
}

static f32 noise3d(f32 x, f32 y, f32 z) {
    f32 ix = floorf(x); f32 iy = floorf(y); f32 iz = floorf(z);
    f32 fx = x - ix; f32 fy = y - iy; f32 fz = z - iz;

    fx = fx * fx * (3.0f - 2.0f * fx);
    fy = fy * fy * (3.0f - 2.0f * fy);
    fz = fz * fz * (3.0f - 2.0f * fz);

    f32 n000 = hash3d(ix, iy, iz);
    f32 n100 = hash3d(ix + 1.0f, iy, iz);
    f32 n010 = hash3d(ix, iy + 1.0f, iz);
    f32 n110 = hash3d(ix + 1.0f, iy + 1.0f, iz);
    f32 n001 = hash3d(ix, iy, iz + 1.0f);
    f32 n101 = hash3d(ix + 1.0f, iy, iz + 1.0f);
    f32 n011 = hash3d(ix, iy + 1.0f, iz + 1.0f);
    f32 n111 = hash3d(ix + 1.0f, iy + 1.0f, iz + 1.0f);

    f32 x00 = n000 * (1.0f - fx) + n100 * fx;
    f32 x10 = n010 * (1.0f - fx) + n110 * fx;
    f32 x01 = n001 * (1.0f - fx) + n101 * fx;
    f32 x11 = n011 * (1.0f - fx) + n111 * fx;

    f32 y0 = x00 * (1.0f - fy) + x10 * fy;
    f32 y1 = x01 * (1.0f - fy) + x11 * fy;

    return y0 * (1.0f - fz) + y1 * fz;
}

static f32 fbm3d(const Vec3& p, int octaves = 4) {
    f32 v = 0.0f;
    f32 a = 0.5f;
    Vec3 q = p;
    for (int i = 0; i < octaves; ++i) {
        v += a * noise3d(q.x, q.y, q.z);
        q.x = q.x * 2.0f + 1.3f;
        q.y = q.y * 2.0f + 1.7f;
        q.z = q.z * 2.0f + 1.9f;
        a *= 0.5f;
    }
    return v;
}

VoxelWorld::VoxelWorld(u32 seed) : seed_(seed) {}

void VoxelWorld::init(rhi::RHIDevice* device) {
    VoxelAtlas::init(device);
}

void VoxelWorld::evaluate_surface(const Vec3& unit_norm, f32& out_height, BlockType& out_type) const {
    f32 s = (f32)seed_ * 0.13f;
    Vec3 p = {unit_norm.x * 3.2f + s, unit_norm.y * 3.2f + s * 1.3f, unit_norm.z * 3.2f + s * 1.7f};

    f32 n_cont = fbm3d(p, 4);
    f32 n_hills = fbm3d({p.x * 2.5f, p.y * 2.5f, p.z * 2.5f}, 3);

    // Discrete Stepped Minecraft Voxel Height
    f32 raw_elev = (n_cont - 0.44f) * 32.0f + n_hills * 8.0f;
    constexpr f32 kVoxelStep = 2.0f; // 2m voxel terrace steps
    out_height = floorf(raw_elev / kVoxelStep) * kVoxelStep;

    f32 polar = fabsf(unit_norm.y);
    if (out_height < 0.0f) {
        out_type = BlockType::Water;
    } else if (out_height <= 2.0f) {
        out_type = BlockType::Sand;
    } else if (out_height <= 16.0f) {
        // Mineral Ore Clusters across grasslands & plains
        f32 n_ore = hash3d(unit_norm.x * 80.0f, unit_norm.y * 80.0f, unit_norm.z * 80.0f);
        if (n_ore > 0.96f) out_type = BlockType::TitaniumOre;
        else if (n_ore > 0.93f) out_type = BlockType::IronOre;
        else if (n_ore > 0.90f) out_type = BlockType::CopperOre;
        else if (n_ore > 0.87f) out_type = BlockType::CoalOre;
        else if (n_ore > 0.84f) out_type = BlockType::SiliconOre;
        else out_type = BlockType::Grass;
    } else {
        out_type = (out_height >= 26.0f) ? BlockType::Cobblestone : BlockType::Stone;
    }
}

f32 VoxelWorld::get_ground_height_at(const Vec3& pos, f32 planet_radius) const {
    f32 len = sqrtf(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
    if (len < 1.0f) return planet_radius;
    Vec3 norm = {pos.x / len, pos.y / len, pos.z / len};
    f32 h;
    BlockType b;
    evaluate_surface(norm, h, b);
    return planet_radius + std::max(0.0f, h);
}

void VoxelWorld::render_continuous_voxel_planet(engine::SpriteBatch& batch,
                                                const Vec3& planet_center,
                                                f32 planet_radius,
                                                f32 rotation_angle,
                                                const Vec3& player_pos,
                                                f32 altitude,
                                                const Mat4& vp,
                                                f32 aspect,
                                                const Vec3& cam_eye,
                                                const Vec3& sun_dir,
                                                std::shared_ptr<rhi::RHITexture> circle_tex,
                                                std::shared_ptr<rhi::RHITexture> glow_tex) {
    auto atlas = VoxelAtlas::texture();
    if (!atlas || !circle_tex) return;

    // 1. Solid Celestial Base Sphere (Deep Ocean Foundation in Space)
    Vec2 p_scr;
    f32 p_depth;
    if (project_pt(planet_center, vp, aspect, p_scr, p_depth)) {
        f32 scr_r = (planet_radius / p_depth) * 360.0f;
        batch.add(circle_tex, p_scr, {scr_r * 2.0f, scr_r * 2.0f}, Color{0.06f, 0.22f, 0.52f, 1.0f}, 0.0f, -700);
        if (glow_tex) {
            Color atmo{0.25f, 0.65f, 1.0f, 0.70f};
            batch.add(glow_tex, p_scr, {scr_r * 2.32f, scr_r * 2.32f}, atmo, 0.0f, -48, rhi::BlendMode::Additive);
        }
    }

    // 2. Global Planetary 3D Voxel Shell (LOD 2: Seamlessly covers 100% of the visible sphere)
    constexpr int kGlobeStacks = 36;
    constexpr int kGlobeSlices = 72;
    constexpr f32 kGlobeBlockSz = 52.0f;

    f32 cam_dist_center = sqrtf(cam_eye.x * cam_eye.x + cam_eye.y * cam_eye.y + cam_eye.z * cam_eye.z);
    Vec3 cam_dir_center = (cam_dist_center > 1.0f) ? Vec3{cam_eye.x / cam_dist_center, cam_eye.y / cam_dist_center, cam_eye.z / cam_dist_center} : Vec3{0.0f, 1.0f, 0.0f};
    f32 horizon_threshold = std::max(-0.20f, (planet_radius / std::max(planet_radius, cam_dist_center)) - 0.35f);

    for (int stack = 0; stack < kGlobeStacks; stack++) {
        f32 phi = -kHalfPi + ((f32)stack + 0.5f) * (kPi / (f32)kGlobeStacks);
        f32 y_norm = sinf(phi);
        f32 r_ring = cosf(phi);

        for (int slice = 0; slice < kGlobeSlices; slice++) {
            f32 theta = ((f32)slice + 0.5f) * (kTwoPi / (f32)kGlobeSlices);
            f32 x_norm = r_ring * sinf(theta);
            f32 z_norm = r_ring * cosf(theta);

            Vec3 norm_globe = {x_norm, y_norm, z_norm};

            // Only skip points on the exact far back of the planet relative to camera
            f32 dot_horizon = norm_globe.x * cam_dir_center.x + norm_globe.y * cam_dir_center.y + norm_globe.z * cam_dir_center.z;
            if (dot_horizon < -0.15f) continue;

            f32 v_height;
            BlockType b_type;
            evaluate_surface(norm_globe, v_height, b_type);

            Vec3 pt_globe = {
                planet_center.x + norm_globe.x * (planet_radius + v_height),
                planet_center.y + norm_globe.y * (planet_radius + v_height),
                planet_center.z + norm_globe.z * (planet_radius + v_height)
            };

            Vec3 ref_g = (fabsf(norm_globe.y) < 0.90f) ? Vec3{0.0f, 1.0f, 0.0f} : Vec3{0.0f, 0.0f, 1.0f};
            Vec3 t_e = {
                ref_g.y * norm_globe.z - ref_g.z * norm_globe.y,
                ref_g.z * norm_globe.x - ref_g.x * norm_globe.z,
                ref_g.x * norm_globe.y - ref_g.y * norm_globe.x
            };
            f32 te_l = sqrtf(t_e.x * t_e.x + t_e.y * t_e.y + t_e.z * t_e.z);
            if (te_l > 1e-4f) { t_e.x /= te_l; t_e.y /= te_l; t_e.z /= te_l; }
            else t_e = {1.0f, 0.0f, 0.0f};

            Vec3 t_n = {
                norm_globe.y * t_e.z - norm_globe.z * t_e.y,
                norm_globe.z * t_e.x - norm_globe.x * t_e.z,
                norm_globe.x * t_e.y - norm_globe.y * t_e.x
            };

            f32 d_cam = sqrtf((pt_globe.x - cam_eye.x) * (pt_globe.x - cam_eye.x) +
                              (pt_globe.y - cam_eye.y) * (pt_globe.y - cam_eye.y) +
                              (pt_globe.z - cam_eye.z) * (pt_globe.z - cam_eye.z));
            if (d_cam < 240.0f) continue;
            f32 layer = -300.0f - d_cam;

            VoxelAtlas::render_cube(batch, pt_globe, kGlobeBlockSz, t_e, norm_globe, t_n, b_type,
                                    vp, aspect, cam_eye, sun_dir, layer, 1.0f, {1.0f, 0.02f, 1.0f});
        }
    }

    // 3. Local Spherical Minecraft Voxel Terrain (LOD 0 & LOD 1: Continuous High-Res 3D World)
    f32 plen = sqrtf(player_pos.x * player_pos.x + player_pos.y * player_pos.y + player_pos.z * player_pos.z);
    Vec3 n_mecha = (plen > 1.0f) ? Vec3{player_pos.x / plen, player_pos.y / plen, player_pos.z / plen} : Vec3{0.0f, 1.0f, 0.0f};

    f32 p_theta = atan2f(n_mecha.z, n_mecha.x);
    f32 p_phi = asinf(std::clamp(n_mecha.y, -1.0f, 1.0f));

    // Smoothly project persistent_t_north_ onto the new normal plane to avoid 180-deg flip at poles
    f32 dot_n = persistent_t_north_.x * n_mecha.x + persistent_t_north_.y * n_mecha.y + persistent_t_north_.z * n_mecha.z;
    Vec3 proj_n = {
        persistent_t_north_.x - n_mecha.x * dot_n,
        persistent_t_north_.y - n_mecha.y * dot_n,
        persistent_t_north_.z - n_mecha.z * dot_n
    };
    f32 pn_len = sqrtf(proj_n.x * proj_n.x + proj_n.y * proj_n.y + proj_n.z * proj_n.z);
    
    if (pn_len > 1e-4f) {
        persistent_t_north_ = {proj_n.x / pn_len, proj_n.y / pn_len, proj_n.z / pn_len};
    } else {
        persistent_t_north_ = {1.0f, 0.0f, 0.0f}; // Fallback
    }

    Vec3 t_north = persistent_t_north_;
    Vec3 t_east = {
        n_mecha.y * t_north.z - n_mecha.z * t_north.y,
        n_mecha.z * t_north.x - n_mecha.x * t_north.z,
        n_mecha.x * t_north.y - n_mecha.y * t_north.x
    };

    // 3.1 Mid-Horizon Voxel Grid (LOD 1: 80m ~ 280m radius, 9m blocks)
    constexpr f32 kMidStep = 9.0f;
    constexpr int kMidExtent = 32;
    f32 step_lat_1 = kMidStep / planet_radius;
    int lat_cx_1 = (int)roundf(p_phi / step_lat_1);

    for (int dlat = -kMidExtent; dlat <= kMidExtent; dlat++) {
        f32 b_phi = (f32)(lat_cx_1 + dlat) * step_lat_1;
        b_phi = std::clamp(b_phi, -kHalfPi, kHalfPi);

        f32 r_cos = cosf(b_phi);
        f32 step_lon_1 = step_lat_1 / std::max(r_cos, 0.01f);
        int lon_cx_1 = (int)roundf(p_theta / step_lon_1);

        for (int dlon = -kMidExtent; dlon <= kMidExtent; dlon++) {
            f32 b_theta = (f32)(lon_cx_1 + dlon) * step_lon_1;

            Vec3 norm_block = {
                cosf(b_phi) * cosf(b_theta),
                sinf(b_phi),
                cosf(b_phi) * sinf(b_theta)
            };

            f32 d_mecha = sqrtf((planet_center.x + norm_block.x * planet_radius - player_pos.x) * (planet_center.x + norm_block.x * planet_radius - player_pos.x) +
                                (planet_center.y + norm_block.y * planet_radius - player_pos.y) * (planet_center.y + norm_block.y * planet_radius - player_pos.y) +
                                (planet_center.z + norm_block.z * planet_radius - player_pos.z) * (planet_center.z + norm_block.z * planet_radius - player_pos.z));
            if (d_mecha < 75.0f || d_mecha > 280.0f) continue;

            f32 v_height;
            BlockType b_type;
            evaluate_surface(norm_block, v_height, b_type);

            Vec3 pt_surface = {
                planet_center.x + norm_block.x * (planet_radius + v_height),
                planet_center.y + norm_block.y * (planet_radius + v_height),
                planet_center.z + norm_block.z * (planet_radius + v_height)
            };

            Vec3 b_east, b_north;
            f32 b_rxz = sqrtf(norm_block.x * norm_block.x + norm_block.z * norm_block.z);
            if (b_rxz > 1e-3f) {
                b_east = {-norm_block.z / b_rxz, 0.0f, norm_block.x / b_rxz};
                b_north = {
                    norm_block.y * b_east.z - norm_block.z * b_east.y,
                    norm_block.z * b_east.x - norm_block.x * b_east.z,
                    norm_block.x * b_east.y - norm_block.y * b_east.x
                };
            } else {
                b_east = {1.0f, 0.0f, 0.0f};
                b_north = {0.0f, 0.0f, 1.0f};
            }

            f32 d_cam = sqrtf((pt_surface.x - cam_eye.x) * (pt_surface.x - cam_eye.x) +
                              (pt_surface.y - cam_eye.y) * (pt_surface.y - cam_eye.y) +
                              (pt_surface.z - cam_eye.z) * (pt_surface.z - cam_eye.z));
            f32 block_layer = -200.0f - d_cam;

            VoxelAtlas::render_cube(batch, pt_surface, kMidStep, b_east, norm_block, b_north, b_type,
                                    vp, aspect, cam_eye, sun_dir, block_layer, 1.0f);
        }
    }

    // 3.2 Fine Player Voxel Grid (LOD 0: 0m ~ 95m radius, 3m fine blocks) & Trees
    constexpr f32 kStep = 3.0f;
    constexpr int kGridExtent = 32;
    f32 step_lat_0 = kStep / planet_radius;
    int lat_cx_0 = (int)roundf(p_phi / step_lat_0);

    for (int dlat = -kGridExtent; dlat <= kGridExtent; dlat++) {
        f32 b_phi = (f32)(lat_cx_0 + dlat) * step_lat_0;
        b_phi = std::clamp(b_phi, -kHalfPi, kHalfPi);

        f32 r_cos = cosf(b_phi);
        f32 step_lon_0 = step_lat_0 / std::max(r_cos, 0.01f);
        int lon_cx_0 = (int)roundf(p_theta / step_lon_0);

        for (int dlon = -kGridExtent; dlon <= kGridExtent; dlon++) {
            int abs_lon = lon_cx_0 + dlon;
            f32 b_theta = (f32)abs_lon * step_lon_0;

            Vec3 norm_block = {
                cosf(b_phi) * cosf(b_theta),
                sinf(b_phi),
                cosf(b_phi) * sinf(b_theta)
            };

            f32 d_mecha = sqrtf((planet_center.x + norm_block.x * planet_radius - player_pos.x) * (planet_center.x + norm_block.x * planet_radius - player_pos.x) +
                                (planet_center.y + norm_block.y * planet_radius - player_pos.y) * (planet_center.y + norm_block.y * planet_radius - player_pos.y) +
                                (planet_center.z + norm_block.z * planet_radius - player_pos.z) * (planet_center.z + norm_block.z * planet_radius - player_pos.z));
            if (d_mecha > 95.0f) continue;

            f32 v_height;
            BlockType b_type;
            evaluate_surface(norm_block, v_height, b_type);

            Vec3 pt_surface = {
                planet_center.x + norm_block.x * (planet_radius + v_height),
                planet_center.y + norm_block.y * (planet_radius + v_height),
                planet_center.z + norm_block.z * (planet_radius + v_height)
            };

            Vec3 b_east, b_north;
            f32 b_rxz = sqrtf(norm_block.x * norm_block.x + norm_block.z * norm_block.z);
            if (b_rxz > 1e-3f) {
                b_east = {-norm_block.z / b_rxz, 0.0f, norm_block.x / b_rxz};
                b_north = {
                    norm_block.y * b_east.z - norm_block.z * b_east.y,
                    norm_block.z * b_east.x - norm_block.x * b_east.z,
                    norm_block.x * b_east.y - norm_block.y * b_east.x
                };
            } else {
                b_east = {1.0f, 0.0f, 0.0f};
                b_north = {0.0f, 0.0f, 1.0f};
            }

            f32 d_cam = sqrtf((pt_surface.x - cam_eye.x) * (pt_surface.x - cam_eye.x) +
                              (pt_surface.y - cam_eye.y) * (pt_surface.y - cam_eye.y) +
                              (pt_surface.z - cam_eye.z) * (pt_surface.z - cam_eye.z));
            f32 block_layer = -100.0f - d_cam;

            VoxelAtlas::render_cube(batch, pt_surface, kStep, b_east, norm_block, b_north, b_type,
                                    vp, aspect, cam_eye, sun_dir, block_layer, 1.0f);

            // 3.3 Stationary Trees on Global Spherical Grid
            if (b_type == BlockType::Grass) {
                f32 hash_val = hash3d(norm_block.x * 133.7f, norm_block.y * 133.7f, norm_block.z * 133.7f);
                if (hash_val > 0.92f) { // ~8% chance for a tree
                    f32 tree_layer = -100.0f - d_cam;
                    
                    Vec3 pt_trunk = {
                        pt_surface.x + norm_block.x * (kStep * 0.75f),
                        pt_surface.y + norm_block.y * (kStep * 0.75f),
                        pt_surface.z + norm_block.z * (kStep * 0.75f)
                    };
                    VoxelAtlas::render_cube(batch, pt_trunk, kStep, b_east, norm_block, b_north, BlockType::OakLog,
                                            vp, aspect, cam_eye, sun_dir, tree_layer + 0.1f, 1.0f);

                    Vec3 pt_leaf = {
                        pt_surface.x + norm_block.x * (kStep * 1.8f),
                        pt_surface.y + norm_block.y * (kStep * 1.8f),
                        pt_surface.z + norm_block.z * (kStep * 1.8f)
                    };
                    VoxelAtlas::render_cube(batch, pt_leaf, kStep * 1.7f, b_east, norm_block, b_north, BlockType::OakLeaves,
                                            vp, aspect, cam_eye, sun_dir, tree_layer + 0.2f, 1.0f);
                }
            }
        }
    }
}

} // namespace dsp
