#pragma once

#include "core/platform.h"
#include "engine/renderer2d.h"
#include "engine/timer.h"
#include "demo/dyson_sphere/universe.h"
#include "demo/dyson_sphere/mecha.h"
#include "demo/dyson_sphere/factory.h"
#include "demo/dyson_sphere/power.h"
#include "demo/dyson_sphere/tech_tree.h"
#include "demo/dyson_sphere/blueprint.h"
#include "demo/dyson_sphere/dsp_ui.h"
#include "demo/dyson_sphere/world3d.h"

namespace dsp {

using namespace aether;

class Game {
public:
    Game(engine::Renderer2D* renderer, engine::Timer* timer);
    ~Game();

    void update();
    void render();

    // 3D 世界坐标 → 2D 屏幕坐标 (中心原点, y 向下); 供静态绘制辅助使用
    struct Proj {
        Vec2 pos{0.0f, 0.0f};
        f32 depth = 1.0f;
        bool ok = false;
    };
    Proj project(const Vec3& world_p, const Mat4& vp, f32 aspect) const;

private:
    // --- 输入与建造 ---
    void handle_input(f32 dt);
    void update_cursor(const Vec3& ray_o, const Vec3& ray_d);
    void try_place(u32 tile_key, bool drag_chain);
    bool try_dismantle(u32 tile_key);
    void init_starting_factory();
    u32 find_spawn_tile();

    // --- 渲染分层 ---
    // Pass A (2D): 深空背景 + 恒星 → 3D pass: 地形/水面/大气/戴森球 (深度缓冲)
    // Pass B (2D): 工厂/机甲/矿脉/UI
    void render_space(engine::SpriteBatch& batch, const Mat4& vp, f32 aspect);
    void render_sky_dome(engine::SpriteBatch& batch, const Planet* planet);
    void render_sun(engine::SpriteBatch& batch, const Mat4& vp, f32 aspect);
    void render_dyson_sphere_3d(const Mat4& vp, f32 aspect);
    void ensure_terrain_meshes(Planet* planet);
    void rebuild_flat_view_mesh();
    void render_veins(engine::SpriteBatch& batch, const Mat4& vp, f32 aspect, const Planet* planet);
    void render_cursor_ghost(engine::SpriteBatch& batch, const Mat4& vp, f32 aspect, const Planet* planet);
    void render_factory(engine::SpriteBatch& batch, const Mat4& vp, f32 aspect, const Planet* planet);
    void render_mecha(engine::SpriteBatch& batch, const Mat4& vp, f32 aspect, const Planet* planet);
    void render_launch_fx(engine::SpriteBatch& batch, const Mat4& vp, f32 aspect);
    void render_lens_flare(engine::SpriteBatch& batch, f32 aspect);

    void update_launch_fx(f32 dt);

    engine::Renderer2D* renderer_ = nullptr;
    engine::Timer* timer_ = nullptr;

    World3D world3d_;
    Planet* built_planet_ = nullptr;   // 已构建地形网格的星球 (切换星球时重建)
    bool flat_view_ = false;           // G: 平面地形调试视图

    Universe universe_;
    Mecha mecha_;
    FactorySystem factory_;
    PowerGrid power_;
    TechTreeManager tech_tree_;
    BlueprintManager blueprints_;
    DSPUI ui_;

    // 光标
    u32 cursor_tile_ = 0;
    Vec3 cursor_world_pos_{0.0f, 0.0f, 0.0f};
    bool cursor_on_planet_ = false;
    i32 build_dir_ = 0;              // 0=+u 1=-u 2=+v 3=-v
    bool belt_dragging_ = false;     // 拖拽铺带
    u32 belt_last_tile_ = 0;
    f32 click_cooldown_ = 0.0f;
    f32 sprite_rot_ = 0.0f;          // 机甲精灵朝向 (对齐表面法线, 帧间平滑)

    // 太阳与昼夜
    Vec3 sun_pos_{-2200.0f, 900.0f, -2800.0f};
    Vec3 sun_dir_{1.0f, 0.0f, 0.0f};
    Vec2 sun_screen_{0.0f, 0.0f};
    bool sun_on_screen_ = false;
    f32 planet_rotation_seed_ = 0.7f;
    f32 elapsed_time_ = 0.0f;

    // 发射特效
    struct LaunchFx {
        Vec3 from{0.0f, 0.0f, 0.0f};
        Vec3 to{0.0f, 0.0f, 0.0f};
        f32 t = 0.0f;
        f32 dur = 3.0f;
        bool rocket = false;
    };
    std::vector<LaunchFx> launch_fx_;
    u32 prev_sails_ = 0;
    u32 prev_rockets_ = 0;

    // 星空缓存
    struct BgStar { f32 theta, phi, size, phase; u8 warm; };
    std::vector<BgStar> bg_stars_;
    bool stars_init_ = false;

    // 基础纹理
    std::shared_ptr<rhi::RHITexture> solid_tex_;
    std::shared_ptr<rhi::RHITexture> glow_tex_;
};

} // namespace dsp
