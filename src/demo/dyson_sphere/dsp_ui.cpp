#include "demo/dyson_sphere/dsp_ui.h"
#include "engine/ui.h"
#include "engine/input.h"
#include "demo/dyson_sphere/dsp_art.h"
#include "demo/dyson_sphere/dsp_audio.h"
#include <cstdio>
#include <algorithm>
#include <cmath>
namespace dsp {

using aether::engine::ui::Rect;
namespace ui = aether::engine::ui;

static constexpr f32 kPi = 3.14159265358979323846f;
static constexpr f32 kTwoPi = kPi * 2.0f;

// 分类与槽位表
struct CategoryDef {
    const char* label;
    std::vector<HotbarSlot> slots;
};

static std::vector<CategoryDef> build_categories() {
    std::vector<CategoryDef> cats;
    cats.push_back({"物流", {{BuildingKind::ConveyorBelt, -1, "传送带"}, {BuildingKind::Sorter, -1, "分拣器"}}});
    cats.push_back({"生产", {{BuildingKind::MiningMachine, -1, "采矿机"}, {BuildingKind::OilExtractor, -1, "抽油机"},
                             {BuildingKind::ArcSmelter, -1, "电弧熔炉"}, {BuildingKind::AssemblingMachine, -1, "组装机"},
                             {BuildingKind::ChemicalPlant, -1, "化工厂"}, {BuildingKind::MatrixLab, -1, "矩阵实验室"}}});
    cats.push_back({"电力", {{BuildingKind::WindTurbine, -1, "风力涡轮机"}, {BuildingKind::SolarPanel, -1, "太阳能板"},
                             {BuildingKind::TeslaTower, -1, "电浆中继塔"}, {BuildingKind::WirelessPowerTower, -1, "无线输电塔"},
                             {BuildingKind::ThermalPowerPlant, -1, "火力发电厂"}, {BuildingKind::FusionPowerPlant, -1, "微型聚变电站"},
                             {BuildingKind::ArtificialStar, -1, "人造恒星"}}});
    cats.push_back({"物流站", {{BuildingKind::PlanetaryLogisticsStation, -1, "行星物流站"},
                               {BuildingKind::InterstellarLogisticsStation, -1, "星际物流站"}}});
    cats.push_back({"巨构", {{BuildingKind::EMRailEjector, -1, "电磁弹射器"}, {BuildingKind::VerticalLaunchSilo, -1, "垂直发射井"},
                             {BuildingKind::RayReceiver, -1, "射线接收站"}}});
    cats.push_back({"蓝图", {}});
    return cats;
}

static const std::vector<CategoryDef>& categories() {
    static const std::vector<CategoryDef> cats = build_categories();
    return cats;
}

static const char* building_desc(BuildingKind k) {
    switch (k) {
        case BuildingKind::ConveyorBelt: return "在网格上运输货物, 沿箭头方向流动";
        case BuildingKind::Sorter: return "从后方抓取货物放到前方建筑";
        case BuildingKind::MiningMachine: return "开采所在瓦片的矿脉, 需建在矿上";
        case BuildingKind::OilExtractor: return "开采原油矿脉";
        case BuildingKind::ArcSmelter: return "熔炼矿石为板材";
        case BuildingKind::AssemblingMachine: return "组装中间产品";
        case BuildingKind::ChemicalPlant: return "化工精炼产物";
        case BuildingKind::MatrixLab: return "消耗材料产出科研矩阵";
        case BuildingKind::PlanetaryLogisticsStation: return "全球物流枢纽, 无人机运输";
        case BuildingKind::InterstellarLogisticsStation: return "星际物流枢纽, 星舰运输";
        case BuildingKind::EMRailEjector: return "向恒星弹射太阳帆 (戴森云)";
        case BuildingKind::VerticalLaunchSilo: return "发射运载火箭搭建戴森球壳";
        case BuildingKind::RayReceiver: return "接收戴森球能量射线";
        case BuildingKind::ArtificialStar: return "反物质反应堆, 输出海量电力";
        case BuildingKind::TeslaTower: return "接通电网, 小范围供电";
        case BuildingKind::WirelessPowerTower: return "大范围供电并为机甲无线充能";
        case BuildingKind::WindTurbine: return "清洁电力, 输出随风速波动";
        case BuildingKind::SolarPanel: return "白天发电, 受日照角度影响";
        case BuildingKind::ThermalPowerPlant: return "燃烧煤炭发电";
        case BuildingKind::FusionPowerPlant: return "聚变发电, 高输出";
        default: return "";
    }
}

DSPUI::DSPUI() = default;

bool DSPUI::is_screen_blocked(const Vec2& pos) const {
    for (const auto& r : blocked_) {
        if (r.contains(pos)) return true;
    }
    return false;
}

void DSPUI::block(const Rect& r) {
    blocked_.push_back(r);
}

void DSPUI::draw_item_icon(ItemKind kind, f32 x, f32 y, f32 size, i32 layer) {
    auto tex = dsp_art::tex_items();
    if (!tex || !renderer_) return;
    Vec2 uv0, uv1;
    dsp_art::cell_uv(8, 8, (int)kind, uv0, uv1);
    f32 half_h = 360.0f;
    f32 half_w = half_h * renderer_->aspect();
    Vec2 cam = renderer_->camera().position;
    renderer_->sprites().add_uv(tex, uv0, uv1,
                                {x - half_w + cam.x + size * 0.5f, y - half_h + cam.y + size * 0.5f},
                                {size, size}, Color{1, 1, 1, 1}, 0.0f, (f32)layer);
}

void DSPUI::draw_building_icon(BuildingKind kind, f32 x, f32 y, f32 size, i32 layer) {
    auto tex = dsp_art::tex_buildings();
    if (!tex || !renderer_) return;
    Vec2 uv0, uv1;
    dsp_art::cell_uv(6, 6, (int)kind - 1, uv0, uv1);
    f32 half_h = 360.0f;
    f32 half_w = half_h * renderer_->aspect();
    Vec2 cam = renderer_->camera().position;
    renderer_->sprites().add_uv(tex, uv0, uv1,
                                {x - half_w + cam.x + size * 0.5f, y - half_h + cam.y + size * 0.5f},
                                {size, size}, Color{1, 1, 1, 1}, 0.0f, (f32)layer);
}

void DSPUI::update_and_render(engine::Renderer2D* renderer,
                              Mecha& mecha,
                              Planet* current_planet,
                              FactorySystem& factory,
                              PowerGrid& power,
                              DysonSphereManager& dyson,
                              TechTreeManager& tech_tree,
                              BlueprintManager& blueprints,
                              Universe& universe,
                              const Vec3& sun_dir,
                              f32 elapsed) {
    if (!renderer) return;
    renderer_ = renderer;
    blocked_.clear();

    ui::begin_frame(renderer);

    // 全局快捷键
    if (engine::Input::was_key_pressed(engine::KeyCode::T)) toggle_modal(ActiveModal::TechTree);
    if (engine::Input::was_key_pressed(engine::KeyCode::Y)) toggle_modal(ActiveModal::DysonEditor);
    if (engine::Input::was_key_pressed(engine::KeyCode::M)) toggle_modal(ActiveModal::GalaxyMap);
    if (engine::Input::was_key_pressed(engine::KeyCode::H)) toggle_modal(ActiveModal::Help);
    if (engine::Input::was_key_pressed(engine::KeyCode::P)) show_power_panel_ = !show_power_panel_;
    if (engine::Input::was_key_pressed(engine::KeyCode::I)) show_throughput_panel_ = !show_throughput_panel_;

    render_top_mecha_bar(mecha, current_planet, universe.current_star(), elapsed);
    render_resource_bar(factory);
    render_side_panels(factory, power, dyson);
    render_hotbar(blueprints, mecha);

    if (current_modal_ == ActiveModal::TechTree) render_tech_tree_modal(tech_tree);
    else if (current_modal_ == ActiveModal::DysonEditor) render_dyson_editor_modal(dyson);
    else if (current_modal_ == ActiveModal::GalaxyMap) render_galaxy_map_modal(universe);
    else if (current_modal_ == ActiveModal::Help) render_help_modal();

    if (current_modal_ != ActiveModal::None) {
        block({0, 0, 1280, 720});
    }

    ui::draw(renderer);
    ui::end_frame();
    renderer_ = nullptr;
}

// ---------------------------------------------------------------------------
// 顶部机甲状态栏
// ---------------------------------------------------------------------------
void DSPUI::render_top_mecha_bar(Mecha& mecha, const Planet* planet, const Star* star, f32 elapsed) {
    auto& th = ui::get_theme();
    f32 x = 12, y = 10, w = 452, h = 64;
    ui::draw_panel({x, y, w, h}, nullptr, nullptr, nullptr, 25);
    block({x, y, w, h});

    char core_buf[32];
    snprintf(core_buf, sizeof(core_buf), "核心 MK.%u", mecha.core_level());
    ui::badge(core_buf, x + 8, y + 14, th.bg_dark, th.accent_cyan, 26);

    char energy_lbl[64];
    snprintf(energy_lbl, sizeof(energy_lbl), "%.0f / %.0f MJ (%.0f%%)",
             mecha.energy_mj(), mecha.max_energy_mj(), mecha.energy_ratio() * 100.0f);
    Color fill = mecha.is_low_energy() ? th.accent_red : th.accent_cyan;
    ui::progress_bar({x + 96, y + 8, 210, 15}, mecha.energy_ratio(), fill, nullptr, energy_lbl, 26);

    char flow_buf[64];
    snprintf(flow_buf, sizeof(flow_buf), "耗电 %.1f MW · 充能 %.1f MW", mecha.power_draw_mw(), mecha.charge_rate_mw());
    ui::draw_text(flow_buf, x + 96, y + 32, 11, 0, th.text_secondary, 26);

    const auto& fuel = mecha.fuel_slot();
    char fuel_buf[48];
    if (fuel.count > 0) {
        const char* f_name = (fuel.kind == FuelKind::Coal) ? "煤炭" :
                             (fuel.kind == FuelKind::HydrogenFuelRod) ? "氢燃料棒" :
                             (fuel.kind == FuelKind::DeuteronFuelRod) ? "氘核燃料棒" : "反物质胶囊";
        snprintf(fuel_buf, sizeof(fuel_buf), "%s ×%u", f_name, fuel.count);
    } else {
        snprintf(fuel_buf, sizeof(fuel_buf), "燃料舱: 空");
    }
    ui::badge(fuel_buf, x + 316, y + 12, th.bg_card, fuel.count > 0 ? th.accent_gold : th.accent_red, 26);

    const char* mode_str = (mecha.mode() == MechaMode::GroundWalk) ? "地表行走" :
                           (mecha.mode() == MechaMode::LowFlight) ? "喷气飞行" :
                           (mecha.mode() == MechaMode::SpaceSailing) ? "太空航行" : "曲率飞行";
    Color mode_col = (mecha.mode() == MechaMode::GroundWalk) ? th.accent_green :
                     (mecha.mode() == MechaMode::LowFlight) ? th.accent_blue :
                     (mecha.mode() == MechaMode::SpaceSailing) ? th.accent_cyan : th.accent_purple;
    ui::badge(mode_str, x + 316, y + 36, th.bg_dark, mode_col, 26);

    // 中央: 天体信息 + 昼夜时钟
    f32 lx = (1280.0f - 400.0f) * 0.5f;
    ui::draw_panel({lx, y, 400, h}, nullptr, nullptr, nullptr, 25);
    block({lx, y, 400, h});
    const char* s_name = star ? star->name.c_str() : "深空";
    const char* p_name = planet ? planet->name().c_str() : "轨道空间";
    ui::draw_text(s_name, lx + 14, y + 12, 12, 0.5f, th.text_accent, 26);
    ui::draw_text(p_name, lx + 14, y + 30, 12, 0.5f, th.text_primary, 26);

    // 昼夜: 太阳方向与行星的方位角 → 当地时间
    f32 sun_a = elapsed * 0.013f;
    f32 day_frac = fmodf(sun_a / kTwoPi + 0.5f, 1.0f);
    i32 hh = (i32)(day_frac * 24.0f);
    i32 mm = (i32)fmodf(day_frac * 24.0f * 60.0f, 60.0f);
    bool is_day = day_frac > 0.25f && day_frac < 0.75f;
    char time_buf[40];
    snprintf(time_buf, sizeof(time_buf), "%s %02d:%02d", is_day ? "[昼]" : "[夜]", hh, mm);
    ui::draw_text(time_buf, lx + 14, y + 46, 12, 1.0f, is_day ? th.accent_gold : th.text_muted, 26);

    // 右上: 界面按钮
    f32 btn_x = 866, bw = 66, bh = 30;
    const char* labels[] = {"[T] 科技", "[Y] 戴森球", "[M] 星图", "[H] 帮助", "[P] 电网", "[I] 产线"};
    for (int i = 0; i < 6; i++) {
        Rect br = {btn_x + (f32)(i % 3) * (bw + 6), y + (f32)(i / 3) * (bh + 4), bw, bh};
        bool active = (i == 0 && current_modal_ == ActiveModal::TechTree) ||
                      (i == 1 && current_modal_ == ActiveModal::DysonEditor) ||
                      (i == 2 && current_modal_ == ActiveModal::GalaxyMap) ||
                      (i == 3 && current_modal_ == ActiveModal::Help) ||
                      (i == 4 && show_power_panel_) || (i == 5 && show_throughput_panel_);
        if (ui::button(labels[i], br, active, 26)) {
            DSPAudioManager::play_click();
            switch (i) {
                case 0: toggle_modal(ActiveModal::TechTree); break;
                case 1: toggle_modal(ActiveModal::DysonEditor); break;
                case 2: toggle_modal(ActiveModal::GalaxyMap); break;
                case 3: toggle_modal(ActiveModal::Help); break;
                case 4: show_power_panel_ = !show_power_panel_; break;
                case 5: show_throughput_panel_ = !show_throughput_panel_; break;
            }
        }
    }
    block({btn_x - 4, y, 3 * (bw + 6) + 8, 2 * (bh + 4) + 4});
}

// ---------------------------------------------------------------------------
// 资源栏
// ---------------------------------------------------------------------------
void DSPUI::render_resource_bar(const FactorySystem& factory) {
    auto& th = ui::get_theme();
    static const ItemKind tracked[] = {
        ItemKind::IronIngot, ItemKind::CopperIngot, ItemKind::Magnet, ItemKind::MagneticCoil,
        ItemKind::CircuitBoard, ItemKind::Processor, ItemKind::TitaniumIngot,
        ItemKind::HighPuritySilicon, ItemKind::MatrixBlue, ItemKind::SolarSail, ItemKind::SmallCarrierRocket
    };
    static const char* names[] = {"铁块", "铜块", "磁铁", "线圈", "电路板", "处理器", "钛块", "高纯硅", "蓝矩阵", "太阳帆", "火箭"};

    f32 slot_w = 74.0f;
    f32 x0 = 12.0f, y0 = 82.0f;
    f32 w = slot_w * 11.0f + 8.0f;
    ui::draw_rect({x0, y0, w, 44}, Color{0.03f, 0.05f, 0.09f, 0.72f}, 24);
    ui::draw_rect_outline({x0, y0, w, 44}, 1.0f, Color{0.2f, 0.35f, 0.55f, 0.5f}, 24);
    block({x0, y0, w, 44});

    for (int i = 0; i < 11; i++) {
        f32 sx = x0 + 4.0f + (f32)i * slot_w;
        u32 count = factory.total_stored(tracked[i]);
        draw_item_icon(tracked[i], sx + 2, y0 + 8, 26, 27);
        char cnt[16];
        if (count >= 10000) snprintf(cnt, sizeof(cnt), "%.1fk", (f32)count / 1000.0f);
        else snprintf(cnt, sizeof(cnt), "%u", count);
        ui::draw_text(cnt, sx + 30, y0 + 10, 12, 0, th.text_primary, 27);
        ui::draw_text(names[i], sx + 30, y0 + 25, 10, 0, th.text_muted, 27);
    }
}

// ---------------------------------------------------------------------------
// 右侧信息面板
// ---------------------------------------------------------------------------
void DSPUI::render_side_panels(const FactorySystem& factory, const PowerGrid& power, const DysonSphereManager& dyson) {
    auto& th = ui::get_theme();
    f32 px = 1014.0f, pw = 254.0f, py = 82.0f;

    if (show_power_panel_) {
        f32 h = 140.0f;
        ui::draw_panel({px, py, pw, h}, "行星电网", nullptr, nullptr, 25);
        block({px, py, pw, h});

        char gen_buf[64], dem_buf[64], sat_buf[32];
        snprintf(gen_buf, sizeof(gen_buf), "发电 %.1f MW", power.total_generation_kw() * 0.001f);
        snprintf(dem_buf, sizeof(dem_buf), "耗电 %.1f MW", power.total_demand_kw() * 0.001f);
        snprintf(sat_buf, sizeof(sat_buf), "满足率 %.0f%%", power.satisfaction_ratio() * 100.0f);
        ui::draw_text(gen_buf, px + 12, py + 38, 12, 0, th.accent_green, 26);
        ui::draw_text(dem_buf, px + 130, py + 38, 12, 0, th.accent_orange, 26);
        Color sat_col = power.satisfaction_ratio() >= 1.0f ? th.accent_green : th.accent_red;
        ui::progress_bar({px + 12, py + 56, pw - 24, 16}, power.satisfaction_ratio(), sat_col, nullptr, sat_buf, 26);
        char accum_buf[64];
        snprintf(accum_buf, sizeof(accum_buf), "蓄电池 %.0f / %.0f MJ", power.accumulator_charge_mj(), power.accumulator_max_mj());
        ui::progress_bar({px + 12, py + 80, pw - 24, 14},
                         power.accumulator_charge_mj() / power.accumulator_max_mj(), th.accent_blue, nullptr, accum_buf, 26);
        if (power.is_mecha_charging()) {
            ui::badge("机甲无线快充中", px + 12, py + 112, th.bg_dark, th.accent_cyan, 26);
        }
        py += h + 10;
    }

    if (show_dyson_panel_) {
        f32 h = 150.0f;
        ui::draw_panel({px, py, pw, h}, "戴森球工程", nullptr, nullptr, 25);
        block({px, py, pw, h});

        char sail_buf[64], node_buf[64], out_buf[64];
        snprintf(sail_buf, sizeof(sail_buf), "在轨太阳帆 %u 枚", dyson.sail_count());
        snprintf(node_buf, sizeof(node_buf), "骨架节点 %u / %u", dyson.completed_nodes(), dyson.node_count());
        snprintf(out_buf, sizeof(out_buf), "总发电 %.3f GW", dyson.total_generation_gw());
        ui::draw_text(sail_buf, px + 12, py + 38, 12, 0.5f, th.accent_gold, 26);
        ui::draw_text(node_buf, px + 12, py + 58, 12, 0.5f, th.accent_cyan, 26);
        ui::draw_text(out_buf, px + 12, py + 82, 15, 1.0f, th.accent_gold, 26, false, 0.4f);
        f32 node_ratio = dyson.node_count() > 0 ? (f32)dyson.completed_nodes() / (f32)dyson.node_count() : 0.0f;
        ui::progress_bar({px + 12, py + 106, pw - 24, 12}, node_ratio, th.accent_cyan, nullptr, nullptr, 26);
        char launch_buf[64];
        snprintf(launch_buf, sizeof(launch_buf), "累计发射: 帆 %u | 火箭 %u", factory.launched_solar_sails(), factory.launched_carrier_rockets());
        ui::draw_text(launch_buf, px + 12, py + 126, 11, 0, th.text_muted, 26);
        py += h + 10;
    }

    if (show_throughput_panel_) {
        f32 h = 168.0f;
        ui::draw_panel({px, py, pw, h}, "产线统计 (/分)", nullptr, nullptr, 25);
        block({px, py, pw, h});
        ItemKind items[] = {ItemKind::IronIngot, ItemKind::CopperIngot, ItemKind::CircuitBoard,
                            ItemKind::MagneticCoil, ItemKind::Processor, ItemKind::MatrixBlue};
        f32 sy = py + 38;
        for (int i = 0; i < 6; i++) {
            f32 prod = factory.get_production_rate(items[i]);
            f32 cons = factory.get_consumption_rate(items[i]);
            draw_item_icon(items[i], px + 12, sy - 2, 18, 27);
            char line[48];
            snprintf(line, sizeof(line), "+%.0f / -%.0f", prod, cons);
            ui::draw_text(line, px + 40, sy, 12, 0, prod >= cons ? th.text_primary : th.accent_orange, 26);
            sy += 21;
        }
    }
}

// ---------------------------------------------------------------------------
// 底部建造栏
// ---------------------------------------------------------------------------
void DSPUI::render_hotbar(const BlueprintManager& blueprints, Mecha& mecha) {
    auto& th = ui::get_theme();

    f32 dock_w = 1000.0f, dock_h = 118.0f;
    f32 dock_x = (1280.0f - dock_w) * 0.5f;
    f32 dock_y = 720.0f - dock_h - 8.0f;
    ui::draw_panel({dock_x, dock_y, dock_w, dock_h}, nullptr, nullptr, nullptr, 25);
    block({dock_x, dock_y, dock_w, dock_h});

    // 分类页签
    const auto& cats = categories();
    f32 tab_w = 86.0f, tab_h = 22.0f;
    for (i32 i = 0; i < (i32)cats.size(); i++) {
        Rect tr = {dock_x + 8.0f + (f32)i * (tab_w + 4.0f), dock_y + 6.0f, tab_w, tab_h};
        if (ui::button(cats[i].label, tr, hotbar_category_ == i, 26)) {
            DSPAudioManager::play_click();
            hotbar_category_ = i;
        }
    }

    // 当前分类槽位
    std::vector<HotbarSlot> slots;
    if (hotbar_category_ == 5) {
        i32 n = std::min((i32)blueprints.presets().size(), 5);
        for (i32 i = 0; i < n; i++) slots.push_back({BuildingKind::None, i, blueprints.presets()[i].name.c_str()});
    } else {
        slots = cats[hotbar_category_].slots;
    }

    f32 slot_w = 88.0f, slot_h = 58.0f;
    f32 sx0 = dock_x + 10.0f, sy0 = dock_y + 36.0f;
    char sel_title[96] = "";

    for (i32 i = 0; i < (i32)slots.size() && i < 10; i++) {
        const auto& s = slots[i];
        Rect r = {sx0 + (f32)i * (slot_w + 6.0f), sy0, slot_w, slot_h};
        bool selected = (s.kind != BuildingKind::None) ? (selected_tool_ == s.kind) : (selected_blueprint_ == s.blueprint_index);

        ui::draw_rect(r, selected ? th.bg_active : th.bg_card, 26);
        ui::draw_rect_outline(r, 1.5f, selected ? th.border_active : th.border, 26);

        if (s.kind != BuildingKind::None) {
            draw_building_icon(s.kind, r.x + (slot_w - 36) * 0.5f, r.y + 4, 36, 27);
        } else {
            ui::draw_text("[图]", r.x + slot_w * 0.5f, r.y + 12, 16, 0, th.accent_gold, 27, true);
        }

        // 热键角标
        char hk[4];
        snprintf(hk, sizeof(hk), "%d", (i + 1) % 10);
        ui::draw_text(hk, r.x + 4, r.y + 3, 10, 0, th.text_muted, 27);

        ui::draw_text(s.name, r.x + slot_w * 0.5f, r.y + slot_h - 12, 11, 0, th.text_primary, 27, true);

        Vec2 mp = engine::Input::mouse_pos();
        if (r.contains(mp)) {
            if (s.kind != BuildingKind::None) {
                char tip[128];
                f32 kw = building_power_demand_kw(s.kind);
                if (kw > 0) snprintf(tip, sizeof(tip), "%s | 耗电 %.0f kW — %s", s.name, kw, building_desc(s.kind));
                else snprintf(tip, sizeof(tip), "%s — %s", s.name, building_desc(s.kind));
                ui::tooltip(tip, {r.x + slot_w * 0.5f, r.y - 8}, 35);
            } else {
                ui::tooltip(blueprints.presets()[s.blueprint_index].desc.c_str(), {r.x + slot_w * 0.5f, r.y - 8}, 35);
            }
        }

        if (ui::button("", r, selected, 28)) {
            DSPAudioManager::play_click();
            if (s.kind != BuildingKind::None) {
                selected_tool_ = selected ? BuildingKind::None : s.kind;
                selected_blueprint_ = -1;
            } else {
                selected_blueprint_ = selected ? -1 : s.blueprint_index;
                selected_tool_ = BuildingKind::None;
            }
        }

        if (selected) snprintf(sel_title, sizeof(sel_title), "%s", s.name);
    }

    // 数字键快捷选择
    static const aether::engine::KeyCode num_keys[10] = {
        aether::engine::KeyCode::Num1, aether::engine::KeyCode::Num2, aether::engine::KeyCode::Num3,
        aether::engine::KeyCode::Num4, aether::engine::KeyCode::Num5, aether::engine::KeyCode::Num6,
        aether::engine::KeyCode::Num7, aether::engine::KeyCode::Num8, aether::engine::KeyCode::Num9,
        aether::engine::KeyCode::Num0
    };
    for (i32 i = 0; i < 10; i++) {
        if (engine::Input::was_key_pressed(num_keys[i])) {
            if (i < (i32)slots.size()) {
                const auto& s = slots[i];
                if (s.kind != BuildingKind::None) {
                    bool was = (selected_tool_ == s.kind);
                    selected_tool_ = was ? BuildingKind::None : s.kind;
                    selected_blueprint_ = -1;
                    if (!was) snprintf(sel_title, sizeof(sel_title), "%s", s.name);
                } else {
                    bool was = (selected_blueprint_ == s.blueprint_index);
                    selected_blueprint_ = was ? -1 : s.blueprint_index;
                    selected_tool_ = BuildingKind::None;
                }
                DSPAudioManager::play_click();
            }
        }
    }

    // 选中提示与操作说明
    if (sel_title[0]) {
        char hint[96];
        snprintf(hint, sizeof(hint), "建造: %s — 左键放置, R 旋转, 拖拽铺带, Esc 取消", sel_title);
        ui::draw_text(hint, dock_x + dock_w * 0.5f, dock_y - 20, 13, 0.5f, th.accent_cyan, 27, true, 0.3f);
    }

    // 起飞按钮
    Rect fly_r = {dock_x + dock_w - 118.0f, sy0 + 8.0f, 100.0f, 42.0f};
    bool flying = (mecha.mode() != MechaMode::GroundWalk);
    if (ui::button(flying ? "降落 [空格]" : "起飞 [空格]", fly_r, flying, 26)) {
        mecha.toggle_flight();
        DSPAudioManager::play_click();
    }
}

// ---------------------------------------------------------------------------
// 科技树
// ---------------------------------------------------------------------------
void DSPUI::render_tech_tree_modal(TechTreeManager& tech_tree) {
    auto& th = ui::get_theme();
    f32 mw = 900.0f, mh = 580.0f;
    f32 mx = (1280.0f - mw) * 0.5f, my = (720.0f - mh) * 0.5f;

    ui::draw_rect({0, 0, 1280, 720}, Color{0, 0, 0, 0.72f}, 30);
    ui::draw_panel({mx, my, mw, mh}, "矩阵科研中心", nullptr, nullptr, 31);

    if (ui::button("关闭 [Esc]", {mx + mw - 110, my + 6, 95, 24}, false, 32)) {
        DSPAudioManager::play_click();
        current_modal_ = ActiveModal::None;
    }

    f32 y = my + 46;
    for (const auto& t : tech_tree.all_techs()) {
        Rect tr = {mx + 20, y, mw - 40, 36};
        bool is_current = (tech_tree.current_research() == t.id);
        Color border = t.unlocked ? th.accent_green :
                       is_current ? th.accent_cyan :
                       (tech_tree.can_research(t.id) ? th.border_bright : th.border);
        ui::draw_rect(tr, is_current ? th.bg_active : th.bg_card, 32);
        ui::draw_rect_outline(tr, 1.5f, border, 32);

        const char* status = t.unlocked ? "已解锁" : is_current ? "研发中" : (tech_tree.can_research(t.id) ? "可研发" : "未解锁");
        Color badge_col = t.unlocked ? th.accent_green : (is_current ? th.accent_cyan : th.text_muted);
        ui::badge(status, mx + 30, y + 18, th.bg_dark, badge_col, 33);
        ui::draw_text(t.name.c_str(), mx + 105, y + 18, 13, 0.5f, th.text_primary, 33);
        ui::draw_text(t.desc.c_str(), mx + 330, y + 18, 11, 0, th.text_muted, 33);

        if (!t.unlocked && tech_tree.can_research(t.id) && !is_current) {
            if (ui::button("开始研究", {mx + mw - 125, y + 5, 100, 26}, false, 33)) {
                tech_tree.select_research(t.id);
                DSPAudioManager::play_click();
            }
        } else if (is_current && !t.unlocked) {
            char prog[16];
            snprintf(prog, sizeof(prog), "%.0f%%", tech_tree.current_progress_ratio() * 100.0f);
            ui::progress_bar({mx + mw - 125, y + 7, 100, 20}, tech_tree.current_progress_ratio(), th.accent_cyan, nullptr, prog, 33);
        }
        y += 40;
    }
}

// ---------------------------------------------------------------------------
// 戴森球编辑器
// ---------------------------------------------------------------------------
void DSPUI::render_dyson_editor_modal(const DysonSphereManager& dyson) {
    auto& th = ui::get_theme();
    f32 mw = 820.0f, mh = 480.0f;
    f32 mx = (1280.0f - mw) * 0.5f, my = (720.0f - mh) * 0.5f;

    ui::draw_rect({0, 0, 1280, 720}, Color{0, 0, 0, 0.72f}, 30);
    ui::draw_panel({mx, my, mw, mh}, "戴森球轨道规划", nullptr, nullptr, 31);
    if (ui::button("关闭 [Esc]", {mx + mw - 110, my + 6, 95, 24}, false, 32)) {
        DSPAudioManager::play_click();
        current_modal_ = ActiveModal::None;
    }

    f32 lw = 380.0f;
    ui::draw_panel({mx + 20, my + 45, lw, mh - 65}, "戴森云 (太阳帆)", nullptr, nullptr, 32);
    char buf[80];
    snprintf(buf, sizeof(buf), "在轨太阳帆: %u 枚", dyson.sail_count());
    ui::draw_text(buf, mx + 35, my + 85, 14, 0.5f, th.accent_gold, 33);
    snprintf(buf, sizeof(buf), "戴森云发电: %.3f GW", dyson.swarm_generation_gw());
    ui::draw_text(buf, mx + 35, my + 115, 13, 0.5f, th.text_primary, 33);
    ui::draw_text("由电磁弹射器发射太阳帆组成\n环绕恒星的能源收集阵列。", mx + 35, my + 150, 12, 0, th.text_secondary, 33);

    f32 rx = mx + lw + 40.0f, rw = mw - lw - 60.0f;
    ui::draw_panel({rx, my + 45, rw, mh - 65}, "戴森壳 (骨架节点)", nullptr, nullptr, 32);
    snprintf(buf, sizeof(buf), "结构节点: %u / %u", dyson.completed_nodes(), dyson.node_count());
    ui::draw_text(buf, rx + 15, my + 85, 13, 0.5f, th.accent_cyan, 33);
    snprintf(buf, sizeof(buf), "骨架连结: %u 条", (u32)dyson.struts().size());
    ui::draw_text(buf, rx + 15, my + 115, 13, 0.5f, th.text_primary, 33);
    snprintf(buf, sizeof(buf), "戴森壳发电: %.3f GW", dyson.shell_generation_gw());
    ui::draw_text(buf, rx + 15, my + 145, 15, 0.5f, th.accent_gold, 33, false, 0.4f);
    f32 ratio = dyson.node_count() > 0 ? (f32)dyson.completed_nodes() / (f32)dyson.node_count() : 0.0f;
    ui::progress_bar({rx + 15, my + 180, rw - 30, 16}, ratio, th.accent_cyan, nullptr, "工程进度", 33);
    ui::draw_text("由垂直发射井发射运载火箭\n逐节点搭建测地线壳体。", rx + 15, my + 215, 12, 0, th.text_secondary, 33);
}

// ---------------------------------------------------------------------------
// 星图
// ---------------------------------------------------------------------------
void DSPUI::render_galaxy_map_modal(Universe& universe) {
    auto& th = ui::get_theme();
    f32 mw = 860.0f, mh = 560.0f;
    f32 mx = (1280.0f - mw) * 0.5f, my = (720.0f - mh) * 0.5f;

    ui::draw_rect({0, 0, 1280, 720}, Color{0, 0, 0, 0.72f}, 30);
    ui::draw_panel({mx, my, mw, mh}, "银河星团星图 (曲率跃迁)", nullptr, nullptr, 31);
    if (ui::button("关闭 [Esc]", {mx + mw - 110, my + 6, 95, 24}, false, 32)) {
        DSPAudioManager::play_click();
        current_modal_ = ActiveModal::None;
    }

    f32 y = my + 48;
    for (size_t i = 0; i < universe.stars().size(); i++) {
        const auto& s = universe.stars()[i];
        Rect sr = {mx + 20, y, mw - 40, 108};
        ui::draw_panel(sr, nullptr, nullptr, nullptr, 32);

        Color sc = star_color(s->type);
        // 恒星图标
        f32 cxp = mx + 52, cyp = y + 40;
        ui::draw_rect({cxp - 14, cyp - 14, 28, 28}, Color{sc.r, sc.g, sc.b, 0.35f}, 33);
        ui::draw_rect({cxp - 9, cyp - 9, 18, 18}, sc, 33);

        char spec_buf[96], pos_buf[96];
        snprintf(spec_buf, sizeof(spec_buf), "%s | 光度 %.2fx | 行星 %u 颗",
                 spectral_name(s->type), s->luminosity, (u32)s->planets.size());
        snprintf(pos_buf, sizeof(pos_buf), "坐标 (%.1f, %.1f, %.1f) 光年", s->pos_ly.x, s->pos_ly.y, s->pos_ly.z);
        ui::draw_text(s->name.c_str(), mx + 80, y + 16, 15, 0.5f, sc, 33);
        ui::draw_text(spec_buf, mx + 80, y + 40, 12, 0, th.text_primary, 33);
        ui::draw_text(pos_buf, mx + 80, y + 60, 11, 0, th.text_muted, 33);

        // 行星列表
        f32 plx = mx + 80;
        for (size_t pi = 0; pi < s->planets.size() && pi < 6; pi++) {
            const auto& p = s->planets[pi];
            ui::draw_rect({plx, y + 80, 90, 18}, Color{p->atmosphere_color().r * 0.5f, p->atmosphere_color().g * 0.5f, p->atmosphere_color().b * 0.5f, 0.8f}, 33);
            ui::draw_text(p->name().c_str(), plx + 4, y + 84, 9, 0, th.text_primary, 34);
            plx += 94;
        }

        if (ui::button("跃迁", {mx + mw - 135, y + 16, 90, 30}, false, 33)) {
            universe.select_star(i);
            DSPAudioManager::play_warp();
            current_modal_ = ActiveModal::None;
        }
        y += 116;
    }
}

// ---------------------------------------------------------------------------
// 帮助
// ---------------------------------------------------------------------------
void DSPUI::render_help_modal() {
    auto& th = ui::get_theme();
    f32 mw = 640.0f, mh = 520.0f;
    f32 mx = (1280.0f - mw) * 0.5f, my = (720.0f - mh) * 0.5f;

    ui::draw_rect({0, 0, 1280, 720}, Color{0, 0, 0, 0.72f}, 30);
    ui::draw_panel({mx, my, mw, mh}, "操作指南", nullptr, nullptr, 31);
    if (ui::button("关闭 [Esc]", {mx + mw - 110, my + 6, 95, 24}, false, 32)) {
        DSPAudioManager::play_click();
        current_modal_ = ActiveModal::None;
    }

    static const char* lines[] = {
        "WASD           移动机甲",
        "空格 / E       上升起飞        Q / Ctrl   下降",
        "鼠标滚轮       缩放视角        右键拖拽   环绕镜头",
        "左键           放置建筑 / 铺设蓝图",
        "按住左键拖拽   连续铺设传送带 (自动转向)",
        "R              旋转建筑朝向",
        "X / 中键       拆除光标处建筑",
        "1 ~ 0          快捷选择建造栏槽位",
        "T / Y / M / H  科技 / 戴森球 / 星图 / 帮助",
        "P / I          电网面板 / 产线统计",
        "Esc            取消选择 / 关闭界面",
    };
    f32 y = my + 52;
    for (const char* l : lines) {
        ui::draw_text(l, mx + 30, y, 14, 0.5f, th.text_primary, 33);
        y += 30;
    }
    ui::draw_text("提示: 采矿机建在矿脉上, 传送带沿箭头把货物送进机器的入口。", mx + 30, my + mh - 50, 12, 0, th.accent_cyan, 33);
    ui::draw_text("目标: 发展工厂, 向恒星发射太阳帆与火箭, 建成戴森球!", mx + 30, my + mh - 30, 12, 0, th.accent_gold, 33);
}

} // namespace dsp
