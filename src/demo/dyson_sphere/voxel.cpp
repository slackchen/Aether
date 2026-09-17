#include "demo/dyson_sphere/voxel.h"
#include <algorithm>
#include <cmath>

namespace dsp {

std::shared_ptr<rhi::RHITexture> VoxelAtlas::atlas_tex_ = nullptr;
std::vector<BlockDef> VoxelAtlas::defs_;
bool VoxelAtlas::initialized_ = false;

static bool project_3d_point(const Vec3 &p, const Mat4 &vp, f32 aspect,
                             Vec2 &out_screen, f32 &out_depth) {
  f32 x = p.x * vp.m[0] + p.y * vp.m[4] + p.z * vp.m[8] + vp.m[12];
  f32 y = p.x * vp.m[1] + p.y * vp.m[5] + p.z * vp.m[9] + vp.m[13];
  f32 z = p.x * vp.m[2] + p.y * vp.m[6] + p.z * vp.m[10] + vp.m[14];
  f32 w = p.x * vp.m[3] + p.y * vp.m[7] + p.z * vp.m[11] + vp.m[15];

  if (w <= 0.01f)
    return false;

  f32 inv_w = 1.0f / w;
  f32 ndc_x = x * inv_w;
  f32 ndc_y = y * inv_w;

  if (ndc_x < -12.0f || ndc_x > 12.0f || ndc_y < -12.0f || ndc_y > 12.0f)
    return false;

  f32 half_h = 360.0f;
  f32 half_w = half_h * aspect;

  out_screen.x = ndc_x * half_w;
  out_screen.y = -ndc_y * half_h;
  out_depth = w;
  return true;
}

void VoxelAtlas::init(rhi::RHIDevice *device) {
  if (initialized_ && atlas_tex_)
    return;

  defs_.resize((size_t)BlockType::Count);

  defs_[(size_t)BlockType::Air] = {"空气",       {0, 0, 0, 0}, {0, 0, 0, 0},
                                   {0, 0, 0, 0}, false,        true,
                                   false,        0.0f};
  defs_[(size_t)BlockType::Grass] = {"草方块",
                                     {0.35f, 0.75f, 0.25f, 1.0f},
                                     {0.45f, 0.32f, 0.18f, 1.0f},
                                     {0.38f, 0.26f, 0.14f, 1.0f},
                                     true,
                                     false,
                                     false,
                                     0.6f};
  defs_[(size_t)BlockType::Dirt] = {"泥土",
                                    {0.45f, 0.32f, 0.18f, 1.0f},
                                    {0.45f, 0.32f, 0.18f, 1.0f},
                                    {0.38f, 0.26f, 0.14f, 1.0f},
                                    true,
                                    false,
                                    false,
                                    0.5f};
  defs_[(size_t)BlockType::Stone] = {"石头",
                                     {0.55f, 0.55f, 0.58f, 1.0f},
                                     {0.50f, 0.50f, 0.52f, 1.0f},
                                     {0.45f, 0.45f, 0.48f, 1.0f},
                                     true,
                                     false,
                                     false,
                                     1.5f};
  defs_[(size_t)BlockType::Cobblestone] = {"圆石",
                                           {0.48f, 0.48f, 0.50f, 1.0f},
                                           {0.42f, 0.42f, 0.45f, 1.0f},
                                           {0.38f, 0.38f, 0.40f, 1.0f},
                                           true,
                                           false,
                                           false,
                                           1.8f};
  defs_[(size_t)BlockType::Sand] = {"沙子",
                                    {0.86f, 0.82f, 0.58f, 1.0f},
                                    {0.82f, 0.78f, 0.52f, 1.0f},
                                    {0.78f, 0.74f, 0.48f, 1.0f},
                                    true,
                                    false,
                                    false,
                                    0.5f};
  defs_[(size_t)BlockType::Water] = {"水",
                                     {0.12f, 0.45f, 0.85f, 0.75f},
                                     {0.10f, 0.38f, 0.78f, 0.75f},
                                     {0.08f, 0.30f, 0.70f, 0.75f},
                                     false,
                                     true,
                                     true,
                                     0.0f};
  defs_[(size_t)BlockType::OakLog] = {"原木",
                                      {0.68f, 0.55f, 0.35f, 1.0f},
                                      {0.38f, 0.26f, 0.14f, 1.0f},
                                      {0.68f, 0.55f, 0.35f, 1.0f},
                                      true,
                                      false,
                                      false,
                                      2.0f};
  defs_[(size_t)BlockType::OakLeaves] = {"树叶",
                                         {0.20f, 0.65f, 0.18f, 0.90f},
                                         {0.18f, 0.58f, 0.16f, 0.90f},
                                         {0.15f, 0.50f, 0.14f, 0.90f},
                                         true,
                                         true,
                                         false,
                                         0.2f};
  defs_[(size_t)BlockType::IronOre] = {"铁矿石",
                                       {0.60f, 0.60f, 0.62f, 1.0f},
                                       {0.72f, 0.62f, 0.55f, 1.0f},
                                       {0.45f, 0.45f, 0.48f, 1.0f},
                                       true,
                                       false,
                                       false,
                                       3.0f};
  defs_[(size_t)BlockType::CopperOre] = {"铜矿石",
                                         {0.55f, 0.55f, 0.58f, 1.0f},
                                         {0.85f, 0.52f, 0.32f, 1.0f},
                                         {0.45f, 0.45f, 0.48f, 1.0f},
                                         true,
                                         false,
                                         false,
                                         2.5f};
  defs_[(size_t)BlockType::CoalOre] = {"煤矿石",
                                       {0.50f, 0.50f, 0.52f, 1.0f},
                                       {0.18f, 0.18f, 0.20f, 1.0f},
                                       {0.45f, 0.45f, 0.48f, 1.0f},
                                       true,
                                       false,
                                       false,
                                       2.0f};
  defs_[(size_t)BlockType::TitaniumOre] = {"钛矿石",
                                           {0.65f, 0.70f, 0.78f, 1.0f},
                                           {0.85f, 0.90f, 1.0f, 1.0f},
                                           {0.50f, 0.55f, 0.65f, 1.0f},
                                           true,
                                           false,
                                           false,
                                           4.0f};
  defs_[(size_t)BlockType::SiliconOre] = {"硅矿石",
                                          {0.55f, 0.55f, 0.60f, 1.0f},
                                          {0.55f, 0.85f, 0.95f, 1.0f},
                                          {0.45f, 0.45f, 0.50f, 1.0f},
                                          true,
                                          false,
                                          false,
                                          3.5f};
  defs_[(size_t)BlockType::Glass] = {"玻璃",
                                     {0.85f, 0.95f, 1.0f, 0.40f},
                                     {0.85f, 0.95f, 1.0f, 0.40f},
                                     {0.85f, 0.95f, 1.0f, 0.40f},
                                     true,
                                     true,
                                     false,
                                     0.3f};
  defs_[(size_t)BlockType::Brick] = {"红砖",
                                     {0.65f, 0.28f, 0.20f, 1.0f},
                                     {0.58f, 0.24f, 0.16f, 1.0f},
                                     {0.50f, 0.20f, 0.14f, 1.0f},
                                     true,
                                     false,
                                     false,
                                     2.0f};

  generate_atlas(device);
  initialized_ = true;
}

const BlockDef &VoxelAtlas::get_def(BlockType type) {
  if (!initialized_) {
    static BlockDef s_default;
    return s_default;
  }
  size_t idx = (size_t)type;
  if (idx >= defs_.size())
    return defs_[0];
  return defs_[idx];
}

void VoxelAtlas::generate_atlas(rhi::RHIDevice *device) {
  if (!device)
    return;

  constexpr u32 kAtlasW = 256;
  constexpr u32 kAtlasH = 256;

  atlas_tex_ = engine::make_texture(
      device, kAtlasW, kAtlasH, [](u32 px, u32 py, u8 out[4]) {
        u32 block_idx = (py / 16) * 16 + (px / 16);
        u32 lx = px % 16;
        u32 ly = py % 16;

        // Pixel art border & noise pattern
        f32 noise =
            ((f32)((px * 37 + py * 19 + block_idx * 53) % 17) - 8.0f) / 60.0f;
        bool is_border = (lx == 0 || lx == 15 || ly == 0 || ly == 15);

        Color base{0.5f, 0.5f, 0.5f, 1.0f};
        if (block_idx < (u32)defs_.size()) {
          base = defs_[block_idx].top_color;
        }

        f32 lum = 1.0f + noise;
        if (is_border)
          lum *= 0.85f;

        out[0] = (u8)(std::clamp(base.r * lum, 0.0f, 1.0f) * 255.0f);
        out[1] = (u8)(std::clamp(base.g * lum, 0.0f, 1.0f) * 255.0f);
        out[2] = (u8)(std::clamp(base.b * lum, 0.0f, 1.0f) * 255.0f);
        out[3] = (u8)(base.a * 255.0f);
      });
}

void VoxelAtlas::render_cube(engine::SpriteBatch &batch,
                             const Vec3 &block_center, f32 block_size,
                             const Vec3 &right_v, const Vec3 &up_v,
                             const Vec3 &fwd_v, BlockType type, const Mat4 &vp,
                             f32 aspect, const Vec3 &cam_eye,
                             const Vec3 &sun_dir, f32 base_layer, f32 alpha,
                             const Vec3 &scale) {
  if (type == BlockType::Air || alpha <= 0.01f)
    return;

  const auto &def = get_def(type);
  f32 h = block_size * 0.5f;

  // 8 vertices in true 3D world space, using the scale parameter
  Vec3 r = {right_v.x * h * scale.x, right_v.y * h * scale.x,
            right_v.z * h * scale.x};
  Vec3 u = {up_v.x * h * scale.y, up_v.y * h * scale.y, up_v.z * h * scale.y};
  Vec3 f = {fwd_v.x * h * scale.z, fwd_v.y * h * scale.z,
            fwd_v.z * h * scale.z};

  Vec3 v[8] = {
      {block_center.x - r.x - u.x - f.x, block_center.y - r.y - u.y - f.y,
       block_center.z - r.z - u.z - f.z}, // 0: -X -Y -Z
      {block_center.x + r.x - u.x - f.x, block_center.y + r.y - u.y - f.y,
       block_center.z + r.z - u.z - f.z}, // 1: +X -Y -Z
      {block_center.x + r.x - u.x + f.x, block_center.y + r.y - u.y + f.y,
       block_center.z + r.z - u.z + f.z}, // 2: +X -Y +Z
      {block_center.x - r.x - u.x + f.x, block_center.y - r.y - u.y + f.y,
       block_center.z - r.z - u.z + f.z}, // 3: -X -Y +Z
      {block_center.x - r.x + u.x - f.x, block_center.y - r.y + u.y - f.y,
       block_center.z - r.z + u.z - f.z}, // 4: -X +Y -Z
      {block_center.x + r.x + u.x - f.x, block_center.y + r.y + u.y - f.y,
       block_center.z + r.z + u.z - f.z}, // 5: +X +Y -Z
      {block_center.x + r.x + u.x + f.x, block_center.y + r.y + u.y + f.y,
       block_center.z + r.z + u.z + f.z}, // 6: +X +Y +Z
      {block_center.x - r.x + u.x + f.x, block_center.y - r.y + u.y + f.y,
       block_center.z - r.z + u.z + f.z}, // 7: -X +Y +Z
  };

  Vec2 p[8];
  f32 depths[8];
  bool valid[8];
  for (int i = 0; i < 8; i++) {
    valid[i] = project_3d_point(v[i], vp, aspect, p[i], depths[i]);
  }

  auto try_render_face = [&](const Vec2 &p0, const Vec2 &p1, const Vec2 &p2,
                             const Vec2 &p3, bool v0, bool v1, bool v2, bool v3,
                             const Color &col, f32 layer) {
    if (!v0 || !v1 || !v2 || !v3)
      return;
    batch.add_quad(atlas_tex_, p0, p1, p2, p3, col, layer);
  };

  auto is_front_facing = [&](const Vec3 &norm, f32 offset_sign) {
    Vec3 f_center = {block_center.x + norm.x * h * offset_sign,
                     block_center.y + norm.y * h * offset_sign,
                     block_center.z + norm.z * h * offset_sign};
    Vec3 to_cam = {cam_eye.x - f_center.x, cam_eye.y - f_center.y,
                   cam_eye.z - f_center.z};
    return (norm.x * to_cam.x * offset_sign + norm.y * to_cam.y * offset_sign +
            norm.z * to_cam.z * offset_sign) > 0.0f;
  };

  // Lighting factors
  f32 top_ndotl = std::clamp(
      (up_v.x * sun_dir.x + up_v.y * sun_dir.y + up_v.z * sun_dir.z) * 0.45f +
          0.65f,
      0.20f, 1.0f);
  Color top_col = def.top_color * top_ndotl;
  top_col.a = def.top_color.a * alpha;

  f32 fwd_ndotl = std::clamp(
      (fwd_v.x * sun_dir.x + fwd_v.y * sun_dir.y + fwd_v.z * sun_dir.z) *
              0.45f +
          0.55f,
      0.15f, 0.85f);
  Color fwd_col = def.side_color * fwd_ndotl;
  fwd_col.a = def.side_color.a * alpha;

  f32 back_ndotl = std::clamp(
      (-fwd_v.x * sun_dir.x - fwd_v.y * sun_dir.y - fwd_v.z * sun_dir.z) *
              0.45f +
          0.50f,
      0.15f, 0.85f);
  Color back_col = def.side_color * back_ndotl;
  back_col.a = def.side_color.a * alpha;

  f32 right_ndotl = std::clamp(
      (right_v.x * sun_dir.x + right_v.y * sun_dir.y + right_v.z * sun_dir.z) *
              0.45f +
          0.50f,
      0.12f, 0.80f);
  Color right_col = def.side_color * right_ndotl;
  right_col.a = def.side_color.a * alpha;

  f32 left_ndotl = std::clamp(
      (-right_v.x * sun_dir.x - right_v.y * sun_dir.y - right_v.z * sun_dir.z) *
              0.45f +
          0.50f,
      0.12f, 0.80f);
  Color left_col = def.side_color * left_ndotl;
  left_col.a = def.side_color.a * alpha;

  // 1. Top Face (+U: TL=7, TR=6, BL=4, BR=5)
  if (is_front_facing(up_v, 1.0f))
    try_render_face(p[7], p[6], p[4], p[5], valid[7], valid[6], valid[4],
                    valid[5], top_col, base_layer);

  // 2. Front Face (+F: TL=6, TR=7, BL=2, BR=3)
  if (is_front_facing(fwd_v, 1.0f))
    try_render_face(p[6], p[7], p[2], p[3], valid[6], valid[7], valid[2],
                    valid[3], fwd_col, base_layer - 0.01f);

  // 3. Back Face (-F: TL=4, TR=5, BL=0, BR=1)
  if (is_front_facing(fwd_v, -1.0f))
    try_render_face(p[4], p[5], p[0], p[1], valid[4], valid[5], valid[0],
                    valid[1], back_col, base_layer - 0.01f);

  // 4. Right Face (+R: TL=5, TR=6, BL=1, BR=2)
  if (is_front_facing(right_v, 1.0f))
    try_render_face(p[5], p[6], p[1], p[2], valid[5], valid[6], valid[1],
                    valid[2], right_col, base_layer - 0.02f);

  // 5. Left Face (-R: TL=7, TR=4, BL=3, BR=0)
  if (is_front_facing(right_v, -1.0f))
    try_render_face(p[7], p[4], p[3], p[0], valid[7], valid[4], valid[3],
                    valid[0], left_col, base_layer - 0.02f);
}

void VoxelAtlas::render_block(engine::SpriteBatch &batch,
                              const Vec3 &block_center, f32 block_size,
                              BlockType type, const Mat4 &vp, f32 aspect,
                              const Vec3 &cam_eye, const Vec3 &sun_dir,
                              f32 base_layer) {
  render_cube(batch, block_center, block_size, {1.0f, 0.0f, 0.0f},
              {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, type, vp, aspect, cam_eye,
              sun_dir, base_layer, 1.0f);
}

} // namespace dsp
