#pragma once

#include "core/platform.h"
#include "engine/renderer2d.h"
#include "engine/ui.h"
#include "demo/dyson_sphere/mecha.h"
#include "demo/dyson_sphere/factory.h"
#include "demo/dyson_sphere/power.h"
#include "demo/dyson_sphere/dyson_sphere.h"
#include "demo/dyson_sphere/tech_tree.h"
#include "demo/dyson_sphere/blueprint.h"
#include "demo/dyson_sphere/universe.h"
#include <vector>

namespace dsp {

using namespace aether;
using aether::engine::ui::Rect;

enum class ActiveModal {
    None = 0,
    TechTree,
    DysonEditor,
    GalaxyMap,
    Help,
};

// 建造槽位定义
struct HotbarSlot {
    BuildingKind kind = BuildingKind::None; // None 表示蓝图槽
    i32 blueprint_index = -1;
    const char* name = "";
};

class DSPUI {
public:
    DSPUI();

    void update_and_render(engine::Renderer2D* renderer,
                           Mecha& mecha,
                           Planet* current_planet,
                           FactorySystem& factory,
                           PowerGrid& power,
                           DysonSphereManager& dyson,
                           TechTreeManager& tech_tree,
                           BlueprintManager& blueprints,
                           Universe& universe,
                           const Vec3& sun_dir,
                           f32 elapsed);

    BuildingKind selected_build_tool() const { return selected_tool_; }
    void clear_build_tool() { selected_tool_ = BuildingKind::None; }
    i32 selected_blueprint_index() const { return selected_blueprint_; }
    void clear_blueprint() { selected_blueprint_ = -1; }

    ActiveModal active_modal() const { return current_modal_; }
    void toggle_modal(ActiveModal m) { current_modal_ = (current_modal_ == m) ? ActiveModal::None : m; }

    // 本帧 UI 是否拦截了屏幕坐标 (用于阻止穿透建造)
    bool is_screen_blocked(const Vec2& pos) const;

private:
    void render_top_mecha_bar(Mecha& mecha, const Planet* planet, const Star* star, f32 elapsed);
    void render_resource_bar(const FactorySystem& factory);
    void render_side_panels(const FactorySystem& factory, const PowerGrid& power, const DysonSphereManager& dyson);
    void render_hotbar(const BlueprintManager& blueprints, Mecha& mecha);
    void render_tech_tree_modal(TechTreeManager& tech_tree);
    void render_dyson_editor_modal(const DysonSphereManager& dyson);
    void render_galaxy_map_modal(Universe& universe);
    void render_help_modal();

    void block(const Rect& r);
    void draw_item_icon(ItemKind kind, f32 x, f32 y, f32 size, i32 layer);
    void draw_building_icon(BuildingKind kind, f32 x, f32 y, f32 size, i32 layer);

    BuildingKind selected_tool_ = BuildingKind::None;
    i32 selected_blueprint_ = -1;
    ActiveModal current_modal_ = ActiveModal::None;
    i32 hotbar_category_ = 0;

    bool show_power_panel_ = true;
    bool show_dyson_panel_ = true;
    bool show_throughput_panel_ = false;

    std::vector<Rect> blocked_;
    engine::Renderer2D* renderer_ = nullptr;
};

} // namespace dsp
