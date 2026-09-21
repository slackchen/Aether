#include "DSPUI.h"
#include "UI.h"
#include "Input.h"
#include "DSPArt.h"
#include "DSPAudio.h"
#include <cstdio>
#include <cmath>

namespace DSP
{

using namespace Aether;
using namespace Aether::Math;
namespace UI = Aether::Engine::UI;

// 分类与槽位表
struct CategoryDef
{
    const char* Label;
    Array<HotbarSlot> Slots;
};

static Array<CategoryDef> BuildCategories()
{
    Array<CategoryDef> cats;
    cats.Add({"物流", {{BuildingKind::ConveyorBelt, -1, "传送带"}, {BuildingKind::Sorter, -1, "分拣器"}}});
    cats.Add({"生产", {{BuildingKind::MiningMachine, -1, "采矿机"}, {BuildingKind::OilExtractor, -1, "抽油机"},
                       {BuildingKind::ArcSmelter, -1, "电弧熔炉"}, {BuildingKind::AssemblingMachine, -1, "组装机"},
                       {BuildingKind::ChemicalPlant, -1, "化工厂"}, {BuildingKind::MatrixLab, -1, "矩阵实验室"}}});
    cats.Add({"电力", {{BuildingKind::WindTurbine, -1, "风力涡轮机"}, {BuildingKind::SolarPanel, -1, "太阳能板"},
                       {BuildingKind::TeslaTower, -1, "电浆中继塔"}, {BuildingKind::WirelessPowerTower, -1, "无线输电塔"},
                       {BuildingKind::ThermalPowerPlant, -1, "火力发电厂"}, {BuildingKind::FusionPowerPlant, -1, "微型聚变电站"},
                       {BuildingKind::ArtificialStar, -1, "人造恒星"}}});
    cats.Add({"物流站", {{BuildingKind::PlanetaryLogisticsStation, -1, "行星物流站"},
                         {BuildingKind::InterstellarLogisticsStation, -1, "星际物流站"}}});
    cats.Add({"巨构", {{BuildingKind::EMRailEjector, -1, "电磁弹射器"}, {BuildingKind::VerticalLaunchSilo, -1, "垂直发射井"},
                       {BuildingKind::RayReceiver, -1, "射线接收站"}}});
    cats.Add({"蓝图", {}});
    return cats;
}

static const Array<CategoryDef>& Categories()
{
    static const Array<CategoryDef> cats = BuildCategories();
    return cats;
}

static const char* BuildingDesc(BuildingKind k)
{
    switch (k)
    {
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

bool DSPUI::IsScreenBlocked(const Vec2& pos) const
{
    for (const Rect& r : mBlocked)
    {
        if (r.Contains(pos)) return true;
    }
    return false;
}

void DSPUI::Block(const Rect& r)
{
    mBlocked.Add(r);
}

void DSPUI::DrawItemIcon(ItemKind kind, f32 x, f32 y, f32 size, i32 layer)
{
    auto tex = DSPArt::TexItems();
    if (!tex || !mRenderer) return;
    Vec2 uv0, uv1;
    DSPArt::CellUv(8, 8, (int)kind, uv0, uv1);
    f32 halfH = 360.0f;
    f32 halfW = halfH * mRenderer->Aspect();
    Vec2 cam = mRenderer->GetCamera().Position;
    mRenderer->Sprites().AddUv(tex, uv0, uv1,
                               {x - halfW + cam.x + size * 0.5f, y - halfH + cam.y + size * 0.5f},
                               {size, size}, Color{1, 1, 1, 1}, 0.0f, (f32)layer);
}

void DSPUI::DrawBuildingIcon(BuildingKind kind, f32 x, f32 y, f32 size, i32 layer)
{
    auto tex = DSPArt::TexBuildings();
    if (!tex || !mRenderer) return;
    Vec2 uv0, uv1;
    DSPArt::CellUv(6, 6, (int)kind - 1, uv0, uv1);
    f32 halfH = 360.0f;
    f32 halfW = halfH * mRenderer->Aspect();
    Vec2 cam = mRenderer->GetCamera().Position;
    mRenderer->Sprites().AddUv(tex, uv0, uv1,
                               {x - halfW + cam.x + size * 0.5f, y - halfH + cam.y + size * 0.5f},
                               {size, size}, Color{1, 1, 1, 1}, 0.0f, (f32)layer);
}

void DSPUI::UpdateAndRender(Engine::Renderer2D* renderer,
                            Mecha& mecha,
                            Planet* currentPlanet,
                            FactorySystem& factory,
                            PowerGrid& power,
                            DysonSphereManager& dyson,
                            TechTreeManager& techTree,
                            BlueprintManager& blueprints,
                            Universe& universe,
                            const Vec3& sunDir,
                            f32 elapsed)
{
    (void)sunDir;
    if (!renderer) return;
    mRenderer = renderer;
    mBlocked.Clear();

    UI::BeginFrame(renderer);

    // 全局快捷键
    if (Engine::Input::WasKeyPressed(Engine::KeyCode::T)) ToggleModal(ActiveModal::TechTree);
    if (Engine::Input::WasKeyPressed(Engine::KeyCode::Y)) ToggleModal(ActiveModal::DysonEditor);
    if (Engine::Input::WasKeyPressed(Engine::KeyCode::M)) ToggleModal(ActiveModal::GalaxyMap);
    if (Engine::Input::WasKeyPressed(Engine::KeyCode::H)) ToggleModal(ActiveModal::Help);
    if (Engine::Input::WasKeyPressed(Engine::KeyCode::P)) mShowPowerPanel = !mShowPowerPanel;
    if (Engine::Input::WasKeyPressed(Engine::KeyCode::I)) mShowThroughputPanel = !mShowThroughputPanel;

    RenderTopMechaBar(mecha, currentPlanet, universe.CurrentStar(), elapsed);
    RenderResourceBar(factory);
    RenderSidePanels(factory, power, dyson);
    RenderHotbar(blueprints, mecha);

    if (mCurrentModal == ActiveModal::TechTree) RenderTechTreeModal(techTree);
    else if (mCurrentModal == ActiveModal::DysonEditor) RenderDysonEditorModal(dyson);
    else if (mCurrentModal == ActiveModal::GalaxyMap) RenderGalaxyMapModal(universe);
    else if (mCurrentModal == ActiveModal::Help) RenderHelpModal();

    if (mCurrentModal != ActiveModal::None)
    {
        Block({0, 0, 1280, 720});
    }

    UI::Draw(renderer);
    UI::EndFrame();
    mRenderer = nullptr;
}

// ---------------------------------------------------------------------------
// 顶部机甲状态栏
// ---------------------------------------------------------------------------
void DSPUI::RenderTopMechaBar(Mecha& mecha, const Planet* planet, const Star* star, f32 elapsed)
{
    auto& th = UI::GetTheme();
    f32 x = 12, y = 10, w = 452, h = 64;
    UI::DrawPanel({x, y, w, h}, nullptr, nullptr, nullptr, 25);
    Block({x, y, w, h});

    char coreBuf[32];
    snprintf(coreBuf, sizeof(coreBuf), "核心 MK.%u", mecha.CoreLevel());
    UI::Badge(coreBuf, x + 8, y + 14, th.BgDark, th.AccentCyan, 26);

    char energyLbl[64];
    snprintf(energyLbl, sizeof(energyLbl), "%.0f / %.0f MJ (%.0f%%)",
             mecha.EnergyMJ(), mecha.MaxEnergyMJ(), mecha.EnergyRatio() * 100.0f);
    Color fill = mecha.IsLowEnergy() ? th.AccentRed : th.AccentCyan;
    UI::ProgressBar({x + 96, y + 8, 210, 15}, mecha.EnergyRatio(), fill, nullptr, energyLbl, 26);

    char flowBuf[64];
    snprintf(flowBuf, sizeof(flowBuf), "耗电 %.1f MW · 充能 %.1f MW", mecha.PowerDrawMW(), mecha.ChargeRateMW());
    UI::DrawText(flowBuf, x + 96, y + 32, 11, 0, th.TextSecondary, 26);

    const FuelItem& fuel = mecha.FuelSlot();
    char fuelBuf[48];
    if (fuel.Count > 0)
    {
        const char* fName = (fuel.Kind == FuelKind::Coal) ? "煤炭" :
                            (fuel.Kind == FuelKind::HydrogenFuelRod) ? "氢燃料棒" :
                            (fuel.Kind == FuelKind::DeuteronFuelRod) ? "氘核燃料棒" : "反物质胶囊";
        snprintf(fuelBuf, sizeof(fuelBuf), "%s ×%u", fName, fuel.Count);
    }
    else
    {
        snprintf(fuelBuf, sizeof(fuelBuf), "燃料舱: 空");
    }
    UI::Badge(fuelBuf, x + 316, y + 12, th.BgCard, fuel.Count > 0 ? th.AccentGold : th.AccentRed, 26);

    const char* modeStr = (mecha.Mode() == MechaMode::GroundWalk) ? "地表行走" :
                          (mecha.Mode() == MechaMode::LowFlight) ? "喷气飞行" :
                          (mecha.Mode() == MechaMode::SpaceSailing) ? "太空航行" : "曲率飞行";
    Color modeCol = (mecha.Mode() == MechaMode::GroundWalk) ? th.AccentGreen :
                    (mecha.Mode() == MechaMode::LowFlight) ? th.AccentBlue :
                    (mecha.Mode() == MechaMode::SpaceSailing) ? th.AccentCyan : th.AccentPurple;
    UI::Badge(modeStr, x + 316, y + 36, th.BgDark, modeCol, 26);

    // 中央: 天体信息 + 昼夜时钟
    f32 lx = (1280.0f - 400.0f) * 0.5f;
    UI::DrawPanel({lx, y, 400, h}, nullptr, nullptr, nullptr, 25);
    Block({lx, y, 400, h});
    const char* sName = star ? star->Name.CStr() : "深空";
    const char* pName = planet ? planet->Name().CStr() : "轨道空间";
    UI::DrawText(sName, lx + 14, y + 12, 12, 0.5f, th.TextAccent, 26);
    UI::DrawText(pName, lx + 14, y + 30, 12, 0.5f, th.TextPrimary, 26);

    // 昼夜: 太阳方向与行星的方位角 → 当地时间
    f32 sunA = elapsed * 0.013f;
    f32 dayFrac = fmodf(sunA / Math::TWO_PI + 0.5f, 1.0f);
    i32 hh = (i32)(dayFrac * 24.0f);
    i32 mm = (i32)fmodf(dayFrac * 24.0f * 60.0f, 60.0f);
    bool isDay = dayFrac > 0.25f && dayFrac < 0.75f;
    char timeBuf[40];
    snprintf(timeBuf, sizeof(timeBuf), "%s %02d:%02d", isDay ? "[昼]" : "[夜]", hh, mm);
    UI::DrawText(timeBuf, lx + 14, y + 46, 12, 1.0f, isDay ? th.AccentGold : th.TextMuted, 26);

    // 右上: 界面按钮
    f32 btnX = 866, bw = 66, bh = 30;
    const char* labels[] = {"[T] 科技", "[Y] 戴森球", "[M] 星图", "[H] 帮助", "[P] 电网", "[I] 产线"};
    for (int i = 0; i < 6; i++)
    {
        Rect br = {btnX + (f32)(i % 3) * (bw + 6), y + (f32)(i / 3) * (bh + 4), bw, bh};
        bool active = (i == 0 && mCurrentModal == ActiveModal::TechTree) ||
                      (i == 1 && mCurrentModal == ActiveModal::DysonEditor) ||
                      (i == 2 && mCurrentModal == ActiveModal::GalaxyMap) ||
                      (i == 3 && mCurrentModal == ActiveModal::Help) ||
                      (i == 4 && mShowPowerPanel) || (i == 5 && mShowThroughputPanel);
        if (UI::Button(labels[i], br, active, 26))
        {
            DSPAudioManager::PlayClick();
            switch (i)
            {
                case 0: ToggleModal(ActiveModal::TechTree); break;
                case 1: ToggleModal(ActiveModal::DysonEditor); break;
                case 2: ToggleModal(ActiveModal::GalaxyMap); break;
                case 3: ToggleModal(ActiveModal::Help); break;
                case 4: mShowPowerPanel = !mShowPowerPanel; break;
                case 5: mShowThroughputPanel = !mShowThroughputPanel; break;
            }
        }
    }
    Block({btnX - 4, y, 3 * (bw + 6) + 8, 2 * (bh + 4) + 4});
}

// ---------------------------------------------------------------------------
// 资源栏
// ---------------------------------------------------------------------------
void DSPUI::RenderResourceBar(const FactorySystem& factory)
{
    auto& th = UI::GetTheme();
    static const ItemKind tracked[] =
    {
        ItemKind::IronIngot, ItemKind::CopperIngot, ItemKind::Magnet, ItemKind::MagneticCoil,
        ItemKind::CircuitBoard, ItemKind::Processor, ItemKind::TitaniumIngot,
        ItemKind::HighPuritySilicon, ItemKind::MatrixBlue, ItemKind::SolarSail, ItemKind::SmallCarrierRocket
    };
    static const char* names[] = {"铁块", "铜块", "磁铁", "线圈", "电路板", "处理器", "钛块", "高纯硅", "蓝矩阵", "太阳帆", "火箭"};

    f32 slotW = 74.0f;
    f32 x0 = 12.0f, y0 = 82.0f;
    f32 w = slotW * 11.0f + 8.0f;
    UI::DrawRect({x0, y0, w, 44}, Color{0.03f, 0.05f, 0.09f, 0.72f}, 24);
    UI::DrawRectOutline({x0, y0, w, 44}, 1.0f, Color{0.2f, 0.35f, 0.55f, 0.5f}, 24);
    Block({x0, y0, w, 44});

    for (int i = 0; i < 11; i++)
    {
        f32 sx = x0 + 4.0f + (f32)i * slotW;
        u32 count = factory.TotalStored(tracked[i]);
        DrawItemIcon(tracked[i], sx + 2, y0 + 8, 26, 27);
        char cnt[16];
        if (count >= 10000) snprintf(cnt, sizeof(cnt), "%.1fk", (f32)count / 1000.0f);
        else snprintf(cnt, sizeof(cnt), "%u", count);
        UI::DrawText(cnt, sx + 30, y0 + 10, 12, 0, th.TextPrimary, 27);
        UI::DrawText(names[i], sx + 30, y0 + 25, 10, 0, th.TextMuted, 27);
    }
}

// ---------------------------------------------------------------------------
// 右侧信息面板
// ---------------------------------------------------------------------------
void DSPUI::RenderSidePanels(const FactorySystem& factory, const PowerGrid& power, const DysonSphereManager& dyson)
{
    auto& th = UI::GetTheme();
    f32 px = 1014.0f, pw = 254.0f, py = 82.0f;

    if (mShowPowerPanel)
    {
        f32 h = 140.0f;
        UI::DrawPanel({px, py, pw, h}, "行星电网", nullptr, nullptr, 25);
        Block({px, py, pw, h});

        char genBuf[64], demBuf[64], satBuf[32];
        snprintf(genBuf, sizeof(genBuf), "发电 %.1f MW", power.TotalGenerationKW() * 0.001f);
        snprintf(demBuf, sizeof(demBuf), "耗电 %.1f MW", power.TotalDemandKW() * 0.001f);
        snprintf(satBuf, sizeof(satBuf), "满足率 %.0f%%", power.SatisfactionRatio() * 100.0f);
        UI::DrawText(genBuf, px + 12, py + 38, 12, 0, th.AccentGreen, 26);
        UI::DrawText(demBuf, px + 130, py + 38, 12, 0, th.AccentOrange, 26);
        Color satCol = power.SatisfactionRatio() >= 1.0f ? th.AccentGreen : th.AccentRed;
        UI::ProgressBar({px + 12, py + 56, pw - 24, 16}, power.SatisfactionRatio(), satCol, nullptr, satBuf, 26);
        char accumBuf[64];
        snprintf(accumBuf, sizeof(accumBuf), "蓄电池 %.0f / %.0f MJ", power.AccumulatorChargeMJ(), power.AccumulatorMaxMJ());
        UI::ProgressBar({px + 12, py + 80, pw - 24, 14},
                        power.AccumulatorChargeMJ() / power.AccumulatorMaxMJ(), th.AccentBlue, nullptr, accumBuf, 26);
        if (power.IsMechaCharging())
        {
            UI::Badge("机甲无线快充中", px + 12, py + 112, th.BgDark, th.AccentCyan, 26);
        }
        py += h + 10;
    }

    if (mShowDysonPanel)
    {
        f32 h = 150.0f;
        UI::DrawPanel({px, py, pw, h}, "戴森球工程", nullptr, nullptr, 25);
        Block({px, py, pw, h});

        char sailBuf[64], nodeBuf[64], outBuf[64];
        snprintf(sailBuf, sizeof(sailBuf), "在轨太阳帆 %u 枚", dyson.SailCount());
        snprintf(nodeBuf, sizeof(nodeBuf), "骨架节点 %u / %u", dyson.CompletedNodes(), dyson.NodeCount());
        snprintf(outBuf, sizeof(outBuf), "总发电 %.3f GW", dyson.TotalGenerationGW());
        UI::DrawText(sailBuf, px + 12, py + 38, 12, 0.5f, th.AccentGold, 26);
        UI::DrawText(nodeBuf, px + 12, py + 58, 12, 0.5f, th.AccentCyan, 26);
        UI::DrawText(outBuf, px + 12, py + 82, 15, 1.0f, th.AccentGold, 26, false, 0.4f);
        f32 nodeRatio = dyson.NodeCount() > 0 ? (f32)dyson.CompletedNodes() / (f32)dyson.NodeCount() : 0.0f;
        UI::ProgressBar({px + 12, py + 106, pw - 24, 12}, nodeRatio, th.AccentCyan, nullptr, nullptr, 26);
        char launchBuf[64];
        snprintf(launchBuf, sizeof(launchBuf), "累计发射: 帆 %u | 火箭 %u", factory.LaunchedSolarSails(), factory.LaunchedCarrierRockets());
        UI::DrawText(launchBuf, px + 12, py + 126, 11, 0, th.TextMuted, 26);
        py += h + 10;
    }

    if (mShowThroughputPanel)
    {
        f32 h = 168.0f;
        UI::DrawPanel({px, py, pw, h}, "产线统计 (/分)", nullptr, nullptr, 25);
        Block({px, py, pw, h});
        ItemKind items[] = {ItemKind::IronIngot, ItemKind::CopperIngot, ItemKind::CircuitBoard,
                            ItemKind::MagneticCoil, ItemKind::Processor, ItemKind::MatrixBlue};
        f32 sy = py + 38;
        for (int i = 0; i < 6; i++)
        {
            f32 prod = factory.GetProductionRate(items[i]);
            f32 cons = factory.GetConsumptionRate(items[i]);
            DrawItemIcon(items[i], px + 12, sy - 2, 18, 27);
            char line[48];
            snprintf(line, sizeof(line), "+%.0f / -%.0f", prod, cons);
            UI::DrawText(line, px + 40, sy, 12, 0, prod >= cons ? th.TextPrimary : th.AccentOrange, 26);
            sy += 21;
        }
    }
}

// ---------------------------------------------------------------------------
// 底部建造栏
// ---------------------------------------------------------------------------
void DSPUI::RenderHotbar(const BlueprintManager& blueprints, Mecha& mecha)
{
    auto& th = UI::GetTheme();

    f32 dockW = 1000.0f, dockH = 118.0f;
    f32 dockX = (1280.0f - dockW) * 0.5f;
    f32 dockY = 720.0f - dockH - 8.0f;
    UI::DrawPanel({dockX, dockY, dockW, dockH}, nullptr, nullptr, nullptr, 25);
    Block({dockX, dockY, dockW, dockH});

    // 分类页签
    const Array<CategoryDef>& cats = Categories();
    f32 tabW = 86.0f, tabH = 22.0f;
    for (i32 i = 0; i < (i32)cats.Count(); i++)
    {
        Rect tr = {dockX + 8.0f + (f32)i * (tabW + 4.0f), dockY + 6.0f, tabW, tabH};
        if (UI::Button(cats[i].Label, tr, mHotbarCategory == i, 26))
        {
            DSPAudioManager::PlayClick();
            mHotbarCategory = i;
        }
    }

    // 当前分类槽位
    Array<HotbarSlot> slots;
    if (mHotbarCategory == 5)
    {
        i32 n = Math::Min((i32)blueprints.Presets().Count(), 5);
        for (i32 i = 0; i < n; i++) slots.Add({BuildingKind::None, i, blueprints.Presets()[i].Name.CStr()});
    }
    else
    {
        slots = cats[mHotbarCategory].Slots;
    }

    f32 slotW = 88.0f, slotH = 58.0f;
    f32 sx0 = dockX + 10.0f, sy0 = dockY + 36.0f;
    char selTitle[96] = "";

    for (i32 i = 0; i < (i32)slots.Count() && i < 10; i++)
    {
        const HotbarSlot& s = slots[i];
        Rect r = {sx0 + (f32)i * (slotW + 6.0f), sy0, slotW, slotH};
        bool selected = (s.Kind != BuildingKind::None) ? (mSelectedTool == s.Kind) : (mSelectedBlueprint == s.BlueprintIndex);

        UI::DrawRect(r, selected ? th.BgActive : th.BgCard, 26);
        UI::DrawRectOutline(r, 1.5f, selected ? th.BorderActive : th.Border, 26);

        if (s.Kind != BuildingKind::None)
        {
            DrawBuildingIcon(s.Kind, r.X + (slotW - 36) * 0.5f, r.Y + 4, 36, 27);
        }
        else
        {
            UI::DrawText("[图]", r.X + slotW * 0.5f, r.Y + 12, 16, 0, th.AccentGold, 27, true);
        }

        // 热键角标
        char hk[4];
        snprintf(hk, sizeof(hk), "%d", (i + 1) % 10);
        UI::DrawText(hk, r.X + 4, r.Y + 3, 10, 0, th.TextMuted, 27);

        UI::DrawText(s.Name, r.X + slotW * 0.5f, r.Y + slotH - 12, 11, 0, th.TextPrimary, 27, true);

        Vec2 mp = Engine::Input::MousePos();
        if (r.Contains(mp))
        {
            if (s.Kind != BuildingKind::None)
            {
                char tip[128];
                f32 kw = BuildingPowerDemandKW(s.Kind);
                if (kw > 0) snprintf(tip, sizeof(tip), "%s | 耗电 %.0f kW — %s", s.Name, kw, BuildingDesc(s.Kind));
                else snprintf(tip, sizeof(tip), "%s — %s", s.Name, BuildingDesc(s.Kind));
                UI::Tooltip(tip, {r.X + slotW * 0.5f, r.Y - 8}, 35);
            }
            else
            {
                UI::Tooltip(blueprints.Presets()[s.BlueprintIndex].Desc.CStr(), {r.X + slotW * 0.5f, r.Y - 8}, 35);
            }
        }

        if (UI::Button("", r, selected, 28))
        {
            DSPAudioManager::PlayClick();
            if (s.Kind != BuildingKind::None)
            {
                mSelectedTool = selected ? BuildingKind::None : s.Kind;
                mSelectedBlueprint = -1;
            }
            else
            {
                mSelectedBlueprint = selected ? -1 : s.BlueprintIndex;
                mSelectedTool = BuildingKind::None;
            }
        }

        if (selected) snprintf(selTitle, sizeof(selTitle), "%s", s.Name);
    }

    // 数字键快捷选择
    static const Engine::KeyCode numKeys[10] =
    {
        Engine::KeyCode::Num1, Engine::KeyCode::Num2, Engine::KeyCode::Num3,
        Engine::KeyCode::Num4, Engine::KeyCode::Num5, Engine::KeyCode::Num6,
        Engine::KeyCode::Num7, Engine::KeyCode::Num8, Engine::KeyCode::Num9,
        Engine::KeyCode::Num0
    };
    for (i32 i = 0; i < 10; i++)
    {
        if (Engine::Input::WasKeyPressed(numKeys[i]))
        {
            if (i < (i32)slots.Count())
            {
                const HotbarSlot& s = slots[i];
                if (s.Kind != BuildingKind::None)
                {
                    bool was = (mSelectedTool == s.Kind);
                    mSelectedTool = was ? BuildingKind::None : s.Kind;
                    mSelectedBlueprint = -1;
                    if (!was) snprintf(selTitle, sizeof(selTitle), "%s", s.Name);
                }
                else
                {
                    bool was = (mSelectedBlueprint == s.BlueprintIndex);
                    mSelectedBlueprint = was ? -1 : s.BlueprintIndex;
                    mSelectedTool = BuildingKind::None;
                }
                DSPAudioManager::PlayClick();
            }
        }
    }

    // 选中提示与操作说明
    if (selTitle[0])
    {
        char hint[96];
        snprintf(hint, sizeof(hint), "建造: %s — 左键放置, R 旋转, 拖拽铺带, Esc 取消", selTitle);
        UI::DrawText(hint, dockX + dockW * 0.5f, dockY - 20, 13, 0.5f, th.AccentCyan, 27, true, 0.3f);
    }

    // 起飞按钮
    Rect flyR = {dockX + dockW - 118.0f, sy0 + 8.0f, 100.0f, 42.0f};
    bool flying = (mecha.Mode() != MechaMode::GroundWalk);
    if (UI::Button(flying ? "降落 [空格]" : "起飞 [空格]", flyR, flying, 26))
    {
        mecha.ToggleFlight();
        DSPAudioManager::PlayClick();
    }
}

// ---------------------------------------------------------------------------
// 科技树
// ---------------------------------------------------------------------------
void DSPUI::RenderTechTreeModal(TechTreeManager& techTree)
{
    auto& th = UI::GetTheme();
    f32 mw = 900.0f, mh = 580.0f;
    f32 mx = (1280.0f - mw) * 0.5f, my = (720.0f - mh) * 0.5f;

    UI::DrawRect({0, 0, 1280, 720}, Color{0, 0, 0, 0.72f}, 30);
    UI::DrawPanel({mx, my, mw, mh}, "矩阵科研中心", nullptr, nullptr, 31);

    if (UI::Button("关闭 [Esc]", {mx + mw - 110, my + 6, 95, 24}, false, 32))
    {
        DSPAudioManager::PlayClick();
        mCurrentModal = ActiveModal::None;
    }

    f32 y = my + 46;
    for (const TechNode& t : techTree.AllTechs())
    {
        Rect tr = {mx + 20, y, mw - 40, 36};
        bool isCurrent = (techTree.CurrentResearch() == t.Id);
        Color border = t.Unlocked ? th.AccentGreen :
                       isCurrent ? th.AccentCyan :
                       (techTree.CanResearch(t.Id) ? th.BorderBright : th.Border);
        UI::DrawRect(tr, isCurrent ? th.BgActive : th.BgCard, 32);
        UI::DrawRectOutline(tr, 1.5f, border, 32);

        const char* status = t.Unlocked ? "已解锁" : isCurrent ? "研发中" : (techTree.CanResearch(t.Id) ? "可研发" : "未解锁");
        Color badgeCol = t.Unlocked ? th.AccentGreen : (isCurrent ? th.AccentCyan : th.TextMuted);
        UI::Badge(status, mx + 30, y + 18, th.BgDark, badgeCol, 33);
        UI::DrawText(t.Name.CStr(), mx + 105, y + 18, 13, 0.5f, th.TextPrimary, 33);
        UI::DrawText(t.Desc.CStr(), mx + 330, y + 18, 11, 0, th.TextMuted, 33);

        if (!t.Unlocked && techTree.CanResearch(t.Id) && !isCurrent)
        {
            if (UI::Button("开始研究", {mx + mw - 125, y + 5, 100, 26}, false, 33))
            {
                techTree.SelectResearch(t.Id);
                DSPAudioManager::PlayClick();
            }
        }
        else if (isCurrent && !t.Unlocked)
        {
            char prog[16];
            snprintf(prog, sizeof(prog), "%.0f%%", techTree.CurrentProgressRatio() * 100.0f);
            UI::ProgressBar({mx + mw - 125, y + 7, 100, 20}, techTree.CurrentProgressRatio(), th.AccentCyan, nullptr, prog, 33);
        }
        y += 40;
    }
}

// ---------------------------------------------------------------------------
// 戴森球编辑器
// ---------------------------------------------------------------------------
void DSPUI::RenderDysonEditorModal(const DysonSphereManager& dyson)
{
    auto& th = UI::GetTheme();
    f32 mw = 820.0f, mh = 480.0f;
    f32 mx = (1280.0f - mw) * 0.5f, my = (720.0f - mh) * 0.5f;

    UI::DrawRect({0, 0, 1280, 720}, Color{0, 0, 0, 0.72f}, 30);
    UI::DrawPanel({mx, my, mw, mh}, "戴森球轨道规划", nullptr, nullptr, 31);
    if (UI::Button("关闭 [Esc]", {mx + mw - 110, my + 6, 95, 24}, false, 32))
    {
        DSPAudioManager::PlayClick();
        mCurrentModal = ActiveModal::None;
    }

    f32 lw = 380.0f;
    UI::DrawPanel({mx + 20, my + 45, lw, mh - 65}, "戴森云 (太阳帆)", nullptr, nullptr, 32);
    char buf[80];
    snprintf(buf, sizeof(buf), "在轨太阳帆: %u 枚", dyson.SailCount());
    UI::DrawText(buf, mx + 35, my + 85, 14, 0.5f, th.AccentGold, 33);
    snprintf(buf, sizeof(buf), "戴森云发电: %.3f GW", dyson.SwarmGenerationGW());
    UI::DrawText(buf, mx + 35, my + 115, 13, 0.5f, th.TextPrimary, 33);
    UI::DrawText("由电磁弹射器发射太阳帆组成\n环绕恒星的能源收集阵列。", mx + 35, my + 150, 12, 0, th.TextSecondary, 33);

    f32 rx = mx + lw + 40.0f, rw = mw - lw - 60.0f;
    UI::DrawPanel({rx, my + 45, rw, mh - 65}, "戴森壳 (骨架节点)", nullptr, nullptr, 32);
    snprintf(buf, sizeof(buf), "结构节点: %u / %u", dyson.CompletedNodes(), dyson.NodeCount());
    UI::DrawText(buf, rx + 15, my + 85, 13, 0.5f, th.AccentCyan, 33);
    snprintf(buf, sizeof(buf), "骨架连结: %u 条", (u32)dyson.Struts().Count());
    UI::DrawText(buf, rx + 15, my + 115, 13, 0.5f, th.TextPrimary, 33);
    snprintf(buf, sizeof(buf), "戴森壳发电: %.3f GW", dyson.ShellGenerationGW());
    UI::DrawText(buf, rx + 15, my + 145, 15, 0.5f, th.AccentGold, 33, false, 0.4f);
    f32 ratio = dyson.NodeCount() > 0 ? (f32)dyson.CompletedNodes() / (f32)dyson.NodeCount() : 0.0f;
    UI::ProgressBar({rx + 15, my + 180, rw - 30, 16}, ratio, th.AccentCyan, nullptr, "工程进度", 33);
    UI::DrawText("由垂直发射井发射运载火箭\n逐节点搭建测地线壳体。", rx + 15, my + 215, 12, 0, th.TextSecondary, 33);
}

// ---------------------------------------------------------------------------
// 星图
// ---------------------------------------------------------------------------
void DSPUI::RenderGalaxyMapModal(Universe& universe)
{
    auto& th = UI::GetTheme();
    f32 mw = 860.0f, mh = 560.0f;
    f32 mx = (1280.0f - mw) * 0.5f, my = (720.0f - mh) * 0.5f;

    UI::DrawRect({0, 0, 1280, 720}, Color{0, 0, 0, 0.72f}, 30);
    UI::DrawPanel({mx, my, mw, mh}, "银河星团星图 (曲率跃迁)", nullptr, nullptr, 31);
    if (UI::Button("关闭 [Esc]", {mx + mw - 110, my + 6, 95, 24}, false, 32))
    {
        DSPAudioManager::PlayClick();
        mCurrentModal = ActiveModal::None;
    }

    f32 y = my + 48;
    for (u32 i = 0; i < (u32)universe.Stars().Count(); i++)
    {
        const auto& s = universe.Stars()[i];
        Rect sr = {mx + 20, y, mw - 40, 108};
        UI::DrawPanel(sr, nullptr, nullptr, nullptr, 32);

        Color sc = StarColor(s->Type);
        // 恒星图标
        f32 cxp = mx + 52, cyp = y + 40;
        UI::DrawRect({cxp - 14, cyp - 14, 28, 28}, Color{sc.r, sc.g, sc.b, 0.35f}, 33);
        UI::DrawRect({cxp - 9, cyp - 9, 18, 18}, sc, 33);

        char specBuf[96], posBuf[96];
        snprintf(specBuf, sizeof(specBuf), "%s | 光度 %.2fx | 行星 %u 颗",
                 SpectralName(s->Type), s->Luminosity, (u32)s->Planets.Count());
        snprintf(posBuf, sizeof(posBuf), "坐标 (%.1f, %.1f, %.1f) 光年", s->PosLy.x, s->PosLy.y, s->PosLy.z);
        UI::DrawText(s->Name.CStr(), mx + 80, y + 16, 15, 0.5f, sc, 33);
        UI::DrawText(specBuf, mx + 80, y + 40, 12, 0, th.TextPrimary, 33);
        UI::DrawText(posBuf, mx + 80, y + 60, 11, 0, th.TextMuted, 33);

        // 行星列表
        f32 plx = mx + 80;
        for (u32 pi = 0; pi < (u32)s->Planets.Count() && pi < 6; pi++)
        {
            const auto& p = s->Planets[pi];
            UI::DrawRect({plx, y + 80, 90, 18}, Color{p->AtmosphereColor().r * 0.5f, p->AtmosphereColor().g * 0.5f, p->AtmosphereColor().b * 0.5f, 0.8f}, 33);
            UI::DrawText(p->Name().CStr(), plx + 4, y + 84, 9, 0, th.TextPrimary, 34);
            plx += 94;
        }

        if (UI::Button("跃迁", {mx + mw - 135, y + 16, 90, 30}, false, 33))
        {
            universe.SelectStar(i);
            DSPAudioManager::PlayWarp();
            mCurrentModal = ActiveModal::None;
        }
        y += 116;
    }
}

// ---------------------------------------------------------------------------
// 帮助
// ---------------------------------------------------------------------------
void DSPUI::RenderHelpModal()
{
    auto& th = UI::GetTheme();
    f32 mw = 640.0f, mh = 520.0f;
    f32 mx = (1280.0f - mw) * 0.5f, my = (720.0f - mh) * 0.5f;

    UI::DrawRect({0, 0, 1280, 720}, Color{0, 0, 0, 0.72f}, 30);
    UI::DrawPanel({mx, my, mw, mh}, "操作指南", nullptr, nullptr, 31);
    if (UI::Button("关闭 [Esc]", {mx + mw - 110, my + 6, 95, 24}, false, 32))
    {
        DSPAudioManager::PlayClick();
        mCurrentModal = ActiveModal::None;
    }

    static const char* lines[] =
    {
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
    for (const char* l : lines)
    {
        UI::DrawText(l, mx + 30, y, 14, 0.5f, th.TextPrimary, 33);
        y += 30;
    }
    UI::DrawText("提示: 采矿机建在矿脉上, 传送带沿箭头把货物送进机器的入口。", mx + 30, my + mh - 50, 12, 0, th.AccentCyan, 33);
    UI::DrawText("目标: 发展工厂, 向恒星发射太阳帆与火箭, 建成戴森球!", mx + 30, my + mh - 30, 12, 0, th.AccentGold, 33);
}

} // namespace DSP
