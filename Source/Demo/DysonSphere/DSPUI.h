#pragma once

#include "Core.h"
#include "Renderer2D.h"
#include "UI.h"
#include "Mecha.h"
#include "Factory.h"
#include "Power.h"
#include "DysonSphere.h"
#include "TechTree.h"
#include "Blueprint.h"
#include "Universe.h"
#include "Container/Array.h"

namespace DSP {

using Aether::f32;
using Aether::i32;
using Aether::Math::Vec2;
using Aether::Math::Vec3;
using Aether::Array;
using Aether::Engine::UI::Rect;

enum class ActiveModal {
    None = 0,
    TechTree,
    DysonEditor,
    GalaxyMap,
    Help,
};

// 建造槽位定义
struct HotbarSlot {
    BuildingKind Kind = BuildingKind::None; // None 表示蓝图槽
    i32 BlueprintIndex = -1;
    const char* Name = "";
};

class DSPUI {
public:
    DSPUI();

    void UpdateAndRender(Aether::Engine::Renderer2D* renderer,
                         Mecha& mecha,
                         Planet* currentPlanet,
                         FactorySystem& factory,
                         PowerGrid& power,
                         DysonSphereManager& dyson,
                         TechTreeManager& techTree,
                         BlueprintManager& blueprints,
                         Universe& universe,
                         const Vec3& sunDir,
                         f32 elapsed);

    BuildingKind SelectedBuildTool() const { return mSelectedTool; }
    void ClearBuildTool() { mSelectedTool = BuildingKind::None; }
    i32 SelectedBlueprintIndex() const { return mSelectedBlueprint; }
    void ClearBlueprint() { mSelectedBlueprint = -1; }

    // Get 前缀避免与 enum 类型 ActiveModal 同名冲突
    ActiveModal GetActiveModal() const { return mCurrentModal; }
    void ToggleModal(ActiveModal m) { mCurrentModal = (mCurrentModal == m) ? ActiveModal::None : m; }

    // 本帧 UI 是否拦截了屏幕坐标 (用于阻止穿透建造)
    bool IsScreenBlocked(const Vec2& pos) const;

private:
    void RenderTopMechaBar(Mecha& mecha, const Planet* planet, const Star* star, f32 elapsed);
    void RenderResourceBar(const FactorySystem& factory);
    void RenderSidePanels(const FactorySystem& factory, const PowerGrid& power, const DysonSphereManager& dyson);
    void RenderHotbar(const BlueprintManager& blueprints, Mecha& mecha);
    void RenderTechTreeModal(TechTreeManager& techTree);
    void RenderDysonEditorModal(const DysonSphereManager& dyson);
    void RenderGalaxyMapModal(Universe& universe);
    void RenderHelpModal();

    void Block(const Rect& r);
    void DrawItemIcon(ItemKind kind, f32 x, f32 y, f32 size, i32 layer);
    void DrawBuildingIcon(BuildingKind kind, f32 x, f32 y, f32 size, i32 layer);

    BuildingKind mSelectedTool = BuildingKind::None;
    i32 mSelectedBlueprint = -1;
    ActiveModal mCurrentModal = ActiveModal::None;
    i32 mHotbarCategory = 0;

    bool mShowPowerPanel = true;
    bool mShowDysonPanel = true;
    bool mShowThroughputPanel = false;

    Array<Rect> mBlocked;
    Aether::Engine::Renderer2D* mRenderer = nullptr;
};

} // namespace DSP
