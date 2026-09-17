#pragma once

#include "core/platform.h"
#include "core/math.h"
#include "rhi/rhi.h"
#include "engine/texture.h"
#include "engine/sprite_batch.h"
#include <memory>
#include <vector>

namespace dsp {

using namespace aether;

enum class BlockType : u8 {
    Air = 0,
    Grass,
    Dirt,
    Stone,
    Cobblestone,
    Sand,
    Water,
    OakLog,
    OakLeaves,
    IronOre,
    CopperOre,
    CoalOre,
    TitaniumOre,
    SiliconOre,
    Glass,
    Brick,
    Count
};

struct BlockDef {
    const char* name = "";
    Color top_color{1.0f, 1.0f, 1.0f, 1.0f};
    Color side_color{1.0f, 1.0f, 1.0f, 1.0f};
    Color bottom_color{1.0f, 1.0f, 1.0f, 1.0f};
    bool is_solid = true;
    bool is_transparent = false;
    bool is_liquid = false;
    f32 hardness = 1.0f; // Mining time in seconds
};

class VoxelAtlas {
public:
    static void init(rhi::RHIDevice* device);
    static const BlockDef& get_def(BlockType type);
    static std::shared_ptr<rhi::RHITexture> texture() { return atlas_tex_; }

    // Render a solid 3D Minecraft voxel cube fixed in 3D world space
    static void render_cube(engine::SpriteBatch& batch,
                            const Vec3& block_center,
                            f32 block_size,
                            const Vec3& right_v,
                            const Vec3& up_v,
                            const Vec3& fwd_v,
                            BlockType type,
                            const Mat4& vp,
                            f32 aspect,
                            const Vec3& cam_eye,
                            const Vec3& sun_dir,
                            f32 base_layer = 0.0f,
                            f32 alpha = 1.0f,
                            const Vec3& scale = {1.0f, 1.0f, 1.0f});

    static void render_block(engine::SpriteBatch& batch,
                             const Vec3& block_center,
                             f32 block_size,
                             BlockType type,
                             const Mat4& vp,
                             f32 aspect,
                             const Vec3& cam_eye,
                             const Vec3& sun_dir,
                             f32 base_layer = 0.0f);

private:
    static void generate_atlas(rhi::RHIDevice* device);
    static std::shared_ptr<rhi::RHITexture> atlas_tex_;
    static std::vector<BlockDef> defs_;
    static bool initialized_;
};

} // namespace dsp
