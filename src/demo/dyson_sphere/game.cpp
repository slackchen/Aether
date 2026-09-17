#include "demo/dyson_sphere/game.h"
#include "engine/texture.h"
#include "engine/input.h"
#include "engine/ui.h"
#include "demo/dyson_sphere/dsp_art.h"
#include "demo/dyson_sphere/dsp_audio.h"
#include "demo/dyson_sphere/planet_grid.h"
#include <cmath>
#include <algorithm>
#include <cstdio>

namespace dsp {

static constexpr f32 kPi = 3.14159265358979323846f;
static constexpr f32 kTwoPi = kPi * 2.0f;
static constexpr f32 kHalfPi = kPi * 0.5f;

static f32 clamp01(f32 v) { return std::clamp(v, 0.0f, 1.0f); }
static Color mul(Color c, f32 m) { return {c.r * m, c.g * m, c.b * m, c.a}; }

// 各建筑的世界尺寸 (米)
static f32 building_world_size(BuildingKind k) {
    switch (k) {
        case BuildingKind::ConveyorBelt: return 15.0f;
        case BuildingKind::Sorter: return 12.0f;
        case BuildingKind::TeslaTower: return 9.0f;
        case BuildingKind::WirelessPowerTower: return 11.0f;
        case BuildingKind::PlanetaryLogisticsStation: return 19.0f;
        case BuildingKind::InterstellarLogisticsStation: return 19.0f;
        case BuildingKind::VerticalLaunchSilo: return 17.0f;
        case BuildingKind::EMRailEjector: return 15.0f;
        default: return 13.0f;
    }
}

Game::Game(engine::Renderer2D* renderer, engine::Timer* timer)
    : renderer_(renderer), timer_(timer), universe_(2026) {
    if (renderer_ && renderer_->device()) {
        solid_tex_ = engine::make_solid_texture(renderer_->device(), 4, 4, {1.0f, 1.0f, 1.0f, 1.0f});
        glow_tex_ = engine::make_glow_texture(renderer_->device(), 128);
        dsp_art::init(renderer_->device());
        world3d_.init(renderer_->device(), renderer_->color_format(), renderer_->depth_format());
        ensure_terrain_meshes(universe_.current_planet());
    }
    DSPAudioManager::init();
    init_starting_factory();
    engine::ui::flash("欢迎来到伊卡洛斯 — 建造你的戴森球文明 (G: 平面/球面地形视图)", 4.0f);
}

Game::~Game() {
    dsp_art::shutdown();
}

void Game::ensure_terrain_meshes(Planet* planet) {
    if (!planet || planet == built_planet_ || !world3d_.ready()) return;
    built_planet_ = planet;
    const TerrainField& field = planet->terrain_field();
    f32 R = planet->radius();
    // 球形系统: 立方体球六面网格, 高度/配色来自同一 TerrainField (+Y 径向高度)
    world3d_.build_terrain_sphere(field, R, 96);
    world3d_.build_water_sphere(R, 48);          // 海平面 = R
    world3d_.build_atmosphere(R * 1.06f, 48);
}

// 平面调试视图: 在机甲脚下沿切平面展开同一块地形高度场 (G 键切换)
void Game::rebuild_flat_view_mesh() {
    Planet* planet = universe_.current_planet();
    if (!planet || !world3d_.ready()) return;
    Vec3 mpos = mecha_.position();
    f32 len = sqrtf(mpos.x * mpos.x + mpos.y * mpos.y + mpos.z * mpos.z);
    Vec3 up = len > 1.0f ? Vec3{mpos.x / len, mpos.y / len, mpos.z / len} : Vec3{0.0f, 1.0f, 0.0f};
    Vec3 ref = fabsf(up.y) > 0.92f ? Vec3{1.0f, 0.0f, 0.0f} : Vec3{0.0f, 1.0f, 0.0f};
    Vec3 east = {
        ref.y * up.z - ref.z * up.y,
        ref.z * up.x - ref.x * up.z,
        ref.x * up.y - ref.y * up.x
    };
    f32 el = sqrtf(east.x * east.x + east.y * east.y + east.z * east.z);
    east = {east.x / el, east.y / el, east.z / el};
    Vec3 north = {
        east.y * up.z - east.z * up.y,
        east.z * up.x - east.x * up.z,
        east.x * up.y - east.y * up.x
    };
    Vec3 center = {up.x * (len + 2.0f), up.y * (len + 2.0f), up.z * (len + 2.0f)};
    const TerrainField& field = planet->terrain_field();
    world3d_.build_terrain_planar(field, center, east, north, up, 1400.0f, 160);
    world3d_.build_water_planar(center, east, north, up, 1400.0f, 6.0f);
}

u32 Game::find_spawn_tile() {
    Planet* p = universe_.current_planet();
    if (!p) return 0;
    constexpr u32 N = PlanetGrid::kTilesPerFace;
    u32 cu = N / 2, cv = N / 2;
    for (u32 r = 0; r < N / 2; r++) {
        for (i32 du = -(i32)r; du <= (i32)r; du++) {
            for (i32 dv = -(i32)r; dv <= (i32)r; dv++) {
                if (std::max(abs(du), abs(dv)) != (i32)r) continue;
                i32 u = std::clamp((i32)cu + du, 0, (i32)N - 1);
                i32 v = std::clamp((i32)cv + dv, 0, (i32)N - 1);
                u32 key = PlanetGrid::key(2, (u8)u, (u8)v);
                if (p->is_buildable_tile(key)) return key;
            }
        }
    }
    return PlanetGrid::key(2, (u8)cu, (u8)cv);
}

void Game::init_starting_factory() {
    Planet* p = universe_.current_planet();
    if (!p) return;

    u32 spawn = find_spawn_tile();
    mecha_.set_position(p->grid().tile_center(p->radius(), spawn) +
                        PlanetGrid::tile_normal(spawn) * 1.5f);

    auto find_vein = [&](int resource) -> u32 {
        for (u32 r = 1; r < 24; r++) {
            for (int d = 0; d < 4; d++) {
                u32 k = spawn;
                for (u32 s = 0; s < r; s++) k = PlanetGrid::neighbor_key(k, d);
                const Tile& t = p->grid().tile(k);
                if ((int)t.resource == resource && t.resource_amount > 0 && p->is_buildable_tile(k)) return k;
            }
        }
        return 0;
    };
    u32 iron_tile = find_vein(1);

    auto place = [&](BuildingKind kind, u32 key, i32 dir, ItemKind recipe = ItemKind::None) -> u32 {
        if (!key || !p->is_buildable_tile(key)) return 0;
        GridPos gp = GridPos::from_key(key);
        Vec3 pos = p->grid().tile_center(p->radius(), key);
        u32 id = factory_.place_building(kind, gp, pos, (f32)dir * kHalfPi, recipe);
        if (id) {
            p->grid().tile(key).building_id = (u16)id;
            if (auto* b = factory_.get_building(id)) b->dir = dir;
        }
        return id;
    };

    place(BuildingKind::WirelessPowerTower, PlanetGrid::neighbor_key(spawn, 0), 0);
    place(BuildingKind::WindTurbine, PlanetGrid::neighbor_key(spawn, 2), 0);
    place(BuildingKind::WindTurbine, PlanetGrid::neighbor_key(spawn, 3), 0);
    place(BuildingKind::SolarPanel, PlanetGrid::neighbor_key(PlanetGrid::neighbor_key(spawn, 1), 2), 0);
    place(BuildingKind::SolarPanel, PlanetGrid::neighbor_key(PlanetGrid::neighbor_key(spawn, 1), 3), 0);

    if (iron_tile) {
        place(BuildingKind::MiningMachine, iron_tile, 0);
        u32 b1 = PlanetGrid::neighbor_key(iron_tile, 0);
        u32 b2 = PlanetGrid::neighbor_key(b1, 0);
        u32 b3 = PlanetGrid::neighbor_key(b2, 0);
        place(BuildingKind::ConveyorBelt, b1, 0);
        place(BuildingKind::ConveyorBelt, b2, 0);
        place(BuildingKind::ConveyorBelt, b3, 0);
        u32 smelt = PlanetGrid::neighbor_key(b3, 0);
        place(BuildingKind::ArcSmelter, smelt, 0, ItemKind::IronIngot);
        u32 out1 = PlanetGrid::neighbor_key(smelt, 0);
        place(BuildingKind::ConveyorBelt, out1, 0);
        u32 asm1 = PlanetGrid::neighbor_key(out1, 0);
        place(BuildingKind::AssemblingMachine, asm1, 0, ItemKind::MagneticCoil);
    }

    place(BuildingKind::MatrixLab, PlanetGrid::neighbor_key(PlanetGrid::neighbor_key(spawn, 0), 2), 0, ItemKind::MatrixBlue);
    place(BuildingKind::EMRailEjector, PlanetGrid::neighbor_key(PlanetGrid::neighbor_key(spawn, 0), 3), 0);
}

// ---------------------------------------------------------------------------
// 输入
// ---------------------------------------------------------------------------
void Game::handle_input(f32 dt) {
    click_cooldown_ = std::max(0.0f, click_cooldown_ - dt);
    Planet* planet = universe_.current_planet();
    DSPCamera& cam = universe_.camera();

    Vec2 mdelta = engine::Input::mouse_delta();
    f32 mwheel = engine::Input::mouse_wheel();
    bool r_down = engine::Input::is_mouse_down(engine::MouseButton::Right);
    cam.handle_input(mdelta, mwheel, r_down, false);

    Vec2 wasd{0.0f, 0.0f};
    if (engine::Input::is_key_down(engine::KeyCode::W) || engine::Input::is_down(engine::Key::Up)) wasd.y += 1.0f;
    if (engine::Input::is_key_down(engine::KeyCode::S) || engine::Input::is_down(engine::Key::Down)) wasd.y -= 1.0f;
    if (engine::Input::is_key_down(engine::KeyCode::A) || engine::Input::is_down(engine::Key::Left)) wasd.x -= 1.0f;
    if (engine::Input::is_key_down(engine::KeyCode::D) || engine::Input::is_down(engine::Key::Right)) wasd.x += 1.0f;

    f32 alt_input = 0.0f;
    if (engine::Input::is_key_down(engine::KeyCode::Space) || engine::Input::is_key_down(engine::KeyCode::E)) alt_input += 1.0f;
    if (engine::Input::is_key_down(engine::KeyCode::Q) || engine::Input::is_key_down(engine::KeyCode::Ctrl)) alt_input -= 1.0f;

    if (engine::Input::was_key_pressed(engine::KeyCode::Space) && mecha_.mode() == MechaMode::GroundWalk) {
        mecha_.toggle_flight();
        DSPAudioManager::play_click();
    }

    if (engine::Input::was_key_pressed(engine::KeyCode::G)) {
        flat_view_ = !flat_view_;
        if (flat_view_) {
            rebuild_flat_view_mesh();
            engine::ui::flash("平面地形调试视图: 山水高度场 (再按 G 返回星球)", 2.5f);
        } else {
            built_planet_ = nullptr;   // 球形网格被平面网格覆盖, 返回时重建
            ensure_terrain_meshes(universe_.current_planet());
            engine::ui::flash("球形视图", 1.5f);
        }
    }

    mecha_.move_3d(wasd, cam.right(), alt_input, dt, planet);

    Vec2 mpos = engine::Input::mouse_pos();
    Vec3 ray_o, ray_d;
    cam.screen_to_ray(mpos, (f32)renderer_->width(), (f32)renderer_->height(), ray_o, ray_d);
    update_cursor(ray_o, ray_d);

    if (engine::Input::was_key_pressed(engine::KeyCode::R)) build_dir_ = (build_dir_ + 1) & 3;

    bool ui_blocked = ui_.is_screen_blocked(mpos);
    BuildingKind tool = ui_.active_modal() == ActiveModal::None ? ui_.selected_build_tool() : BuildingKind::None;
    i32 bp_sel = ui_.selected_blueprint_index();

    if (cursor_on_planet_ && (engine::Input::was_key_pressed(engine::KeyCode::X) ||
                              engine::Input::was_mouse_pressed(engine::MouseButton::Middle))) {
        if (try_dismantle(cursor_tile_)) DSPAudioManager::play_dismantle();
    }

    bool l_pressed = engine::Input::was_mouse_pressed(engine::MouseButton::Left);
    bool l_down = engine::Input::is_mouse_down(engine::MouseButton::Left);
    bool l_released = engine::Input::was_mouse_released(engine::MouseButton::Left);

    if (l_released) belt_dragging_ = false;

    if (!ui_blocked && cursor_on_planet_ && l_pressed && click_cooldown_ <= 0.0f) {
        if (bp_sel >= 0 && bp_sel < (i32)blueprints_.presets().size()) {
            const auto& bp = blueprints_.presets()[bp_sel];
            u32 placed = blueprints_.paste_blueprint(bp, cursor_tile_, planet, factory_, mecha_);
            if (placed > 0) {
                DSPAudioManager::play_build();
                char msg[96];
                snprintf(msg, sizeof(msg), "蓝图铺设完成: %s (共 %u 座建筑)", bp.name.c_str(), placed);
                engine::ui::flash(msg, 2.0f);
                click_cooldown_ = 0.35f;
            }
        } else if (tool != BuildingKind::None) {
            try_place(cursor_tile_, false);
        }
    } else if (!ui_blocked && cursor_on_planet_ && l_down && tool == BuildingKind::ConveyorBelt &&
               belt_dragging_ && cursor_tile_ != belt_last_tile_ && click_cooldown_ <= 0.0f) {
        try_place(cursor_tile_, true);
    }

    if (engine::Input::was_key_pressed(engine::KeyCode::Escape) && ui_.active_modal() == ActiveModal::None) {
        ui_.clear_build_tool();
        ui_.clear_blueprint();
        belt_dragging_ = false;
    }
}

void Game::update_cursor(const Vec3& ray_o, const Vec3& ray_d) {
    Planet* planet = universe_.current_planet();
    cursor_on_planet_ = false;
    if (!planet) return;
    if (PlanetGrid::raycast(planet->radius(), ray_o, ray_d, cursor_world_pos_, cursor_tile_)) {
        cursor_on_planet_ = true;
    }
}

void Game::try_place(u32 tile_key, bool drag_chain) {
    Planet* planet = universe_.current_planet();
    if (!planet) return;
    BuildingKind tool = ui_.selected_build_tool();
    if (tool == BuildingKind::None) return;

    i32 dir = build_dir_;
    if (drag_chain) {
        TileCoord a = PlanetGrid::coord(belt_last_tile_);
        TileCoord b = PlanetGrid::coord(tile_key);
        if (a.face == b.face) {
            if (b.u == a.u + 1) dir = 0;
            else if (b.u == a.u - 1) dir = 1;
            else if (b.v == a.v + 1) dir = 2;
            else if (b.v == a.v - 1) dir = 3;
        }
    }

    if (!planet->is_buildable_tile(tile_key)) return;
    const Tile& t = planet->grid().tile(tile_key);
    if (tool == BuildingKind::MiningMachine && t.resource == 0) {
        engine::ui::flash("采矿机必须建在矿脉上", 1.2f);
        return;
    }
    if (tool == BuildingKind::OilExtractor && t.resource != (u8)ResourceKind::CrudeOil) {
        engine::ui::flash("抽油机必须建在原油矿脉上", 1.2f);
        return;
    }

    GridPos gp = GridPos::from_key(tile_key);
    Vec3 pos = planet->grid().tile_center(planet->radius(), tile_key);
    f32 rot = (f32)dir * kHalfPi;
    u32 id = factory_.place_building(tool, gp, pos, rot, ItemKind::None);
    if (id == 0) return;

    planet->grid().tile(tile_key).building_id = (u16)id;
    if (auto* b = factory_.get_building(id)) {
        b->dir = dir;
        if (tool == BuildingKind::Sorter) {
            b->src_key = PlanetGrid::neighbor_key(tile_key, (dir & 1) ? dir - 1 : dir + 1);
            b->dst_key = PlanetGrid::neighbor_key(tile_key, dir);
        }
    }
    mecha_.dispatch_drone(pos, id);
    DSPAudioManager::play_build();
    click_cooldown_ = 0.18f;

    if (tool == BuildingKind::ConveyorBelt) {
        belt_dragging_ = true;
        belt_last_tile_ = tile_key;
    }
}

bool Game::try_dismantle(u32 tile_key) {
    Planet* planet = universe_.current_planet();
    if (!planet) return false;
    u16 bid = planet->grid().tile(tile_key).building_id;
    if (bid == 0) return false;
    bool ok = factory_.remove_building(bid, planet);
    if (ok) engine::ui::flash("已拆除建筑", 0.8f);
    return ok;
}

// ---------------------------------------------------------------------------
// 更新
// ---------------------------------------------------------------------------
void Game::update() {
    f32 dt = timer_->delta();
    if (dt <= 0.0f || dt > 0.1f) dt = 0.0166f;
    elapsed_time_ += dt;

    Planet* planet = universe_.current_planet();
    Star* star = universe_.current_star();

    handle_input(dt);

    f32 sun_a = elapsed_time_ * 0.013f;
    f32 base_x = -2200.0f, base_z = -2800.0f;
    sun_pos_ = {base_x * cosf(sun_a) + base_z * sinf(sun_a), 900.0f,
                -base_x * sinf(sun_a) + base_z * cosf(sun_a)};
    f32 slen = sqrtf(sun_pos_.x * sun_pos_.x + sun_pos_.y * sun_pos_.y + sun_pos_.z * sun_pos_.z);
    sun_dir_ = {sun_pos_.x / slen, sun_pos_.y / slen, sun_pos_.z / slen};

    universe_.update(dt);
    mecha_.update(dt, planet, universe_.camera().current_scale() >= ViewScale::StarSystem);

    f32 mecha_energy = mecha_.energy_mj();
    f32 satisfaction = power_.satisfaction_ratio();
    factory_.update(dt, planet, satisfaction);

    power_.update(dt, factory_.buildings(), mecha_.position(), mecha_energy,
                  mecha_.max_energy_mj(), sun_dir_, elapsed_time_);
    mecha_.set_energy(mecha_energy);

    tech_tree_.update(dt, factory_.buildings());
    TechId newly_unlocked;
    if (tech_tree_.has_new_unlock_event(newly_unlocked)) {
        DSPAudioManager::play_tech_unlock();
        if (const auto* t = tech_tree_.get_tech(newly_unlocked)) {
            char unlock_buf[128];
            snprintf(unlock_buf, sizeof(unlock_buf), "科技研发完成: %s", t->name.c_str());
            engine::ui::flash(unlock_buf, 3.0f);
        }
    }

    if (star) {
        star->dyson_sphere.update(dt, factory_.launched_solar_sails(), factory_.launched_carrier_rockets());
    }

    u32 sails_now = factory_.launched_solar_sails();
    u32 rockets_now = factory_.launched_carrier_rockets();
    if (sails_now > prev_sails_) {
        for (const auto& b : factory_.buildings()) {
            if (b.kind == BuildingKind::EMRailEjector) {
                launch_fx_.push_back({b.world_pos, sun_pos_, 0.0f, 4.0f, false});
                break;
            }
        }
    }
    if (rockets_now > prev_rockets_) {
        for (const auto& b : factory_.buildings()) {
            if (b.kind == BuildingKind::VerticalLaunchSilo) {
                launch_fx_.push_back({b.world_pos, sun_pos_, 0.0f, 6.0f, true});
                break;
            }
        }
    }
    prev_sails_ = sails_now;
    prev_rockets_ = rockets_now;

    for (auto it = launch_fx_.begin(); it != launch_fx_.end(); ) {
        it->t += dt / it->dur;
        if (it->t >= 1.0f) it = launch_fx_.erase(it);
        else ++it;
    }

    Vec3 planet_pos = {0.0f, 0.0f, 0.0f};
    universe_.camera().update(dt, mecha_.position(), planet_pos, sun_pos_, mecha_.altitude());

    DSPAudioManager::update(dt, universe_.camera(), (f32)factory_.buildings().size());
}

// ---------------------------------------------------------------------------
// 投影辅助 (2D 精灵层使用)
// ---------------------------------------------------------------------------
Game::Proj Game::project(const Vec3& world_p, const Mat4& vp, f32 aspect) const {
    Proj out;
    f32 x = world_p.x * vp.m[0] + world_p.y * vp.m[4] + world_p.z * vp.m[8] + vp.m[12];
    f32 y = world_p.x * vp.m[1] + world_p.y * vp.m[5] + world_p.z * vp.m[9] + vp.m[13];
    f32 w = world_p.x * vp.m[3] + world_p.y * vp.m[7] + world_p.z * vp.m[11] + vp.m[15];
    if (w <= 0.1f) return out;
    f32 ndc_x = x / w, ndc_y = y / w;
    if (ndc_x < -3.0f || ndc_x > 3.0f || ndc_y < -3.0f || ndc_y > 3.0f) return out;
    out.pos.x = ndc_x * 360.0f * aspect;
    out.pos.y = -ndc_y * 360.0f;
    out.depth = w;
    out.ok = true;
    return out;
}

static f32 proj_scale(f32 fov_deg) {
    return 360.0f / tanf(fov_deg * 0.5f * kPi / 180.0f);
}

static f32 world_to_px(const Game&, f32 world_size, f32 depth, f32 fov_deg) {
    return world_size * proj_scale(fov_deg) / depth;
}

static Color planet_grid_vein_glow(u8 resource) {
    switch (resource) {
        case 1: return {0.4f, 0.6f, 0.9f, 1};
        case 2: return {0.95f, 0.55f, 0.25f, 1};
        case 3: return {0.5f, 0.5f, 0.55f, 1};
        case 4: return {0.8f, 0.8f, 0.75f, 1};
        case 5: return {0.7f, 0.85f, 1.0f, 1};
        case 6: return {0.4f, 0.9f, 0.95f, 1};
        default: return {0.7f, 0.3f, 0.9f, 1};
    }
}

static f32 screen_angle_of(const Game& game, const Vec3& pos, const Vec3& dir, const Mat4& vp, f32 aspect) {
    auto p1 = game.project(pos, vp, aspect);
    Vec3 p2w = {pos.x + dir.x * 2.0f, pos.y + dir.y * 2.0f, pos.z + dir.z * 2.0f};
    auto p2 = game.project(p2w, vp, aspect);
    if (!p1.ok || !p2.ok) return 0.0f;
    return atan2f(p2.pos.y - p1.pos.y, p2.pos.x - p1.pos.x);
}

static void line_3d(engine::SpriteBatch& batch, const Game& game,
                    const std::shared_ptr<rhi::RHITexture>& solid,
                    const Vec3& a, const Vec3& b, const Mat4& vp, f32 aspect,
                    f32 width_px, Color col, f32 layer, rhi::BlendMode blend = rhi::BlendMode::Alpha) {
    auto pa = game.project(a, vp, aspect);
    auto pb = game.project(b, vp, aspect);
    if (!pa.ok || !pb.ok) return;
    f32 dx = pb.pos.x - pa.pos.x, dy = pb.pos.y - pa.pos.y;
    f32 len = sqrtf(dx * dx + dy * dy);
    if (len < 0.5f) return;
    f32 ang = atan2f(dy, dx);
    batch.add(solid, {(pa.pos.x + pb.pos.x) * 0.5f, (pa.pos.y + pb.pos.y) * 0.5f},
              {len, width_px}, col, ang, layer, blend);
}

// ---------------------------------------------------------------------------
// 渲染主流程
//   Pass A (2D): 深空背景 + 恒星
//   3D pass:   地形 / 水面 / 大气 / 戴森球 (真实深度缓冲, 硬件遮挡)
//   Pass B (2D): 天空穹顶 + 工厂/机甲/矿脉/UI
// ---------------------------------------------------------------------------
void Game::render() {
    if (!renderer_ || !renderer_->begin_frame()) return;

    auto* enc = renderer_->encoder();
    auto& batch = renderer_->sprites();
    batch.clear();

    f32 aspect = renderer_->aspect();
    Mat4 vp3d = universe_.camera().view_projection(aspect);

    renderer_->camera().position = {0.0f, 0.0f};
    renderer_->camera().zoom = 1.0f;
    Mat4 vp2d = renderer_->camera().view_projection(aspect);

    Planet* planet = universe_.current_planet();
    Star* star = universe_.current_star();

    if (!stars_init_) {
        stars_init_ = true;
        for (int i = 0; i < 320; i++) {
            BgStar s;
            s.theta = (f32)i * 2.39996f;                       // 黄金角均匀分布
            s.phi = sinf((f32)i * 0.777f) * 1.35f;
            s.size = 1.2f + (f32)(i % 5) * 0.55f;
            s.phase = (f32)(i % 17) * 0.9f;
            s.warm = (u8)(i % 7);
            bg_stars_.push_back(s);
        }
    }

    // --- Pass A: 深空背景 + 恒星 (随后被 3D 地形正确遮挡) ---
    render_space(batch, vp3d, aspect);
    if (!flat_view_) render_sun(batch, vp3d, aspect);
    batch.render(enc, vp2d);
    batch.clear();

    // --- 3D pass: 地形/水面/大气/戴森球 ---
    if (world3d_.ready()) {
        if (planet) ensure_terrain_meshes(planet);
        world3d_.set_globals(vp3d, universe_.camera().eye(), sun_dir_, elapsed_time_);
        world3d_.draw_terrain(enc);
        world3d_.draw_water(enc);
        if (!flat_view_) {
            if (star) render_dyson_sphere_3d(vp3d, aspect);
            world3d_.draw_atmosphere(enc);
        }
    }

    // --- Pass B: 游戏层精灵 ---
    if (!flat_view_) render_sky_dome(batch, planet);
    if (planet) {
        render_veins(batch, vp3d, aspect, planet);
        render_factory(batch, vp3d, aspect, planet);
        render_cursor_ghost(batch, vp3d, aspect, planet);
        render_mecha(batch, vp3d, aspect, planet);
    }
    render_launch_fx(batch, vp3d, aspect);
    render_lens_flare(batch, aspect);

    // UI
    static DysonSphereManager s_fallback_dyson;
    DysonSphereManager& dyson = star ? star->dyson_sphere : s_fallback_dyson;
    ui_.update_and_render(renderer_, mecha_, planet, factory_, power_, dyson,
                          tech_tree_, blueprints_, universe_, sun_dir_, elapsed_time_);

    batch.render(enc, vp2d);
    renderer_->end_frame();
}

// ---------------------------------------------------------------------------
// 1) 深空背景: 星云 / 星空 / 银河
// ---------------------------------------------------------------------------
void Game::render_space(engine::SpriteBatch& batch, const Mat4& vp, f32 aspect) {
    (void)vp;
    f32 half_w = 360.0f * aspect;
    auto neb = dsp_art::tex_nebula();
    auto solid = solid_tex_;

    // 深空基底
    if (solid) {
        batch.add(solid, {0, 0}, {half_w * 2.4f, 780.0f}, Color{0.012f, 0.015f, 0.032f, 1.0f}, 0.0f, -200000.0f);
        batch.add(solid, {0, -half_w}, {half_w * 2.4f, 900.0f}, Color{0.008f, 0.010f, 0.024f, 1.0f}, 0.0f, -200000.0f);
    }

    // 视差星云 (随镜头缓慢偏移)
    const DSPCamera& cam = universe_.camera();
    Vec3 fwd = cam.forward();
    f32 yaw = atan2f(fwd.x, fwd.z);
    f32 pitch = asinf(clamp01(fwd.y));
    Vec2 par = {-yaw * 120.0f, pitch * 90.0f};

    if (neb) {
        batch.add(neb, {-260.0f + par.x, -130.0f + par.y}, {820.0f, 520.0f},
                  Color{0.55f, 0.35f, 0.95f, 0.55f}, 0.35f + elapsed_time_ * 0.004f, -198500.0f, rhi::BlendMode::Additive);
        batch.add(neb, {300.0f + par.x * 1.3f, 170.0f + par.y * 1.2f}, {700.0f, 460.0f},
                  Color{0.20f, 0.55f, 0.95f, 0.45f}, -0.5f + elapsed_time_ * 0.003f, -198400.0f, rhi::BlendMode::Additive);
        batch.add(neb, {-80.0f + par.x * 0.7f, 60.0f + par.y * 0.8f}, {980.0f, 620.0f},
                  Color{0.85f, 0.45f, 0.55f, 0.25f}, 1.1f + elapsed_time_ * 0.002f, -198300.0f, rhi::BlendMode::Additive);
    }

    // 星空
    auto star4 = dsp_art::tex_star4();
    for (const auto& s : bg_stars_) {
        f32 d = 18000.0f;
        Vec3 sp = {d * cosf(s.phi) * sinf(s.theta), d * sinf(s.phi), d * cosf(s.phi) * cosf(s.theta)};
        auto pr = project(sp, vp, aspect);
        if (!pr.ok) continue;
        f32 tw = 0.45f + 0.5f * sinf(elapsed_time_ * 2.2f + s.phase);
        Color c = s.warm == 0 ? Color{0.75f, 0.85f, 1.0f, tw} :
                  s.warm == 1 ? Color{1.0f, 0.85f, 0.6f, tw} : Color{0.95f, 0.97f, 1.0f, tw};
        batch.add(glow_tex_, pr.pos, {s.size * 2.4f, s.size * 2.4f}, c, 0.0f, -198000.0f, rhi::BlendMode::Additive);
        if (s.size > 3.2f && star4) {
            batch.add(star4, pr.pos, {s.size * 5.0f, s.size * 5.0f}, Color{c.r, c.g, c.b, tw * 0.8f}, 0.0f, -197990.0f, rhi::BlendMode::Additive);
        }
    }
}

// ---------------------------------------------------------------------------
// 2) 天空穹顶 (大气内)
// ---------------------------------------------------------------------------
void Game::render_sky_dome(engine::SpriteBatch& batch, const Planet* planet) {
    if (!planet || !solid_tex_) return;
    f32 aspect = renderer_->aspect();
    f32 half_w = 360.0f * aspect;

    f32 op = planet->atmospheric_opacity(mecha_.altitude());
    if (op < 0.01f) return;

    Vec3 up = {0.0f, 1.0f, 0.0f};
    f32 plen = sqrtf(mecha_.position().x * mecha_.position().x + mecha_.position().y * mecha_.position().y + mecha_.position().z * mecha_.position().z);
    if (plen > 1.0f) {
        up = {mecha_.position().x / plen, mecha_.position().y / plen, mecha_.position().z / plen};
    }
    Color sky = planet->sky_color(mecha_.altitude(), sun_dir_, up);
    sky.a *= 0.85f;
    if (sky.a > 0.02f) {
        batch.add(solid_tex_, {0.0f, 0.0f}, {half_w * 2.4f, 780.0f}, sky, 0.0f, -197000.0f);
    }

    // 地平线雾光
    if (mecha_.altitude() < 60.0f) {
        Color horizon = planet->horizon_fog_color(mecha_.altitude(), sun_dir_);
        horizon.a = horizon.a * clamp01(1.0f - mecha_.altitude() / 60.0f);
        if (horizon.a > 0.02f) {
            batch.add(glow_tex_, {0.0f, 200.0f}, {half_w * 2.2f, 380.0f}, horizon, 0.0f, -196900.0f, rhi::BlendMode::Additive);
        }
    }
}

// ---------------------------------------------------------------------------
// 3) 恒星
// ---------------------------------------------------------------------------
void Game::render_sun(engine::SpriteBatch& batch, const Mat4& vp, f32 aspect) {
    Star* s = universe_.current_star();
    if (!s || !glow_tex_) return;
    auto pr = project(sun_pos_, vp, aspect);
    if (!pr.ok) return;

    Color sc = star_color(s->type);
    f32 pulse = 1.0f + 0.05f * sinf(elapsed_time_ * 1.7f);
    f32 core = world_to_px(*this, s->radius * 2.0f, pr.depth, universe_.camera().fov_deg());
    core = std::clamp(core, 10.0f, 900.0f);

    auto flare = dsp_art::tex_flare();
    auto star4 = dsp_art::tex_star4();

    batch.add(glow_tex_, pr.pos, {core * 3.4f * pulse, core * 3.4f * pulse}, mul(sc, 0.35f), 0.0f, -196800.0f, rhi::BlendMode::Additive);
    if (star4) batch.add(star4, pr.pos, {core * 2.6f, core * 2.6f}, Color{sc.r, sc.g, sc.b, 0.9f}, 0.0f, -196790.0f, rhi::BlendMode::Additive);
    batch.add(glow_tex_, pr.pos, {core * 1.1f, core * 1.1f}, Color{1.0f, 0.98f, 0.9f, 1.0f}, 0.0f, -196780.0f, rhi::BlendMode::Additive);
    if (flare) {
        batch.add(flare, pr.pos, {core * 6.5f, core * 0.55f}, Color{sc.r, sc.g, sc.b, 0.55f}, 0.0f, -196770.0f, rhi::BlendMode::Additive);
        batch.add(flare, pr.pos, {core * 0.55f, core * 6.5f}, Color{sc.r, sc.g, sc.b, 0.30f}, kHalfPi, -196760.0f, rhi::BlendMode::Additive);
    }
    sun_screen_ = pr.pos;
    sun_on_screen_ = true;
}

// ---------------------------------------------------------------------------
// 4) 戴森球 (真 3D: 帆板三角网 + 骨架线 + 节点/太阳帆公告板,
//    全部走深度测试, 被星球正确遮挡)
// ---------------------------------------------------------------------------
static void push_sphere_tri(std::vector<World3D::Vertex>& out, Vec3 a, Vec3 b, Vec3 c,
                            Color col, int depth) {
    auto norm = [](const Vec3& v) {
        f32 l = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
        return Vec3{v.x / l, v.y / l, v.z / l};
    };
    if (depth <= 0) {
        out.push_back({a, a, col});
        out.push_back({b, b, col});
        out.push_back({c, c, col});
        return;
    }
    Vec3 ab = norm({a.x + b.x, a.y + b.y, a.z + b.z});
    Vec3 bc = norm({b.x + c.x, b.y + c.y, b.z + c.z});
    Vec3 ca = norm({c.x + a.x, c.y + a.y, c.z + a.z});
    push_sphere_tri(out, a, ab, ca, col, depth - 1);
    push_sphere_tri(out, ab, b, bc, col, depth - 1);
    push_sphere_tri(out, ca, bc, c, col, depth - 1);
    push_sphere_tri(out, ab, bc, ca, col, depth - 1);
}

static void push_billboard(std::vector<World3D::Vertex>& out, const Vec3& p,
                           const Vec3& right, const Vec3& up, f32 size, Color col) {
    Vec3 ru = {right.x * size, right.y * size, right.z * size};
    Vec3 uu = {up.x * size, up.y * size, up.z * size};
    Vec3 p0 = {p.x - ru.x - uu.x, p.y - ru.y - uu.y, p.z - ru.z - uu.z};
    Vec3 p1 = {p.x + ru.x - uu.x, p.y + ru.y - uu.y, p.z + ru.z - uu.z};
    Vec3 p2 = {p.x + ru.x + uu.x, p.y + ru.y + uu.y, p.z + ru.z + uu.z};
    Vec3 p3 = {p.x - ru.x + uu.x, p.y - ru.y + uu.y, p.z - ru.z + uu.z};
    Vec3 dummy{0.0f, 1.0f, 0.0f};
    out.push_back({p0, dummy, col});
    out.push_back({p1, dummy, col});
    out.push_back({p2, dummy, col});
    out.push_back({p0, dummy, col});
    out.push_back({p2, dummy, col});
    out.push_back({p3, dummy, col});
}

void Game::render_dyson_sphere_3d(const Mat4& vp, f32 aspect) {
    (void)vp;
    (void)aspect;
    Star* star = universe_.current_star();
    if (!star || !world3d_.ready()) return;
    const auto& dyson = star->dyson_sphere;

    static std::vector<World3D::Vertex> lines, tris;
    static bool reserved = false;
    if (!reserved) {
        lines.reserve(8192);
        tris.reserve(131072);
        reserved = true;
    }
    lines.clear();
    tris.clear();

    // 帆板: 完成度越高越亮
    for (const auto& p : dyson.panels()) {
        if (p.fill_ratio <= 0.0f) continue;
        if (p.node_indices[0] >= dyson.nodes().size() ||
            p.node_indices[1] >= dyson.nodes().size() ||
            p.node_indices[2] >= dyson.nodes().size()) continue;
        const Vec3& a = dyson.nodes()[p.node_indices[0]].pos;
        const Vec3& b = dyson.nodes()[p.node_indices[1]].pos;
        const Vec3& c = dyson.nodes()[p.node_indices[2]].pos;
        Color col = p.completed ? Color{0.30f, 0.70f, 1.0f, 0.28f}
                                : Color{0.55f, 0.75f, 0.95f, 0.05f + 0.20f * p.fill_ratio};
        static std::vector<World3D::Vertex> panel;
        panel.clear();
        push_sphere_tri(panel, a, b, c, col, 1);
        for (const auto& v : panel) {
            tris.push_back({{v.pos.x + sun_pos_.x, v.pos.y + sun_pos_.y, v.pos.z + sun_pos_.z},
                            v.normal, v.color});
        }
    }

    // 骨架连杆
    for (const auto& st : dyson.struts()) {
        if (st.node_a >= dyson.nodes().size() || st.node_b >= dyson.nodes().size()) continue;
        const auto& na = dyson.nodes()[st.node_a];
        const auto& nb = dyson.nodes()[st.node_b];
        Color col = st.completed ? Color{0.25f, 0.85f, 1.0f, 0.65f} : Color{0.45f, 0.55f, 0.65f, 0.30f};
        lines.push_back({{sun_pos_.x + na.pos.x, sun_pos_.y + na.pos.y, sun_pos_.z + na.pos.z},
                         {0.0f, 1.0f, 0.0f}, col});
        lines.push_back({{sun_pos_.x + nb.pos.x, sun_pos_.y + nb.pos.y, sun_pos_.z + nb.pos.z},
                         {0.0f, 1.0f, 0.0f}, col});
    }

    // 节点公告板
    const DSPCamera& cam = universe_.camera();
    for (const auto& n : dyson.nodes()) {
        Vec3 p = {sun_pos_.x + n.pos.x, sun_pos_.y + n.pos.y, sun_pos_.z + n.pos.z};
        Color nc = n.completed ? Color{0.3f, 0.95f, 1.0f, 0.9f} : Color{0.55f, 0.62f, 0.72f, 0.5f};
        push_billboard(tris, p, cam.right(), cam.up(), 16.0f, nc);
    }

    // 太阳帆蜂群
    for (const auto& s : dyson.sails()) {
        Vec3 p = {sun_pos_.x + s.pos.x, sun_pos_.y + s.pos.y, sun_pos_.z + s.pos.z};
        push_billboard(tris, p, cam.right(), cam.up(), 9.0f, Color{1.0f, 0.85f, 0.45f, 0.85f});
    }

    world3d_.upload_lines(lines.data(), (u32)lines.size());
    world3d_.upload_tris(tris.data(), (u32)tris.size());
    world3d_.draw_lines(renderer_->encoder());
    world3d_.draw_tris(renderer_->encoder());
}

// ---------------------------------------------------------------------------
// 5) 矿脉晶体
// ---------------------------------------------------------------------------
void Game::render_veins(engine::SpriteBatch& batch, const Mat4& vp, f32 aspect, const Planet* planet) {
    const PlanetGrid& grid = planet->grid();
    f32 R = planet->radius();
    f32 fov = universe_.camera().fov_deg();
    Vec3 eye = universe_.camera().eye();
    f32 eye_len = sqrtf(eye.x * eye.x + eye.y * eye.y + eye.z * eye.z);
    Vec3 eye_dir = eye_len > 1.0f ? Vec3{eye.x / eye_len, eye.y / eye_len, eye.z / eye_len} : Vec3{0.0f, 1.0f, 0.0f};
    auto vein_tex = dsp_art::tex_veins();
    if (!vein_tex) return;
    f32 horizon_cos = std::clamp(R / std::max(eye_len, 1.0f), 0.0f, 1.0f);

    f32 cam_dist = universe_.camera().zoom_distance();
    u32 stride = cam_dist > 900.0f ? 3 : 1; // 远景抽稀

    constexpr u32 VN = PlanetGrid::kTilesPerFace;
    for (u32 face = 0; face < PlanetGrid::kFaceCount; face++) {
        for (u32 u = 0; u < VN; u++) {
            for (u32 v = 0; v < VN; v++) {
                u32 key = PlanetGrid::key((u8)face, (u8)u, (u8)v);
                const Tile& t = grid.tile(key);
                if (t.resource == 0) continue;
                if (stride > 1 && ((key * 2654435761u) % stride) != 0) continue;
                Vec3 n = PlanetGrid::tile_normal(key);
                if (n.x * eye_dir.x + n.y * eye_dir.y + n.z * eye_dir.z < horizon_cos - 0.005f) continue;

                Vec3 pos = grid.tile_center(R, key) + n * 1.2f;
                auto pr = project(pos, vp, aspect);
                if (!pr.ok) continue;
                f32 world_sz = 9.0f + (f32)t.richness * 2.5f;
                f32 px = std::clamp(world_to_px(*this, world_sz, pr.depth, fov), 3.0f, 64.0f);

                Vec2 uv0, uv1;
                dsp_art::cell_uv(8, 1, (int)t.resource - 1, uv0, uv1);
                f32 rot = (f32)(t.tint % 8) * kPi * 0.25f;
                f32 deplete = t.resource_amount > 0 ? clamp01((f32)t.resource_amount / 600000.0f) : 0.0f;
                f32 sz = px * (0.55f + 0.45f * deplete);
                Color light = mul(Color{1, 1, 1, 1}, 0.45f + 0.6f * std::max(0.0f, n.x * sun_dir_.x + n.y * sun_dir_.y + n.z * sun_dir_.z));
                light.a = 1.0f;
                batch.add_uv(vein_tex, uv0, uv1, pr.pos, {sz, sz}, light, rot, -60.0f);
                if (glow_tex_) {
                    Color gc = planet_grid_vein_glow(t.resource);
                    batch.add(glow_tex_, pr.pos, {sz * 1.6f, sz * 1.6f}, Color{gc.r, gc.g, gc.b, 0.30f * deplete}, 0.0f, -59.0f, rhi::BlendMode::Additive);
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// 6) 光标与建造预览
// ---------------------------------------------------------------------------
void Game::render_cursor_ghost(engine::SpriteBatch& batch, const Mat4& vp, f32 aspect, const Planet* planet) {
    if (!cursor_on_planet_ || !solid_tex_) return;
    const PlanetGrid& grid = planet->grid();
    f32 R = planet->radius();
    BuildingKind tool = ui_.selected_build_tool();
    i32 bp_sel = ui_.selected_blueprint_index();

    Vec3 n = PlanetGrid::tile_normal(cursor_tile_);
    Vec3 pos = grid.tile_center(R, cursor_tile_);

    // 瓦片高亮框
    Vec3 e, nt;
    PlanetGrid::tile_basis(cursor_tile_, e, nt);
    f32 half = R * PlanetGrid::kStep * 0.5f * (kPi * 0.5f) * 0.92f;
    Vec3 c[4] = {
        pos + e * half + nt * half,
        pos - e * half + nt * half,
        pos - e * half - nt * half,
        pos + e * half - nt * half
    };
    bool occupied = grid.tile(cursor_tile_).building_id != 0;
    Color ring = occupied ? Color{1.0f, 0.45f, 0.3f, 0.85f} : Color{0.25f, 0.95f, 1.0f, 0.85f};
    for (int i = 0; i < 4; i++) {
        Vec3 lift = n * 0.6f;
        line_3d(batch, *this, solid_tex_, c[i] + lift, c[(i + 1) & 3] + lift, vp, aspect, 2.0f, ring, -55.0f);
    }

    // 建筑幽灵
    if (tool != BuildingKind::None && (int)tool < (int)BuildingKind::Count) {
        bool placeable = planet->is_buildable_tile(cursor_tile_);
        const Tile& t = grid.tile(cursor_tile_);
        if (tool == BuildingKind::MiningMachine && t.resource == 0) placeable = false;
        if (tool == BuildingKind::OilExtractor && t.resource != (u8)ResourceKind::CrudeOil) placeable = false;

        auto btex = dsp_art::tex_buildings();
        if (btex) {
            int idx = (int)tool - 1;
            Vec2 uv0, uv1;
            dsp_art::cell_uv(6, 6, idx, uv0, uv1);
            Vec3 facing;
            {
                Vec3 nb = grid.tile_center(R, PlanetGrid::neighbor_key(cursor_tile_, build_dir_));
                facing = {nb.x - pos.x, nb.y - pos.y, nb.z - pos.z};
                f32 fl = sqrtf(facing.x * facing.x + facing.y * facing.y + facing.z * facing.z);
                if (fl > 1e-4f) { facing.x /= fl; facing.y /= fl; facing.z /= fl; }
            }
            f32 ang = screen_angle_of(*this, pos, facing, vp, aspect);
            auto pr = project(pos + n * 2.0f, vp, aspect);
            if (pr.ok) {
                f32 sz = building_world_size(tool) * proj_scale(universe_.camera().fov_deg()) / pr.depth;
                sz = std::clamp(sz, 6.0f, 240.0f);
                Color gc = placeable ? Color{0.45f, 1.0f, 0.6f, 0.60f} : Color{1.0f, 0.35f, 0.3f, 0.60f};
                batch.add_uv(btex, uv0, uv1, pr.pos, {sz, sz}, gc, ang, -50.0f);
            }
        }

        // 电力塔供电范围
        if (tool == BuildingKind::TeslaTower || tool == BuildingKind::WirelessPowerTower) {
            f32 radius = (tool == BuildingKind::TeslaTower) ? 12.0f : 22.0f;
            Vec3 e2, n2;
            PlanetGrid::tile_basis(cursor_tile_, e2, n2);
            Vec3 prev = pos + e2 * radius + n * 1.0f;
            for (int i = 1; i <= 26; i++) {
                f32 a = (f32)i / 26.0f * kTwoPi;
                Vec3 p = pos + (e2 * cosf(a) + n2 * sinf(a)) * radius + n * 1.0f;
                line_3d(batch, *this, solid_tex_, prev, p, vp, aspect, 1.4f, Color{0.3f, 0.9f, 1.0f, 0.5f}, -54.0f, rhi::BlendMode::Additive);
                prev = p;
            }
        }
    }

    // 蓝图范围预览
    if (bp_sel >= 0 && bp_sel < (i32)blueprints_.presets().size()) {
        const auto& bp = blueprints_.presets()[bp_sel];
        for (const auto& item : bp.items) {
            TileCoord oc = PlanetGrid::coord(cursor_tile_);
            constexpr i32 NN = (i32)PlanetGrid::kTilesPerFace;
            i32 uu = std::clamp(oc.u + item.dx, 0, NN - 1);
            i32 vv = std::clamp(oc.v + item.dy, 0, NN - 1);
            u32 key = PlanetGrid::key(oc.face, (u8)uu, (u8)vv);
            Vec3 p = grid.tile_center(R, key);
            auto ppr = project(p, vp, aspect);
            if (!ppr.ok) continue;
            Color bc = planet->is_buildable_tile(key) ? Color{0.3f, 0.9f, 1.0f, 0.5f} : Color{1.0f, 0.4f, 0.3f, 0.5f};
            batch.add(glow_tex_, ppr.pos, {6.0f, 6.0f}, bc, 0.0f, -53.0f, rhi::BlendMode::Additive);
        }
    }

    // 拆除高亮
    if (grid.tile(cursor_tile_).building_id != 0) {
        Vec3 lift = pos + n * 3.0f;
        auto lpr = project(lift, vp, aspect);
        if (lpr.ok) {
            batch.add(glow_tex_, lpr.pos, {18.0f, 18.0f}, Color{1.0f, 0.3f, 0.25f, 0.5f}, elapsed_time_ * 2.0f, -52.0f, rhi::BlendMode::Additive);
        }
    }
}

// ---------------------------------------------------------------------------
// 7) 工厂建筑 / 传送带 / 物流
// ---------------------------------------------------------------------------
void Game::render_factory(engine::SpriteBatch& batch, const Mat4& vp, f32 aspect, const Planet* planet) {
    const PlanetGrid& grid = planet->grid();
    f32 R = planet->radius();
    f32 fov = universe_.camera().fov_deg();
    f32 pscale = proj_scale(fov);
    Vec3 eye = universe_.camera().eye();
    f32 eye_len = sqrtf(eye.x * eye.x + eye.y * eye.y + eye.z * eye.z);
    Vec3 eye_dir = eye_len > 1.0f ? Vec3{eye.x / eye_len, eye.y / eye_len, eye.z / eye_len} : Vec3{0.0f, 1.0f, 0.0f};
    auto btex = dsp_art::tex_buildings();
    auto items = dsp_art::tex_items();
    auto fx = dsp_art::tex_fx();
    if (!btex) return;
    bool show_bars = universe_.camera().zoom_distance() < 420.0f;
    f32 horizon_cos = std::clamp(R / std::max(eye_len, 1.0f), 0.0f, 1.0f);

    const auto& list = factory_.buildings();

    // 电力弧线
    for (size_t i = 0; i < list.size(); i++) {
        if (list[i].kind != BuildingKind::TeslaTower && list[i].kind != BuildingKind::WirelessPowerTower) continue;
        for (size_t j = i + 1; j < list.size(); j++) {
            if (list[j].kind != BuildingKind::TeslaTower && list[j].kind != BuildingKind::WirelessPowerTower) continue;
            Vec3 d = {list[j].world_pos.x - list[i].world_pos.x, list[j].world_pos.y - list[i].world_pos.y, list[j].world_pos.z - list[i].world_pos.z};
            f32 dl = sqrtf(d.x * d.x + d.y * d.y + d.z * d.z);
            if (dl > 55.0f) continue;
            Vec3 a = list[i].world_pos + PlanetGrid::tile_normal(list[i].tile_key) * 6.0f;
            Vec3 b = list[j].world_pos + PlanetGrid::tile_normal(list[j].tile_key) * 6.0f;
            line_3d(batch, *this, solid_tex_, a, b, vp, aspect, 1.5f, Color{0.25f, 0.9f, 1.0f, 0.5f}, -11.0f, rhi::BlendMode::Additive);
            // 能量脉冲
            f32 t = fmodf(elapsed_time_ * 0.7f + (f32)i * 0.13f, 1.0f);
            Vec3 pulse = {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t};
            auto pp = project(pulse, vp, aspect);
            if (pp.ok) batch.add(glow_tex_, pp.pos, {5.0f, 5.0f}, Color{0.5f, 1.0f, 1.0f, 0.8f}, 0.0f, -10.0f, rhi::BlendMode::Additive);
        }
    }

    // 建筑
    for (const auto& b : list) {
        Vec3 n = PlanetGrid::tile_normal(b.tile_key);
        if (n.x * eye_dir.x + n.y * eye_dir.y + n.z * eye_dir.z < horizon_cos - 0.02f) continue;
        auto pr = project(b.world_pos + n * 1.5f, vp, aspect);
        if (!pr.ok) continue;

        f32 world_sz = building_world_size(b.kind);
        f32 sz = world_sz * pscale / pr.depth;
        if (sz < 1.5f) continue;
        sz = std::min(sz, 260.0f);

        // 地面阴影
        auto spr = project(b.world_pos + n * 0.2f, vp, aspect);
        if (spr.ok) {
            batch.add(glow_tex_, spr.pos, {sz * 0.8f, sz * 0.8f}, Color{0.0f, 0.0f, 0.0f, 0.30f}, 0.0f, -40.0f);
        }

        // 朝向角
        Vec3 nb = grid.tile_center(R, PlanetGrid::neighbor_key(b.tile_key, b.dir));
        Vec3 facing = {nb.x - b.world_pos.x, nb.y - b.world_pos.y, nb.z - b.world_pos.z};
        f32 fl = sqrtf(facing.x * facing.x + facing.y * facing.y + facing.z * facing.z);
        if (fl > 1e-4f) { facing.x /= fl; facing.y /= fl; facing.z /= fl; }
        f32 ang = screen_angle_of(*this, b.world_pos, facing, vp, aspect);

        int idx = (int)b.kind - 1;
        Vec2 uv0, uv1;
        dsp_art::cell_uv(6, 6, idx, uv0, uv1);
        Color light = mul(Color{1, 1, 1, 1}, 0.55f + 0.5f * std::max(0.0f, n.x * sun_dir_.x + n.y * sun_dir_.y + n.z * sun_dir_.z));
        light.a = 1.0f;
        batch.add_uv(btex, uv0, uv1, pr.pos, {sz, sz}, light, ang, -25.0f);

        // 工作状态灯
        bool is_machine = b.kind == BuildingKind::ArcSmelter || b.kind == BuildingKind::AssemblingMachine ||
                          b.kind == BuildingKind::ChemicalPlant || b.kind == BuildingKind::MatrixLab ||
                          b.kind == BuildingKind::MiningMachine;
        if (glow_tex_ && is_machine) {
            bool working = b.progress > 0.0f && b.progress < 1.0f;
            Color lamp = working ? Color{0.25f, 1.0f, 0.5f, 0.6f + 0.3f * sinf(b.anim_timer * 6.0f)} :
                         b.powered ? Color{0.2f, 0.7f, 1.0f, 0.4f} : Color{1.0f, 0.25f, 0.2f, 0.6f};
            batch.add(glow_tex_, pr.pos, {sz * 0.5f, sz * 0.5f}, lamp, 0.0f, -24.0f, rhi::BlendMode::Additive);
        }
        // 熔炉火光
        if (glow_tex_ && b.kind == BuildingKind::ArcSmelter && b.progress > 0.0f) {
            f32 fl2 = 0.5f + 0.5f * sinf(b.anim_timer * 9.0f);
            batch.add(glow_tex_, pr.pos, {sz * 0.55f, sz * 0.55f}, Color{1.0f, 0.45f, 0.12f, 0.4f + 0.3f * fl2}, 0.0f, -23.0f, rhi::BlendMode::Additive);
        }

        // 传送带 + 货物
        if (b.kind == BuildingKind::ConveyorBelt && items) {
            for (const auto& it : b.belt_items) {
                if (it.kind == ItemKind::None) continue;
                f32 off = (it.progress - 0.5f) * (R * PlanetGrid::kStep * 1.35f);
                Vec3 ip = b.world_pos + facing * off + n * 0.9f;
                auto ipr = project(ip, vp, aspect);
                if (!ipr.ok) continue;
                Vec2 iuv0, iuv1;
                dsp_art::cell_uv(8, 8, (int)it.kind, iuv0, iuv1);
                f32 isz = std::clamp(4.2f * pscale / pr.depth, 2.0f, 30.0f);
                batch.add_uv(items, iuv0, iuv1, ipr.pos, {isz, isz}, Color{1, 1, 1, 1}, 0.0f, -15.0f);
            }
        }

        // 分拣器臂
        if (b.kind == BuildingKind::Sorter) {
            Building* src = factory_.building_at_tile(b.src_key);
            Building* dst = factory_.building_at_tile(b.dst_key);
            if (src && dst) {
                f32 t = b.sorter_arm_progress;
                Vec3 arm = {src->world_pos.x + (dst->world_pos.x - src->world_pos.x) * t,
                            src->world_pos.y + (dst->world_pos.y - src->world_pos.y) * t,
                            src->world_pos.z + (dst->world_pos.z - src->world_pos.z) * t};
                line_3d(batch, *this, solid_tex_, b.world_pos + n * 1.5f, arm + n * 1.5f, vp, aspect, 2.0f,
                        Color{0.9f, 0.8f, 0.4f, 0.85f}, -14.0f);
                if (b.sorter_held_item.kind != ItemKind::None && items) {
                    auto apr = project(arm + n * 2.0f, vp, aspect);
                    if (apr.ok) {
                        Vec2 iuv0, iuv1;
                        dsp_art::cell_uv(8, 8, (int)b.sorter_held_item.kind, iuv0, iuv1);
                        f32 isz = std::clamp(4.0f * pscale / apr.depth, 2.0f, 26.0f);
                        batch.add_uv(items, iuv0, iuv1, apr.pos, {isz, isz}, Color{1, 1, 1, 1}, 0.0f, -14.0f);
                    }
                }
            }
        }

        // 进度条
        if (show_bars && is_machine && b.progress > 0.01f) {
            f32 bar_w = std::clamp(sz * 0.75f, 14.0f, 60.0f);
            Vec2 bp = {pr.pos.x - bar_w * 0.5f, pr.pos.y - sz * 0.62f};
            batch.add(solid_tex_, {bp.x + bar_w * 0.5f, bp.y + 2.0f}, {bar_w + 2.0f, 5.0f}, Color{0.05f, 0.07f, 0.10f, 0.8f}, 0.0f, -12.0f);
            batch.add(solid_tex_, {bp.x + bar_w * 0.5f * b.progress, bp.y + 2.0f}, {bar_w * b.progress, 3.0f},
                      Color{0.25f, 0.95f, 1.0f, 0.95f}, 0.0f, -12.0f);
        }
    }

    // 物流飞船
    for (const auto& s : factory_.active_ships()) {
        auto pr = project(s.current_pos, vp, aspect);
        if (!pr.ok) continue;
        f32 px = std::clamp(8.0f * pscale / pr.depth, 3.0f, 40.0f);
        if (fx) {
            batch.add_uv(fx, {0.75f, 0.0f}, {1.0f, 0.5f}, pr.pos, {px, px * 0.6f}, Color{1, 1, 1, 1}, 0.0f, -3.0f);
        }
        batch.add(glow_tex_, pr.pos, {px * 1.8f, px * 1.8f}, Color{0.25f, 0.8f, 1.0f, 0.5f}, 0.0f, -4.0f, rhi::BlendMode::Additive);
    }
}

// ---------------------------------------------------------------------------
// 8) 机甲与无人机
// ---------------------------------------------------------------------------
void Game::render_mecha(engine::SpriteBatch& batch, const Mat4& vp, f32 aspect, const Planet* planet) {
    (void)planet;
    f32 fov = universe_.camera().fov_deg();
    f32 pscale = proj_scale(fov);
    auto mecha_tex = dsp_art::tex_mecha();
    auto fx = dsp_art::tex_fx();
    if (!mecha_tex) return;

    Vec3 mpos = mecha_.position();
    auto pr = project(mpos, vp, aspect);
    if (!pr.ok) return;
    f32 sz = std::clamp(7.0f * pscale / pr.depth, 8.0f, 180.0f);

    // 阴影
    Vec3 ground = mpos;
    {
        f32 gl = sqrtf(mpos.x * mpos.x + mpos.y * mpos.y + mpos.z * mpos.z);
        if (gl > 1.0f) ground = {mpos.x / gl * (gl - mecha_.altitude()), mpos.y / gl * (gl - mecha_.altitude()), mpos.z / gl * (gl - mecha_.altitude())};
    }
    auto gpr = project(ground, vp, aspect);
    if (gpr.ok && mecha_.altitude() > 0.5f) {
        f32 ssz = sz * clamp01(1.0f - mecha_.altitude() / 90.0f) * 0.8f + sz * 0.2f;
        batch.add(glow_tex_, gpr.pos, {ssz, ssz * 0.7f}, Color{0.0f, 0.0f, 0.0f, 0.35f}, 0.0f, -41.0f);
    }

    // 朝向: 精灵 "上方向" 对齐星球表面法线 —— 脚始终踩向地面,
    // 与移动方向/相机方位角无关 (之前对齐速度或相机朝向, 脚会朝移动方向乱转)
    Vec3 mup = mpos;
    {
        f32 mpl = sqrtf(mpos.x * mpos.x + mpos.y * mpos.y + mpos.z * mpos.z);
        if (mpl > 1.0f) mup = {mpos.x / mpl, mpos.y / mpl, mpos.z / mpl};
        else mup = {0.0f, 1.0f, 0.0f};
    }
    f32 ang = sprite_rot_;   // 退化角度 (相机正对头顶) 时沿用上次朝向, 避免抖动
    {
        auto p1 = project(mpos, vp, aspect);
        Vec3 head = {mpos.x + mup.x * 3.0f, mpos.y + mup.y * 3.0f, mpos.z + mup.z * 3.0f};
        auto p2 = project(head, vp, aspect);
        if (p1.ok && p2.ok) {
            f32 dx = p2.pos.x - p1.pos.x, dy = p2.pos.y - p1.pos.y;
            if (dx * dx + dy * dy > 4.0f) ang = atan2f(dy, dx) + kHalfPi;
        }
    }
    sprite_rot_ = ang;

    Color mecha_col = mecha_.is_low_energy() ? Color{1.0f, 0.5f, 0.45f, 1.0f} : Color{1, 1, 1, 1};
    batch.add(mecha_tex, pr.pos, {sz * 0.75f, sz}, mecha_col, ang, -8.0f);

    // 核心辉光
    if (glow_tex_) {
        Color core = mecha_.is_low_energy() ? Color{1.0f, 0.25f, 0.2f, 0.7f} : Color{0.25f, 0.9f, 1.0f, 0.55f + 0.2f * sinf(elapsed_time_ * 3.0f)};
        batch.add(glow_tex_, pr.pos, {sz * 0.5f, sz * 0.5f}, core, 0.0f, -7.0f, rhi::BlendMode::Additive);
    }

    // 推进器尾焰
    f32 burn = mecha_.thruster_intensity();
    if (glow_tex_ && burn > 0.02f && mecha_.mode() != MechaMode::GroundWalk) {
        f32 flame = sz * (0.9f + burn * 1.3f);
        Vec2 off = {sinf(ang) * sz * 0.42f, cosf(ang) * sz * 0.42f};
        batch.add(glow_tex_, {pr.pos.x - off.x * 0.3f, pr.pos.y - off.y * 0.3f}, {sz * 0.5f, flame},
                  Color{0.25f, 0.85f, 1.0f, 0.75f * burn}, ang + kPi, -9.0f, rhi::BlendMode::Additive);
        batch.add(glow_tex_, {pr.pos.x - off.x * 0.55f, pr.pos.y - off.y * 0.55f}, {sz * 0.26f, flame * 0.55f},
                  Color{0.85f, 0.95f, 1.0f, 0.8f * burn}, ang + kPi, -9.0f, rhi::BlendMode::Additive);
    }

    // 再入等离子
    f32 reentry = mecha_.reentry_intensity();
    if (glow_tex_ && reentry > 0.02f) {
        batch.add(glow_tex_, pr.pos, {sz * 2.6f, sz * 2.6f}, Color{1.0f, 0.4f, 0.1f, 0.7f * reentry}, elapsed_time_ * 7.0f, -6.0f, rhi::BlendMode::Additive);
        batch.add(glow_tex_, {pr.pos.x + sinf(ang) * sz * 0.7f, pr.pos.y + cosf(ang) * sz * 0.7f}, {sz * 0.8f, sz * 1.6f},
                  Color{1.0f, 0.6f, 0.15f, 0.65f * reentry}, ang, -6.0f, rhi::BlendMode::Additive);
    }

    // 无人机
    for (const auto& d : mecha_.drones()) {
        if (!d.active) continue;
        auto dpr = project(d.pos, vp, aspect);
        if (!dpr.ok) continue;
        f32 dsz = std::clamp(2.2f * pscale / dpr.depth, 2.0f, 18.0f);
        if (fx) batch.add_uv(fx, {0.5f, 0.0f}, {0.75f, 0.5f}, dpr.pos, {dsz, dsz}, Color{1, 1, 1, 1}, elapsed_time_ * 3.0f, -5.0f);
        if (!d.returning) {
            line_3d(batch, *this, solid_tex_, d.pos, d.target_pos, vp, aspect, 1.4f,
                    Color{0.25f, 1.0f, 0.55f, 0.7f}, -5.0f, rhi::BlendMode::Additive);
        }
    }
}

// ---------------------------------------------------------------------------
// 9) 发射特效
// ---------------------------------------------------------------------------
void Game::render_launch_fx(engine::SpriteBatch& batch, const Mat4& vp, f32 aspect) {
    if (launch_fx_.empty()) return;
    f32 fov = universe_.camera().fov_deg();
    f32 pscale = proj_scale(fov);
    auto fx = dsp_art::tex_fx();
    if (!fx || !glow_tex_) return;

    for (const auto& lf : launch_fx_) {
        f32 t = clamp01(lf.t);
        // 贝塞尔: 起点 → 抬升中点 → 恒星
        Vec3 mid = {(lf.from.x + lf.to.x) * 0.5f, (lf.from.y + lf.to.y) * 0.5f + 500.0f, (lf.from.z + lf.to.z) * 0.5f};
        f32 it = 1.0f - t;
        Vec3 p = {it * it * lf.from.x + 2 * it * t * mid.x + t * t * lf.to.x,
                  it * it * lf.from.y + 2 * it * t * mid.y + t * t * lf.to.y,
                  it * it * lf.from.z + 2 * it * t * mid.z + t * t * lf.to.z};
        Vec3 d = {2 * it * (mid.x - lf.from.x) + 2 * t * (lf.to.x - mid.x),
                  2 * it * (mid.y - lf.from.y) + 2 * t * (lf.to.y - mid.y),
                  2 * it * (mid.z - lf.from.z) + 2 * t * (lf.to.z - mid.z)};
        auto pr = project(p, vp, aspect);
        if (!pr.ok) continue;
        f32 px = std::clamp((lf.rocket ? 8.0f : 5.0f) * pscale / pr.depth, 3.0f, 40.0f);
        f32 ang = atan2f(d.y, d.x) + kHalfPi;

        // 尾焰拖尾
        batch.add(glow_tex_, pr.pos, {px * 1.6f, px * 3.2f}, lf.rocket ? Color{1.0f, 0.6f, 0.2f, 0.75f} : Color{0.3f, 0.9f, 1.0f, 0.65f},
                  ang, -2.0f, rhi::BlendMode::Additive);
        Vec2 uv0 = lf.rocket ? Vec2{0.25f, 0.0f} : Vec2{0.0f, 0.0f};
        Vec2 uv1 = lf.rocket ? Vec2{0.5f, 0.5f} : Vec2{0.25f, 0.5f};
        batch.add_uv(fx, uv0, uv1, pr.pos, {px, px * 1.4f}, Color{1, 1, 1, 1}, ang, -1.0f);
    }
}

// ---------------------------------------------------------------------------
// 10) 镜头光晕
// ---------------------------------------------------------------------------
void Game::render_lens_flare(engine::SpriteBatch& batch, f32 aspect) {
    if (!sun_on_screen_ || !glow_tex_) return;
    sun_on_screen_ = false;
    f32 half_w = 360.0f * aspect;
    Vec2 c = {0.0f, 0.0f};
    Vec2 s = sun_screen_;
    // 太阳在屏幕外太远则跳过
    if (fabsf(s.x) > half_w * 1.4f || fabsf(s.y) > 420.0f) return;

    auto flare = dsp_art::tex_flare();
    if (flare) {
        batch.add(flare, s, {520.0f, 42.0f}, Color{1.0f, 0.9f, 0.75f, 0.30f}, 0.0f, 18.0f, rhi::BlendMode::Additive);
    }
    struct Ghost { f32 t; f32 size; Color c; };
    Ghost ghosts[] = {
        {0.35f, 34.0f, Color{0.9f, 0.5f, 0.3f, 0.10f}},
        {0.65f, 22.0f, Color{0.3f, 0.8f, 1.0f, 0.09f}},
        {1.05f, 52.0f, Color{0.6f, 0.4f, 1.0f, 0.08f}},
        {1.45f, 16.0f, Color{1.0f, 0.8f, 0.4f, 0.12f}},
    };
    for (const auto& g : ghosts) {
        Vec2 p = {c.x + (c.x - s.x) * g.t, c.y + (c.y - s.y) * g.t};
        batch.add(glow_tex_, p, {g.size, g.size}, g.c, 0.0f, 18.0f, rhi::BlendMode::Additive);
    }
}

} // namespace dsp
