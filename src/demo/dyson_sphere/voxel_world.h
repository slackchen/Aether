#pragma once

#include "core/platform.h"
#include "core/math.h"
#include "demo/dyson_sphere/voxel.h"
#include <vector>
#include <memory>
#include <algorithm>
#include <cmath>

namespace dsp {

using namespace aether;

class VoxelWorld {
public:
    VoxelWorld(u32 seed = 1337);

    void init(rhi::RHIDevice* device);

    // Continuous spherical voxel height and block evaluator
    void evaluate_surface(const Vec3& unit_norm, f32& out_height, BlockType& out_type) const;

    f32 get_ground_height_at(const Vec3& pos, f32 planet_radius = 500.0f) const;

    // Render Continuous Minecraft-style 3D Spherical Voxel World
    void render_continuous_voxel_planet(engine::SpriteBatch& batch,
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
                                        std::shared_ptr<rhi::RHITexture> glow_tex);

private:
    u32 seed_ = 1337;
    Vec3 persistent_t_north_{0.0f, 0.0f, 1.0f};
};

} // namespace dsp
