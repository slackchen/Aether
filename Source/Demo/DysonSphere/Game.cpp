#include "Game.h"
#include "Texture.h"
#include "Input.h"
#include "UI.h"
#include "DSPArt.h"
#include "DSPAudio.h"
#include "PlanetGrid.h"
#include <cmath>
#include <cstdio>

namespace DSP
{

using namespace Aether;
using namespace Aether::Math;

static f32 Clamp01(f32 v) { return Math::Clamp(v, 0.0f, 1.0f); }
static Color MulColor(Color c, f32 m) { return {c.r * m, c.g * m, c.b * m, c.a}; }

// 各建筑的世界尺寸 (米)
static f32 BuildingWorldSize(BuildingKind k)
{
    switch (k)
    {
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

Game::Game(Engine::Renderer2D* renderer, Engine::Timer* timer)
    : mRenderer(renderer), mTimer(timer), mUniverse(2026)
{
    if (mRenderer && mRenderer->Device())
    {
        mSolidTex = Engine::MakeSolidTexture(mRenderer->Device(), 4, 4, {1.0f, 1.0f, 1.0f, 1.0f});
        mGlowTex = Engine::MakeGlowTexture(mRenderer->Device(), 128);
        DSPArt::Init(mRenderer->Device());
        mWorld3D.Init(mRenderer->Device(), mRenderer->ColorFormat(), mRenderer->DepthFormat());
        EnsureTerrainMeshes(mUniverse.CurrentPlanet());
    }
    DSPAudioManager::Init();
    InitStartingFactory();
    Engine::UI::Flash("欢迎来到伊卡洛斯 — 建造你的戴森球文明 (G: 平面/球面地形视图)", 4.0f);
}

Game::~Game()
{
    DSPArt::Shutdown();
}

void Game::EnsureTerrainMeshes(Planet* planet)
{
    if (!planet || planet == mBuiltPlanet || !mWorld3D.IsReady()) return;
    mBuiltPlanet = planet;
    const TerrainField& field = planet->GetTerrainField();
    f32 R = planet->Radius();
    // 球形系统: 立方体球六面网格, 高度/配色来自同一 TerrainField (+Y 径向高度)
    mWorld3D.BuildTerrainSphere(field, R, 96);
    mWorld3D.BuildWaterSphere(R, 48);          // 海平面 = R
    mWorld3D.BuildAtmosphere(R * 1.06f, 48);
}

// 平面调试视图: 在机甲脚下沿切平面展开同一块地形高度场 (G 键切换)
void Game::RebuildFlatViewMesh()
{
    Planet* planet = mUniverse.CurrentPlanet();
    if (!planet || !mWorld3D.IsReady()) return;
    Vec3 mpos = mMecha.Position();
    f32 len = sqrtf(mpos.x * mpos.x + mpos.y * mpos.y + mpos.z * mpos.z);
    Vec3 up = len > 1.0f ? Vec3{mpos.x / len, mpos.y / len, mpos.z / len} : Vec3{0.0f, 1.0f, 0.0f};
    Vec3 ref = fabsf(up.y) > 0.92f ? Vec3{1.0f, 0.0f, 0.0f} : Vec3{0.0f, 1.0f, 0.0f};
    Vec3 east =
    {
        ref.y * up.z - ref.z * up.y,
        ref.z * up.x - ref.x * up.z,
        ref.x * up.y - ref.y * up.x
    };
    f32 el = sqrtf(east.x * east.x + east.y * east.y + east.z * east.z);
    east = {east.x / el, east.y / el, east.z / el};
    Vec3 north =
    {
        east.y * up.z - east.z * up.y,
        east.z * up.x - east.x * up.z,
        east.x * up.y - east.y * up.x
    };
    Vec3 center = {up.x * (len + 2.0f), up.y * (len + 2.0f), up.z * (len + 2.0f)};
    const TerrainField& field = planet->GetTerrainField();
    mWorld3D.BuildTerrainPlanar(field, center, east, north, up, 1400.0f, 160);
    mWorld3D.BuildWaterPlanar(center, east, north, up, 1400.0f, 6.0f);
}

u32 Game::FindSpawnTile()
{
    Planet* p = mUniverse.CurrentPlanet();
    if (!p) return 0;
    constexpr u32 N = PlanetGrid::TILES_PER_FACE;
    u32 cu = N / 2, cv = N / 2;
    for (u32 r = 0; r < N / 2; r++)
    {
        for (i32 du = -(i32)r; du <= (i32)r; du++)
        {
            for (i32 dv = -(i32)r; dv <= (i32)r; dv++)
            {
                if (Math::Max(Math::Abs(du), Math::Abs(dv)) != (i32)r) continue;
                i32 u = Math::Clamp((i32)cu + du, 0, (i32)N - 1);
                i32 v = Math::Clamp((i32)cv + dv, 0, (i32)N - 1);
                u32 key = PlanetGrid::Key(2, (u8)u, (u8)v);
                if (p->IsBuildableTile(key)) return key;
            }
        }
    }
    return PlanetGrid::Key(2, (u8)cu, (u8)cv);
}

void Game::InitStartingFactory()
{
    Planet* p = mUniverse.CurrentPlanet();
    if (!p) return;

    u32 spawn = FindSpawnTile();
    mMecha.SetPosition(p->Grid().TileCenter(p->Radius(), spawn) +
                       PlanetGrid::TileNormal(spawn) * 1.5f);

    auto findVein = [&](int resource) -> u32 {
        for (u32 r = 1; r < 24; r++)
        {
            for (int d = 0; d < 4; d++)
            {
                u32 k = spawn;
                for (u32 s = 0; s < r; s++) k = PlanetGrid::NeighborKey(k, d);
                const Tile& t = p->Grid().GetTile(k);
                if ((int)t.Resource == resource && t.ResourceAmount > 0 && p->IsBuildableTile(k)) return k;
            }
        }
        return 0;
    };
    u32 ironTile = findVein(1);

    auto place = [&](BuildingKind kind, u32 key, i32 dir, ItemKind recipe = ItemKind::None) -> u32 {
        if (!key || !p->IsBuildableTile(key)) return 0;
        GridPos gp = GridPos::FromKey(key);
        Vec3 pos = p->Grid().TileCenter(p->Radius(), key);
        u32 id = mFactory.PlaceBuilding(kind, gp, pos, (f32)dir * Math::HALF_PI, recipe);
        if (id)
        {
            p->Grid().GetTile(key).BuildingId = (u16)id;
            if (auto* b = mFactory.GetBuilding(id)) b->Dir = dir;
        }
        return id;
    };

    place(BuildingKind::WirelessPowerTower, PlanetGrid::NeighborKey(spawn, 0), 0);
    place(BuildingKind::WindTurbine, PlanetGrid::NeighborKey(spawn, 2), 0);
    place(BuildingKind::WindTurbine, PlanetGrid::NeighborKey(spawn, 3), 0);
    place(BuildingKind::SolarPanel, PlanetGrid::NeighborKey(PlanetGrid::NeighborKey(spawn, 1), 2), 0);
    place(BuildingKind::SolarPanel, PlanetGrid::NeighborKey(PlanetGrid::NeighborKey(spawn, 1), 3), 0);

    if (ironTile)
    {
        place(BuildingKind::MiningMachine, ironTile, 0);
        u32 b1 = PlanetGrid::NeighborKey(ironTile, 0);
        u32 b2 = PlanetGrid::NeighborKey(b1, 0);
        u32 b3 = PlanetGrid::NeighborKey(b2, 0);
        place(BuildingKind::ConveyorBelt, b1, 0);
        place(BuildingKind::ConveyorBelt, b2, 0);
        place(BuildingKind::ConveyorBelt, b3, 0);
        u32 smelt = PlanetGrid::NeighborKey(b3, 0);
        place(BuildingKind::ArcSmelter, smelt, 0, ItemKind::IronIngot);
        u32 out1 = PlanetGrid::NeighborKey(smelt, 0);
        place(BuildingKind::ConveyorBelt, out1, 0);
        u32 asm1 = PlanetGrid::NeighborKey(out1, 0);
        place(BuildingKind::AssemblingMachine, asm1, 0, ItemKind::MagneticCoil);
    }

    place(BuildingKind::MatrixLab, PlanetGrid::NeighborKey(PlanetGrid::NeighborKey(spawn, 0), 2), 0, ItemKind::MatrixBlue);
    place(BuildingKind::EMRailEjector, PlanetGrid::NeighborKey(PlanetGrid::NeighborKey(spawn, 0), 3), 0);
}

// ---------------------------------------------------------------------------
// 输入
// ---------------------------------------------------------------------------
void Game::HandleInput(f32 dt)
{
    mClickCooldown = Math::Max(0.0f, mClickCooldown - dt);
    Planet* planet = mUniverse.CurrentPlanet();
    DSPCamera& cam = mUniverse.Camera();

    Vec2 mdelta = Engine::Input::MouseDelta();
    f32 mwheel = Engine::Input::MouseWheel();
    bool rDown = Engine::Input::IsMouseDown(Engine::MouseButton::Right);
    cam.HandleInput(mdelta, mwheel, rDown, false);

    Vec2 wasd{0.0f, 0.0f};
    if (Engine::Input::IsKeyDown(Engine::KeyCode::W) || Engine::Input::IsDown(Engine::Key::Up)) wasd.y += 1.0f;
    if (Engine::Input::IsKeyDown(Engine::KeyCode::S) || Engine::Input::IsDown(Engine::Key::Down)) wasd.y -= 1.0f;
    if (Engine::Input::IsKeyDown(Engine::KeyCode::A) || Engine::Input::IsDown(Engine::Key::Left)) wasd.x -= 1.0f;
    if (Engine::Input::IsKeyDown(Engine::KeyCode::D) || Engine::Input::IsDown(Engine::Key::Right)) wasd.x += 1.0f;

    f32 altInput = 0.0f;
    if (Engine::Input::IsKeyDown(Engine::KeyCode::Space) || Engine::Input::IsKeyDown(Engine::KeyCode::E)) altInput += 1.0f;
    if (Engine::Input::IsKeyDown(Engine::KeyCode::Q) || Engine::Input::IsKeyDown(Engine::KeyCode::Ctrl)) altInput -= 1.0f;

    if (Engine::Input::WasKeyPressed(Engine::KeyCode::Space) && mMecha.Mode() == MechaMode::GroundWalk)
    {
        mMecha.ToggleFlight();
        DSPAudioManager::PlayClick();
    }

    if (Engine::Input::WasKeyPressed(Engine::KeyCode::G))
    {
        mFlatView = !mFlatView;
        if (mFlatView)
        {
            RebuildFlatViewMesh();
            Engine::UI::Flash("平面地形调试视图: 山水高度场 (再按 G 返回星球)", 2.5f);
        }
        else
        {
            mBuiltPlanet = nullptr;   // 球形网格被平面网格覆盖, 返回时重建
            EnsureTerrainMeshes(mUniverse.CurrentPlanet());
            Engine::UI::Flash("球形视图", 1.5f);
        }
    }

    mMecha.Move3D(wasd, cam.Right(), altInput, dt, planet);

    Vec2 mpos = Engine::Input::MousePos();
    Vec3 rayO, rayD;
    cam.ScreenToRay(mpos, (f32)mRenderer->Width(), (f32)mRenderer->Height(), rayO, rayD);
    UpdateCursor(rayO, rayD);

    if (Engine::Input::WasKeyPressed(Engine::KeyCode::R)) mBuildDir = (mBuildDir + 1) & 3;

    bool uiBlocked = mUI.IsScreenBlocked(mpos);
    BuildingKind tool = mUI.GetActiveModal() == ActiveModal::None ? mUI.SelectedBuildTool() : BuildingKind::None;
    i32 bpSel = mUI.SelectedBlueprintIndex();

    if (mCursorOnPlanet && (Engine::Input::WasKeyPressed(Engine::KeyCode::X) ||
                            Engine::Input::WasMousePressed(Engine::MouseButton::Middle)))
    {
        if (TryDismantle(mCursorTile)) DSPAudioManager::PlayDismantle();
    }

    bool lPressed = Engine::Input::WasMousePressed(Engine::MouseButton::Left);
    bool lDown = Engine::Input::IsMouseDown(Engine::MouseButton::Left);
    bool lReleased = Engine::Input::WasMouseReleased(Engine::MouseButton::Left);

    if (lReleased) mBeltDragging = false;

    if (!uiBlocked && mCursorOnPlanet && lPressed && mClickCooldown <= 0.0f)
    {
        if (bpSel >= 0 && bpSel < (i32)mBlueprints.Presets().Count())
        {
            const Blueprint& bp = mBlueprints.Presets()[bpSel];
            u32 placed = mBlueprints.PasteBlueprint(bp, mCursorTile, planet, mFactory, mMecha);
            if (placed > 0)
            {
                DSPAudioManager::PlayBuild();
                char msg[96];
                snprintf(msg, sizeof(msg), "蓝图铺设完成: %s (共 %u 座建筑)", bp.Name.CStr(), placed);
                Engine::UI::Flash(msg, 2.0f);
                mClickCooldown = 0.35f;
            }
        }
        else if (tool != BuildingKind::None)
        {
            TryPlace(mCursorTile, false);
        }
    }
    else if (!uiBlocked && mCursorOnPlanet && lDown && tool == BuildingKind::ConveyorBelt &&
             mBeltDragging && mCursorTile != mBeltLastTile && mClickCooldown <= 0.0f)
    {
        TryPlace(mCursorTile, true);
    }

    if (Engine::Input::WasKeyPressed(Engine::KeyCode::Escape) && mUI.GetActiveModal() == ActiveModal::None)
    {
        mUI.ClearBuildTool();
        mUI.ClearBlueprint();
        mBeltDragging = false;
    }
}

void Game::UpdateCursor(const Vec3& rayO, const Vec3& rayD)
{
    Planet* planet = mUniverse.CurrentPlanet();
    mCursorOnPlanet = false;
    if (!planet) return;
    if (PlanetGrid::Raycast(planet->Radius(), rayO, rayD, mCursorWorldPos, mCursorTile))
    {
        mCursorOnPlanet = true;
    }
}

void Game::TryPlace(u32 tileKey, bool dragChain)
{
    Planet* planet = mUniverse.CurrentPlanet();
    if (!planet) return;
    BuildingKind tool = mUI.SelectedBuildTool();
    if (tool == BuildingKind::None) return;

    i32 dir = mBuildDir;
    if (dragChain)
    {
        TileCoord a = PlanetGrid::Coord(mBeltLastTile);
        TileCoord b = PlanetGrid::Coord(tileKey);
        if (a.Face == b.Face)
        {
            if (b.U == a.U + 1) dir = 0;
            else if (b.U == a.U - 1) dir = 1;
            else if (b.V == a.V + 1) dir = 2;
            else if (b.V == a.V - 1) dir = 3;
        }
    }

    if (!planet->IsBuildableTile(tileKey)) return;
    const Tile& t = planet->Grid().GetTile(tileKey);
    if (tool == BuildingKind::MiningMachine && t.Resource == 0)
    {
        Engine::UI::Flash("采矿机必须建在矿脉上", 1.2f);
        return;
    }
    if (tool == BuildingKind::OilExtractor && t.Resource != (u8)ResourceKind::CrudeOil)
    {
        Engine::UI::Flash("抽油机必须建在原油矿脉上", 1.2f);
        return;
    }

    GridPos gp = GridPos::FromKey(tileKey);
    Vec3 pos = planet->Grid().TileCenter(planet->Radius(), tileKey);
    f32 rot = (f32)dir * Math::HALF_PI;
    u32 id = mFactory.PlaceBuilding(tool, gp, pos, rot, ItemKind::None);
    if (id == 0) return;

    planet->Grid().GetTile(tileKey).BuildingId = (u16)id;
    if (auto* b = mFactory.GetBuilding(id))
    {
        b->Dir = dir;
        if (tool == BuildingKind::Sorter)
        {
            b->SrcKey = PlanetGrid::NeighborKey(tileKey, (dir & 1) ? dir - 1 : dir + 1);
            b->DstKey = PlanetGrid::NeighborKey(tileKey, dir);
        }
    }
    mMecha.DispatchDrone(pos, id);
    DSPAudioManager::PlayBuild();
    mClickCooldown = 0.18f;

    if (tool == BuildingKind::ConveyorBelt)
    {
        mBeltDragging = true;
        mBeltLastTile = tileKey;
    }
}

bool Game::TryDismantle(u32 tileKey)
{
    Planet* planet = mUniverse.CurrentPlanet();
    if (!planet) return false;
    u16 bid = planet->Grid().GetTile(tileKey).BuildingId;
    if (bid == 0) return false;
    bool ok = mFactory.RemoveBuilding(bid, planet);
    if (ok) Engine::UI::Flash("已拆除建筑", 0.8f);
    return ok;
}

// ---------------------------------------------------------------------------
// 更新
// ---------------------------------------------------------------------------
void Game::Update()
{
    f32 dt = mTimer->Delta();
    if (dt <= 0.0f || dt > 0.1f) dt = 0.0166f;
    mElapsedTime += dt;

    Planet* planet = mUniverse.CurrentPlanet();
    Star* star = mUniverse.CurrentStar();

    HandleInput(dt);

    f32 sunA = mElapsedTime * 0.013f;
    f32 baseX = -2200.0f, baseZ = -2800.0f;
    mSunPos = {baseX * cosf(sunA) + baseZ * sinf(sunA), 900.0f,
               -baseX * sinf(sunA) + baseZ * cosf(sunA)};
    f32 slen = sqrtf(mSunPos.x * mSunPos.x + mSunPos.y * mSunPos.y + mSunPos.z * mSunPos.z);
    mSunDir = {mSunPos.x / slen, mSunPos.y / slen, mSunPos.z / slen};

    mUniverse.Update(dt);
    mMecha.Update(dt, planet, mUniverse.Camera().CurrentScale() >= ViewScale::StarSystem);

    f32 mechaEnergy = mMecha.EnergyMJ();
    f32 satisfaction = mPower.SatisfactionRatio();
    mFactory.Update(dt, planet, satisfaction);

    mPower.Update(dt, mFactory.Buildings(), mMecha.Position(), mechaEnergy,
                  mMecha.MaxEnergyMJ(), mSunDir, mElapsedTime);
    mMecha.SetEnergy(mechaEnergy);

    mTechTree.Update(dt, mFactory.Buildings());
    TechId newlyUnlocked;
    if (mTechTree.HasNewUnlockEvent(newlyUnlocked))
    {
        DSPAudioManager::PlayTechUnlock();
        if (const TechNode* t = mTechTree.GetTech(newlyUnlocked))
        {
            char unlockBuf[128];
            snprintf(unlockBuf, sizeof(unlockBuf), "科技研发完成: %s", t->Name.CStr());
            Engine::UI::Flash(unlockBuf, 3.0f);
        }
    }

    if (star)
    {
        star->DysonSphere.Update(dt, mFactory.LaunchedSolarSails(), mFactory.LaunchedCarrierRockets());
    }

    u32 sailsNow = mFactory.LaunchedSolarSails();
    u32 rocketsNow = mFactory.LaunchedCarrierRockets();
    if (sailsNow > mPrevSails)
    {
        for (const Building& b : mFactory.Buildings())
        {
            if (b.Kind == BuildingKind::EMRailEjector)
            {
                mLaunchFx.Add({b.WorldPos, mSunPos, 0.0f, 4.0f, false});
                break;
            }
        }
    }
    if (rocketsNow > mPrevRockets)
    {
        for (const Building& b : mFactory.Buildings())
        {
            if (b.Kind == BuildingKind::VerticalLaunchSilo)
            {
                mLaunchFx.Add({b.WorldPos, mSunPos, 0.0f, 6.0f, true});
                break;
            }
        }
    }
    mPrevSails = sailsNow;
    mPrevRockets = rocketsNow;

    UpdateLaunchFx(dt);

    Vec3 planetPos = {0.0f, 0.0f, 0.0f};
    mUniverse.Camera().Update(dt, mMecha.Position(), planetPos, mSunPos, mMecha.Altitude());

    DSPAudioManager::Update(dt, mUniverse.Camera(), (f32)mFactory.Buildings().Count());
}

void Game::UpdateLaunchFx(f32 dt)
{
    for (u32 i = 0; i < (u32)mLaunchFx.Count();)
    {
        mLaunchFx[i].T += dt / mLaunchFx[i].Dur;
        if (mLaunchFx[i].T >= 1.0f) mLaunchFx.RemoveAt(i);
        else i++;
    }
}

// ---------------------------------------------------------------------------
// 投影辅助 (2D 精灵层使用)
// ---------------------------------------------------------------------------
Game::Proj Game::Project(const Vec3& worldP, const Mat4& vp, f32 aspect) const
{
    Proj out;
    f32 x = worldP.x * vp.m[0] + worldP.y * vp.m[4] + worldP.z * vp.m[8] + vp.m[12];
    f32 y = worldP.x * vp.m[1] + worldP.y * vp.m[5] + worldP.z * vp.m[9] + vp.m[13];
    f32 w = worldP.x * vp.m[3] + worldP.y * vp.m[7] + worldP.z * vp.m[11] + vp.m[15];
    if (w <= 0.1f) return out;
    f32 ndcX = x / w, ndcY = y / w;
    if (ndcX < -3.0f || ndcX > 3.0f || ndcY < -3.0f || ndcY > 3.0f) return out;
    out.Pos.x = ndcX * 360.0f * aspect;
    out.Pos.y = -ndcY * 360.0f;
    out.Depth = w;
    out.Ok = true;
    return out;
}

static f32 ProjScale(f32 fovDeg)
{
    return 360.0f / tanf(fovDeg * 0.5f * Math::DEG_TO_RAD);
}

static f32 WorldToPx(const Game&, f32 worldSize, f32 depth, f32 fovDeg)
{
    return worldSize * ProjScale(fovDeg) / depth;
}

static Color PlanetGridVeinGlow(u8 resource)
{
    switch (resource)
    {
        case 1: return {0.4f, 0.6f, 0.9f, 1};
        case 2: return {0.95f, 0.55f, 0.25f, 1};
        case 3: return {0.5f, 0.5f, 0.55f, 1};
        case 4: return {0.8f, 0.8f, 0.75f, 1};
        case 5: return {0.7f, 0.85f, 1.0f, 1};
        case 6: return {0.4f, 0.9f, 0.95f, 1};
        default: return {0.7f, 0.3f, 0.9f, 1};
    }
}

static f32 ScreenAngleOf(const Game& game, const Vec3& pos, const Vec3& dir, const Mat4& vp, f32 aspect)
{
    auto p1 = game.Project(pos, vp, aspect);
    Vec3 p2w = {pos.x + dir.x * 2.0f, pos.y + dir.y * 2.0f, pos.z + dir.z * 2.0f};
    auto p2 = game.Project(p2w, vp, aspect);
    if (!p1.Ok || !p2.Ok) return 0.0f;
    return atan2f(p2.Pos.y - p1.Pos.y, p2.Pos.x - p1.Pos.x);
}

static void Line3D(Engine::SpriteBatch& batch, const Game& game,
                   const RefPtr<RHI::RHITexture>& solid,
                   const Vec3& a, const Vec3& b, const Mat4& vp, f32 aspect,
                   f32 widthPx, Color col, f32 layer, RHI::BlendMode blend = RHI::BlendMode::Alpha)
{
    auto pa = game.Project(a, vp, aspect);
    auto pb = game.Project(b, vp, aspect);
    if (!pa.Ok || !pb.Ok) return;
    f32 dx = pb.Pos.x - pa.Pos.x, dy = pb.Pos.y - pa.Pos.y;
    f32 len = sqrtf(dx * dx + dy * dy);
    if (len < 0.5f) return;
    f32 ang = atan2f(dy, dx);
    batch.Add(solid, {(pa.Pos.x + pb.Pos.x) * 0.5f, (pa.Pos.y + pb.Pos.y) * 0.5f},
              {len, widthPx}, col, ang, layer, blend);
}

// ---------------------------------------------------------------------------
// 渲染主流程
//   Pass A (2D): 深空背景 + 恒星
//   3D pass:   地形 / 水面 / 大气 / 戴森球 (真实深度缓冲, 硬件遮挡)
//   Pass B (2D): 天空穹顶 + 工厂/机甲/矿脉/UI
// ---------------------------------------------------------------------------
void Game::Render()
{
    if (!mRenderer || !mRenderer->BeginFrame()) return;

    auto* enc = mRenderer->Encoder();
    auto& batch = mRenderer->Sprites();
    batch.Clear();

    f32 aspect = mRenderer->Aspect();
    Mat4 vp3d = mUniverse.Camera().ViewProjection(aspect);

    mRenderer->GetCamera().Position = {0.0f, 0.0f};
    mRenderer->GetCamera().Zoom = 1.0f;
    Mat4 vp2d = mRenderer->GetCamera().ViewProjection(aspect);

    Planet* planet = mUniverse.CurrentPlanet();
    Star* star = mUniverse.CurrentStar();

    if (!mStarsInit)
    {
        mStarsInit = true;
        for (int i = 0; i < 320; i++)
        {
            BgStar s;
            s.Theta = (f32)i * 2.39996f;                       // 黄金角均匀分布
            s.Phi = sinf((f32)i * 0.777f) * 1.35f;
            s.Size = 1.2f + (f32)(i % 5) * 0.55f;
            s.Phase = (f32)(i % 17) * 0.9f;
            s.Warm = (u8)(i % 7);
            mBgStars.Add(s);
        }
    }

    // --- Pass A: 深空背景 + 恒星 (随后被 3D 地形正确遮挡) ---
    RenderSpace(batch, vp3d, aspect);
    if (!mFlatView) RenderSun(batch, vp3d, aspect);
    batch.Render(enc, vp2d);
    batch.Clear();

    // --- 3D pass: 地形/水面/大气/戴森球 ---
    if (mWorld3D.IsReady())
    {
        if (planet) EnsureTerrainMeshes(planet);
        mWorld3D.SetGlobals(vp3d, mUniverse.Camera().Eye(), mSunDir, mElapsedTime);
        mWorld3D.DrawTerrain(enc);
        mWorld3D.DrawWater(enc);
        if (!mFlatView)
        {
            if (star) RenderDysonSphere3D(vp3d, aspect);
            mWorld3D.DrawAtmosphere(enc);
        }
    }

    // --- Pass B: 游戏层精灵 ---
    if (!mFlatView) RenderSkyDome(batch, planet);
    if (planet)
    {
        RenderVeins(batch, vp3d, aspect, planet);
        RenderFactory(batch, vp3d, aspect, planet);
        RenderCursorGhost(batch, vp3d, aspect, planet);
        RenderMecha(batch, vp3d, aspect, planet);
    }
    RenderLaunchFx(batch, vp3d, aspect);
    RenderLensFlare(batch, aspect);

    // UI
    static DysonSphereManager sFallbackDyson;
    DysonSphereManager& dyson = star ? star->DysonSphere : sFallbackDyson;
    mUI.UpdateAndRender(mRenderer, mMecha, planet, mFactory, mPower, dyson,
                        mTechTree, mBlueprints, mUniverse, mSunDir, mElapsedTime);

    batch.Render(enc, vp2d);
    mRenderer->EndFrame();
}

// ---------------------------------------------------------------------------
// 1) 深空背景: 星云 / 星空 / 银河
// ---------------------------------------------------------------------------
void Game::RenderSpace(Engine::SpriteBatch& batch, const Mat4& vp, f32 aspect)
{
    (void)vp;
    f32 halfW = 360.0f * aspect;
    auto neb = DSPArt::TexNebula();
    auto solid = mSolidTex;

    // 深空基底
    if (solid)
    {
        batch.Add(solid, {0, 0}, {halfW * 2.4f, 780.0f}, Color{0.012f, 0.015f, 0.032f, 1.0f}, 0.0f, -200000.0f);
        batch.Add(solid, {0, -halfW}, {halfW * 2.4f, 900.0f}, Color{0.008f, 0.010f, 0.024f, 1.0f}, 0.0f, -200000.0f);
    }

    // 视差星云 (随镜头缓慢偏移)
    const DSPCamera& cam = mUniverse.Camera();
    Vec3 fwd = cam.Forward();
    f32 yaw = atan2f(fwd.x, fwd.z);
    f32 pitch = asinf(Clamp01(fwd.y));
    Vec2 par = {-yaw * 120.0f, pitch * 90.0f};

    if (neb)
    {
        batch.Add(neb, {-260.0f + par.x, -130.0f + par.y}, {820.0f, 520.0f},
                  Color{0.55f, 0.35f, 0.95f, 0.55f}, 0.35f + mElapsedTime * 0.004f, -198500.0f, RHI::BlendMode::Additive);
        batch.Add(neb, {300.0f + par.x * 1.3f, 170.0f + par.y * 1.2f}, {700.0f, 460.0f},
                  Color{0.20f, 0.55f, 0.95f, 0.45f}, -0.5f + mElapsedTime * 0.003f, -198400.0f, RHI::BlendMode::Additive);
        batch.Add(neb, {-80.0f + par.x * 0.7f, 60.0f + par.y * 0.8f}, {980.0f, 620.0f},
                  Color{0.85f, 0.45f, 0.55f, 0.25f}, 1.1f + mElapsedTime * 0.002f, -198300.0f, RHI::BlendMode::Additive);
    }

    // 星空
    auto star4 = DSPArt::TexStar4();
    for (const BgStar& s : mBgStars)
    {
        f32 d = 18000.0f;
        Vec3 sp = {d * cosf(s.Phi) * sinf(s.Theta), d * sinf(s.Phi), d * cosf(s.Phi) * cosf(s.Theta)};
        auto pr = Project(sp, vp, aspect);
        if (!pr.Ok) continue;
        f32 tw = 0.45f + 0.5f * sinf(mElapsedTime * 2.2f + s.Phase);
        Color c = s.Warm == 0 ? Color{0.75f, 0.85f, 1.0f, tw} :
                  s.Warm == 1 ? Color{1.0f, 0.85f, 0.6f, tw} : Color{0.95f, 0.97f, 1.0f, tw};
        batch.Add(mGlowTex, pr.Pos, {s.Size * 2.4f, s.Size * 2.4f}, c, 0.0f, -198000.0f, RHI::BlendMode::Additive);
        if (s.Size > 3.2f && star4)
        {
            batch.Add(star4, pr.Pos, {s.Size * 5.0f, s.Size * 5.0f}, Color{c.r, c.g, c.b, tw * 0.8f}, 0.0f, -197990.0f, RHI::BlendMode::Additive);
        }
    }
}

// ---------------------------------------------------------------------------
// 2) 天空穹顶 (大气内)
// ---------------------------------------------------------------------------
void Game::RenderSkyDome(Engine::SpriteBatch& batch, const Planet* planet)
{
    if (!planet || !mSolidTex) return;
    f32 aspect = mRenderer->Aspect();
    f32 halfW = 360.0f * aspect;

    f32 op = planet->AtmosphericOpacity(mMecha.Altitude());
    if (op < 0.01f) return;

    Vec3 up = {0.0f, 1.0f, 0.0f};
    f32 plen = sqrtf(mMecha.Position().x * mMecha.Position().x + mMecha.Position().y * mMecha.Position().y + mMecha.Position().z * mMecha.Position().z);
    if (plen > 1.0f)
    {
        up = {mMecha.Position().x / plen, mMecha.Position().y / plen, mMecha.Position().z / plen};
    }
    Color sky = planet->SkyColor(mMecha.Altitude(), mSunDir, up);
    sky.a *= 0.85f;
    if (sky.a > 0.02f)
    {
        batch.Add(mSolidTex, {0.0f, 0.0f}, {halfW * 2.4f, 780.0f}, sky, 0.0f, -197000.0f);
    }

    // 地平线雾光
    if (mMecha.Altitude() < 60.0f)
    {
        Color horizon = planet->HorizonFogColor(mMecha.Altitude(), mSunDir);
        horizon.a = horizon.a * Clamp01(1.0f - mMecha.Altitude() / 60.0f);
        if (horizon.a > 0.02f)
        {
            batch.Add(mGlowTex, {0.0f, 200.0f}, {halfW * 2.2f, 380.0f}, horizon, 0.0f, -196900.0f, RHI::BlendMode::Additive);
        }
    }
}

// ---------------------------------------------------------------------------
// 3) 恒星
// ---------------------------------------------------------------------------
void Game::RenderSun(Engine::SpriteBatch& batch, const Mat4& vp, f32 aspect)
{
    Star* s = mUniverse.CurrentStar();
    if (!s || !mGlowTex) return;
    auto pr = Project(mSunPos, vp, aspect);
    if (!pr.Ok) return;

    Color sc = StarColor(s->Type);
    f32 pulse = 1.0f + 0.05f * sinf(mElapsedTime * 1.7f);
    f32 core = WorldToPx(*this, s->Radius * 2.0f, pr.Depth, mUniverse.Camera().FovDeg());
    core = Math::Clamp(core, 10.0f, 900.0f);

    auto flare = DSPArt::TexFlare();
    auto star4 = DSPArt::TexStar4();

    batch.Add(mGlowTex, pr.Pos, {core * 3.4f * pulse, core * 3.4f * pulse}, MulColor(sc, 0.35f), 0.0f, -196800.0f, RHI::BlendMode::Additive);
    if (star4) batch.Add(star4, pr.Pos, {core * 2.6f, core * 2.6f}, Color{sc.r, sc.g, sc.b, 0.9f}, 0.0f, -196790.0f, RHI::BlendMode::Additive);
    batch.Add(mGlowTex, pr.Pos, {core * 1.1f, core * 1.1f}, Color{1.0f, 0.98f, 0.9f, 1.0f}, 0.0f, -196780.0f, RHI::BlendMode::Additive);
    if (flare)
    {
        batch.Add(flare, pr.Pos, {core * 6.5f, core * 0.55f}, Color{sc.r, sc.g, sc.b, 0.55f}, 0.0f, -196770.0f, RHI::BlendMode::Additive);
        batch.Add(flare, pr.Pos, {core * 0.55f, core * 6.5f}, Color{sc.r, sc.g, sc.b, 0.30f}, Math::HALF_PI, -196760.0f, RHI::BlendMode::Additive);
    }
    mSunScreen = pr.Pos;
    mSunOnScreen = true;
}

// ---------------------------------------------------------------------------
// 4) 戴森球 (真 3D: 帆板三角网 + 骨架线 + 节点/太阳帆公告板,
//    全部走深度测试, 被星球正确遮挡)
// ---------------------------------------------------------------------------
static void PushSphereTri(Array<World3D::Vertex>& out, Vec3 a, Vec3 b, Vec3 c,
                          Color col, int depth)
{
    auto norm = [](const Vec3& v) {
        f32 l = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
        return Vec3{v.x / l, v.y / l, v.z / l};
    };
    if (depth <= 0)
    {
        out.Add({a, a, col});
        out.Add({b, b, col});
        out.Add({c, c, col});
        return;
    }
    Vec3 ab = norm({a.x + b.x, a.y + b.y, a.z + b.z});
    Vec3 bc = norm({b.x + c.x, b.y + c.y, b.z + c.z});
    Vec3 ca = norm({c.x + a.x, c.y + a.y, c.z + a.z});
    PushSphereTri(out, a, ab, ca, col, depth - 1);
    PushSphereTri(out, ab, b, bc, col, depth - 1);
    PushSphereTri(out, ca, bc, c, col, depth - 1);
    PushSphereTri(out, ab, bc, ca, col, depth - 1);
}

static void PushBillboard(Array<World3D::Vertex>& out, const Vec3& p,
                          const Vec3& right, const Vec3& up, f32 size, Color col)
{
    Vec3 ru = {right.x * size, right.y * size, right.z * size};
    Vec3 uu = {up.x * size, up.y * size, up.z * size};
    Vec3 p0 = {p.x - ru.x - uu.x, p.y - ru.y - uu.y, p.z - ru.z - uu.z};
    Vec3 p1 = {p.x + ru.x - uu.x, p.y + ru.y - uu.y, p.z + ru.z - uu.z};
    Vec3 p2 = {p.x + ru.x + uu.x, p.y + ru.y + uu.y, p.z + ru.z + uu.z};
    Vec3 p3 = {p.x - ru.x + uu.x, p.y - ru.y + uu.y, p.z - ru.z + uu.z};
    Vec3 dummy{0.0f, 1.0f, 0.0f};
    out.Add({p0, dummy, col});
    out.Add({p1, dummy, col});
    out.Add({p2, dummy, col});
    out.Add({p0, dummy, col});
    out.Add({p2, dummy, col});
    out.Add({p3, dummy, col});
}

void Game::RenderDysonSphere3D(const Mat4& vp, f32 aspect)
{
    (void)vp;
    (void)aspect;
    Star* star = mUniverse.CurrentStar();
    if (!star || !mWorld3D.IsReady()) return;
    const DysonSphereManager& dyson = star->DysonSphere;

    static Array<World3D::Vertex> lines, tris;
    static bool sReserved = false;
    if (!sReserved)
    {
        lines.Reserve(8192);
        tris.Reserve(131072);
        sReserved = true;
    }
    lines.Clear();
    tris.Clear();

    // 帆板: 完成度越高越亮
    for (const DysonShellPanel& p : dyson.Panels())
    {
        if (p.FillRatio <= 0.0f) continue;
        if (p.NodeIndices[0] >= (u32)dyson.Nodes().Count() ||
            p.NodeIndices[1] >= (u32)dyson.Nodes().Count() ||
            p.NodeIndices[2] >= (u32)dyson.Nodes().Count()) continue;
        const Vec3& a = dyson.Nodes()[p.NodeIndices[0]].Pos;
        const Vec3& b = dyson.Nodes()[p.NodeIndices[1]].Pos;
        const Vec3& c = dyson.Nodes()[p.NodeIndices[2]].Pos;
        Color col = p.Completed ? Color{0.30f, 0.70f, 1.0f, 0.28f}
                                : Color{0.55f, 0.75f, 0.95f, 0.05f + 0.20f * p.FillRatio};
        static Array<World3D::Vertex> panel;
        panel.Clear();
        PushSphereTri(panel, a, b, c, col, 1);
        for (const World3D::Vertex& v : panel)
        {
            tris.Add({{v.Position.x + mSunPos.x, v.Position.y + mSunPos.y, v.Position.z + mSunPos.z},
                      v.Normal, v.Color});
        }
    }

    // 骨架连杆
    for (const DysonStrut& st : dyson.Struts())
    {
        if (st.NodeA >= (u32)dyson.Nodes().Count() || st.NodeB >= (u32)dyson.Nodes().Count()) continue;
        const DysonNode& na = dyson.Nodes()[st.NodeA];
        const DysonNode& nb = dyson.Nodes()[st.NodeB];
        Color col = st.Completed ? Color{0.25f, 0.85f, 1.0f, 0.65f} : Color{0.45f, 0.55f, 0.65f, 0.30f};
        lines.Add({{mSunPos.x + na.Pos.x, mSunPos.y + na.Pos.y, mSunPos.z + na.Pos.z},
                   {0.0f, 1.0f, 0.0f}, col});
        lines.Add({{mSunPos.x + nb.Pos.x, mSunPos.y + nb.Pos.y, mSunPos.z + nb.Pos.z},
                   {0.0f, 1.0f, 0.0f}, col});
    }

    // 节点公告板
    const DSPCamera& cam = mUniverse.Camera();
    for (const DysonNode& n : dyson.Nodes())
    {
        Vec3 p = {mSunPos.x + n.Pos.x, mSunPos.y + n.Pos.y, mSunPos.z + n.Pos.z};
        Color nc = n.Completed ? Color{0.3f, 0.95f, 1.0f, 0.9f} : Color{0.55f, 0.62f, 0.72f, 0.5f};
        PushBillboard(tris, p, cam.Right(), cam.Up(), 16.0f, nc);
    }

    // 太阳帆蜂群
    for (const SolarSailParticle& s : dyson.Sails())
    {
        Vec3 p = {mSunPos.x + s.Pos.x, mSunPos.y + s.Pos.y, mSunPos.z + s.Pos.z};
        PushBillboard(tris, p, cam.Right(), cam.Up(), 9.0f, Color{1.0f, 0.85f, 0.45f, 0.85f});
    }

    mWorld3D.UploadLines(lines.Data(), (u32)lines.Count());
    mWorld3D.UploadTris(tris.Data(), (u32)tris.Count());
    mWorld3D.DrawLines(mRenderer->Encoder());
    mWorld3D.DrawTris(mRenderer->Encoder());
}

// ---------------------------------------------------------------------------
// 5) 矿脉晶体
// ---------------------------------------------------------------------------
void Game::RenderVeins(Engine::SpriteBatch& batch, const Mat4& vp, f32 aspect, const Planet* planet)
{
    const PlanetGrid& grid = planet->Grid();
    f32 R = planet->Radius();
    f32 fov = mUniverse.Camera().FovDeg();
    Vec3 eye = mUniverse.Camera().Eye();
    f32 eyeLen = sqrtf(eye.x * eye.x + eye.y * eye.y + eye.z * eye.z);
    Vec3 eyeDir = eyeLen > 1.0f ? Vec3{eye.x / eyeLen, eye.y / eyeLen, eye.z / eyeLen} : Vec3{0.0f, 1.0f, 0.0f};
    auto veinTex = DSPArt::TexVeins();
    if (!veinTex) return;
    f32 horizonCos = Math::Clamp(R / Math::Max(eyeLen, 1.0f), 0.0f, 1.0f);

    f32 camDist = mUniverse.Camera().ZoomDistance();
    u32 stride = camDist > 900.0f ? 3 : 1; // 远景抽稀

    constexpr u32 VN = PlanetGrid::TILES_PER_FACE;
    for (u32 face = 0; face < PlanetGrid::FACE_COUNT; face++)
    {
        for (u32 u = 0; u < VN; u++)
        {
            for (u32 v = 0; v < VN; v++)
            {
                u32 key = PlanetGrid::Key((u8)face, (u8)u, (u8)v);
                const Tile& t = grid.GetTile(key);
                if (t.Resource == 0) continue;
                if (stride > 1 && ((key * 2654435761u) % stride) != 0) continue;
                Vec3 n = PlanetGrid::TileNormal(key);
                if (n.x * eyeDir.x + n.y * eyeDir.y + n.z * eyeDir.z < horizonCos - 0.005f) continue;

                Vec3 pos = grid.TileCenter(R, key) + n * 1.2f;
                auto pr = Project(pos, vp, aspect);
                if (!pr.Ok) continue;
                f32 worldSz = 9.0f + (f32)t.Richness * 2.5f;
                f32 px = Math::Clamp(WorldToPx(*this, worldSz, pr.Depth, fov), 3.0f, 64.0f);

                Vec2 uv0, uv1;
                DSPArt::CellUv(8, 1, (int)t.Resource - 1, uv0, uv1);
                f32 rot = (f32)(t.Tint % 8) * Math::PI * 0.25f;
                f32 deplete = t.ResourceAmount > 0 ? Clamp01((f32)t.ResourceAmount / 600000.0f) : 0.0f;
                f32 sz = px * (0.55f + 0.45f * deplete);
                Color light = MulColor(Color{1, 1, 1, 1}, 0.45f + 0.6f * Math::Max(0.0f, n.x * mSunDir.x + n.y * mSunDir.y + n.z * mSunDir.z));
                light.a = 1.0f;
                batch.AddUv(veinTex, uv0, uv1, pr.Pos, {sz, sz}, light, rot, -60.0f);
                if (mGlowTex)
                {
                    Color gc = PlanetGridVeinGlow(t.Resource);
                    batch.Add(mGlowTex, pr.Pos, {sz * 1.6f, sz * 1.6f}, Color{gc.r, gc.g, gc.b, 0.30f * deplete}, 0.0f, -59.0f, RHI::BlendMode::Additive);
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// 6) 光标与建造预览
// ---------------------------------------------------------------------------
void Game::RenderCursorGhost(Engine::SpriteBatch& batch, const Mat4& vp, f32 aspect, const Planet* planet)
{
    if (!mCursorOnPlanet || !mSolidTex) return;
    const PlanetGrid& grid = planet->Grid();
    f32 R = planet->Radius();
    BuildingKind tool = mUI.SelectedBuildTool();
    i32 bpSel = mUI.SelectedBlueprintIndex();

    Vec3 n = PlanetGrid::TileNormal(mCursorTile);
    Vec3 pos = grid.TileCenter(R, mCursorTile);

    // 瓦片高亮框
    Vec3 e, nt;
    PlanetGrid::TileBasis(mCursorTile, e, nt);
    f32 half = R * PlanetGrid::STEP * 0.5f * (Math::PI * 0.5f) * 0.92f;
    Vec3 c[4] =
    {
        pos + e * half + nt * half,
        pos - e * half + nt * half,
        pos - e * half - nt * half,
        pos + e * half - nt * half
    };
    bool occupied = grid.GetTile(mCursorTile).BuildingId != 0;
    Color ring = occupied ? Color{1.0f, 0.45f, 0.3f, 0.85f} : Color{0.25f, 0.95f, 1.0f, 0.85f};
    for (int i = 0; i < 4; i++)
    {
        Vec3 lift = n * 0.6f;
        Line3D(batch, *this, mSolidTex, c[i] + lift, c[(i + 1) & 3] + lift, vp, aspect, 2.0f, ring, -55.0f);
    }

    // 建筑幽灵
    if (tool != BuildingKind::None && (int)tool < (int)BuildingKind::Count)
    {
        bool placeable = planet->IsBuildableTile(mCursorTile);
        const Tile& t = grid.GetTile(mCursorTile);
        if (tool == BuildingKind::MiningMachine && t.Resource == 0) placeable = false;
        if (tool == BuildingKind::OilExtractor && t.Resource != (u8)ResourceKind::CrudeOil) placeable = false;

        auto btex = DSPArt::TexBuildings();
        if (btex)
        {
            int idx = (int)tool - 1;
            Vec2 uv0, uv1;
            DSPArt::CellUv(6, 6, idx, uv0, uv1);
            Vec3 facing;
            {
                Vec3 nb = grid.TileCenter(R, PlanetGrid::NeighborKey(mCursorTile, mBuildDir));
                facing = {nb.x - pos.x, nb.y - pos.y, nb.z - pos.z};
                f32 fl = sqrtf(facing.x * facing.x + facing.y * facing.y + facing.z * facing.z);
                if (fl > 1e-4f) { facing.x /= fl; facing.y /= fl; facing.z /= fl; }
            }
            f32 ang = ScreenAngleOf(*this, pos, facing, vp, aspect);
            auto pr = Project(pos + n * 2.0f, vp, aspect);
            if (pr.Ok)
            {
                f32 sz = BuildingWorldSize(tool) * ProjScale(mUniverse.Camera().FovDeg()) / pr.Depth;
                sz = Math::Clamp(sz, 6.0f, 240.0f);
                Color gc = placeable ? Color{0.45f, 1.0f, 0.6f, 0.60f} : Color{1.0f, 0.35f, 0.3f, 0.60f};
                batch.AddUv(btex, uv0, uv1, pr.Pos, {sz, sz}, gc, ang, -50.0f);
            }
        }

        // 电力塔供电范围
        if (tool == BuildingKind::TeslaTower || tool == BuildingKind::WirelessPowerTower)
        {
            f32 radius = (tool == BuildingKind::TeslaTower) ? 12.0f : 22.0f;
            Vec3 e2, n2;
            PlanetGrid::TileBasis(mCursorTile, e2, n2);
            Vec3 prev = pos + e2 * radius + n * 1.0f;
            for (int i = 1; i <= 26; i++)
            {
                f32 a = (f32)i / 26.0f * Math::TWO_PI;
                Vec3 p = pos + (e2 * cosf(a) + n2 * sinf(a)) * radius + n * 1.0f;
                Line3D(batch, *this, mSolidTex, prev, p, vp, aspect, 1.4f, Color{0.3f, 0.9f, 1.0f, 0.5f}, -54.0f, RHI::BlendMode::Additive);
                prev = p;
            }
        }
    }

    // 蓝图范围预览
    if (bpSel >= 0 && bpSel < (i32)mBlueprints.Presets().Count())
    {
        const Blueprint& bp = mBlueprints.Presets()[bpSel];
        for (const BlueprintItem& item : bp.Items)
        {
            TileCoord oc = PlanetGrid::Coord(mCursorTile);
            constexpr i32 NN = (i32)PlanetGrid::TILES_PER_FACE;
            i32 uu = Math::Clamp(oc.U + item.Dx, 0, NN - 1);
            i32 vv = Math::Clamp(oc.V + item.Dy, 0, NN - 1);
            u32 key = PlanetGrid::Key(oc.Face, (u8)uu, (u8)vv);
            Vec3 p = grid.TileCenter(R, key);
            auto ppr = Project(p, vp, aspect);
            if (!ppr.Ok) continue;
            Color bc = planet->IsBuildableTile(key) ? Color{0.3f, 0.9f, 1.0f, 0.5f} : Color{1.0f, 0.4f, 0.3f, 0.5f};
            batch.Add(mGlowTex, ppr.Pos, {6.0f, 6.0f}, bc, 0.0f, -53.0f, RHI::BlendMode::Additive);
        }
    }

    // 拆除高亮
    if (grid.GetTile(mCursorTile).BuildingId != 0)
    {
        Vec3 lift = pos + n * 3.0f;
        auto lpr = Project(lift, vp, aspect);
        if (lpr.Ok)
        {
            batch.Add(mGlowTex, lpr.Pos, {18.0f, 18.0f}, Color{1.0f, 0.3f, 0.25f, 0.5f}, mElapsedTime * 2.0f, -52.0f, RHI::BlendMode::Additive);
        }
    }
}

// ---------------------------------------------------------------------------
// 7) 工厂建筑 / 传送带 / 物流
// ---------------------------------------------------------------------------
void Game::RenderFactory(Engine::SpriteBatch& batch, const Mat4& vp, f32 aspect, const Planet* planet)
{
    const PlanetGrid& grid = planet->Grid();
    f32 R = planet->Radius();
    f32 fov = mUniverse.Camera().FovDeg();
    f32 pscale = ProjScale(fov);
    Vec3 eye = mUniverse.Camera().Eye();
    f32 eyeLen = sqrtf(eye.x * eye.x + eye.y * eye.y + eye.z * eye.z);
    Vec3 eyeDir = eyeLen > 1.0f ? Vec3{eye.x / eyeLen, eye.y / eyeLen, eye.z / eyeLen} : Vec3{0.0f, 1.0f, 0.0f};
    auto btex = DSPArt::TexBuildings();
    auto items = DSPArt::TexItems();
    auto fx = DSPArt::TexFx();
    if (!btex) return;
    bool showBars = mUniverse.Camera().ZoomDistance() < 420.0f;
    f32 horizonCos = Math::Clamp(R / Math::Max(eyeLen, 1.0f), 0.0f, 1.0f);

    const Array<Building>& list = mFactory.Buildings();

    // 电力弧线
    for (u32 i = 0; i < (u32)list.Count(); i++)
    {
        if (list[i].Kind != BuildingKind::TeslaTower && list[i].Kind != BuildingKind::WirelessPowerTower) continue;
        for (u32 j = i + 1; j < (u32)list.Count(); j++)
        {
            if (list[j].Kind != BuildingKind::TeslaTower && list[j].Kind != BuildingKind::WirelessPowerTower) continue;
            Vec3 d = {list[j].WorldPos.x - list[i].WorldPos.x, list[j].WorldPos.y - list[i].WorldPos.y, list[j].WorldPos.z - list[i].WorldPos.z};
            f32 dl = sqrtf(d.x * d.x + d.y * d.y + d.z * d.z);
            if (dl > 55.0f) continue;
            Vec3 a = list[i].WorldPos + PlanetGrid::TileNormal(list[i].TileKey) * 6.0f;
            Vec3 b = list[j].WorldPos + PlanetGrid::TileNormal(list[j].TileKey) * 6.0f;
            Line3D(batch, *this, mSolidTex, a, b, vp, aspect, 1.5f, Color{0.25f, 0.9f, 1.0f, 0.5f}, -11.0f, RHI::BlendMode::Additive);
            // 能量脉冲
            f32 t = fmodf(mElapsedTime * 0.7f + (f32)i * 0.13f, 1.0f);
            Vec3 pulse = {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t};
            auto pp = Project(pulse, vp, aspect);
            if (pp.Ok) batch.Add(mGlowTex, pp.Pos, {5.0f, 5.0f}, Color{0.5f, 1.0f, 1.0f, 0.8f}, 0.0f, -10.0f, RHI::BlendMode::Additive);
        }
    }

    // 建筑
    for (const Building& b : list)
    {
        Vec3 n = PlanetGrid::TileNormal(b.TileKey);
        if (n.x * eyeDir.x + n.y * eyeDir.y + n.z * eyeDir.z < horizonCos - 0.02f) continue;
        auto pr = Project(b.WorldPos + n * 1.5f, vp, aspect);
        if (!pr.Ok) continue;

        f32 worldSz = BuildingWorldSize(b.Kind);
        f32 sz = worldSz * pscale / pr.Depth;
        if (sz < 1.5f) continue;
        sz = Math::Min(sz, 260.0f);

        // 地面阴影
        auto spr = Project(b.WorldPos + n * 0.2f, vp, aspect);
        if (spr.Ok)
        {
            batch.Add(mGlowTex, spr.Pos, {sz * 0.8f, sz * 0.8f}, Color{0.0f, 0.0f, 0.0f, 0.30f}, 0.0f, -40.0f);
        }

        // 朝向角
        Vec3 nb = grid.TileCenter(R, PlanetGrid::NeighborKey(b.TileKey, b.Dir));
        Vec3 facing = {nb.x - b.WorldPos.x, nb.y - b.WorldPos.y, nb.z - b.WorldPos.z};
        f32 fl = sqrtf(facing.x * facing.x + facing.y * facing.y + facing.z * facing.z);
        if (fl > 1e-4f) { facing.x /= fl; facing.y /= fl; facing.z /= fl; }
        f32 ang = ScreenAngleOf(*this, b.WorldPos, facing, vp, aspect);

        int idx = (int)b.Kind - 1;
        Vec2 uv0, uv1;
        DSPArt::CellUv(6, 6, idx, uv0, uv1);
        Color light = MulColor(Color{1, 1, 1, 1}, 0.55f + 0.5f * Math::Max(0.0f, n.x * mSunDir.x + n.y * mSunDir.y + n.z * mSunDir.z));
        light.a = 1.0f;
        batch.AddUv(btex, uv0, uv1, pr.Pos, {sz, sz}, light, ang, -25.0f);

        // 工作状态灯
        bool isMachine = b.Kind == BuildingKind::ArcSmelter || b.Kind == BuildingKind::AssemblingMachine ||
                         b.Kind == BuildingKind::ChemicalPlant || b.Kind == BuildingKind::MatrixLab ||
                         b.Kind == BuildingKind::MiningMachine;
        if (mGlowTex && isMachine)
        {
            bool working = b.Progress > 0.0f && b.Progress < 1.0f;
            Color lamp = working ? Color{0.25f, 1.0f, 0.5f, 0.6f + 0.3f * sinf(b.AnimTimer * 6.0f)} :
                         b.Powered ? Color{0.2f, 0.7f, 1.0f, 0.4f} : Color{1.0f, 0.25f, 0.2f, 0.6f};
            batch.Add(mGlowTex, pr.Pos, {sz * 0.5f, sz * 0.5f}, lamp, 0.0f, -24.0f, RHI::BlendMode::Additive);
        }
        // 熔炉火光
        if (mGlowTex && b.Kind == BuildingKind::ArcSmelter && b.Progress > 0.0f)
        {
            f32 fl2 = 0.5f + 0.5f * sinf(b.AnimTimer * 9.0f);
            batch.Add(mGlowTex, pr.Pos, {sz * 0.55f, sz * 0.55f}, Color{1.0f, 0.45f, 0.12f, 0.4f + 0.3f * fl2}, 0.0f, -23.0f, RHI::BlendMode::Additive);
        }

        // 传送带 + 货物
        if (b.Kind == BuildingKind::ConveyorBelt && items)
        {
            for (const BeltItem& it : b.BeltItems)
            {
                if (it.Kind == ItemKind::None) continue;
                f32 off = (it.Progress - 0.5f) * (R * PlanetGrid::STEP * 1.35f);
                Vec3 ip = b.WorldPos + facing * off + n * 0.9f;
                auto ipr = Project(ip, vp, aspect);
                if (!ipr.Ok) continue;
                Vec2 iuv0, iuv1;
                DSPArt::CellUv(8, 8, (int)it.Kind, iuv0, iuv1);
                f32 isz = Math::Clamp(4.2f * pscale / pr.Depth, 2.0f, 30.0f);
                batch.AddUv(items, iuv0, iuv1, ipr.Pos, {isz, isz}, Color{1, 1, 1, 1}, 0.0f, -15.0f);
            }
        }

        // 分拣器臂
        if (b.Kind == BuildingKind::Sorter)
        {
            Building* src = mFactory.BuildingAtTile(b.SrcKey);
            Building* dst = mFactory.BuildingAtTile(b.DstKey);
            if (src && dst)
            {
                f32 t = b.SorterArmProgress;
                Vec3 arm = {src->WorldPos.x + (dst->WorldPos.x - src->WorldPos.x) * t,
                            src->WorldPos.y + (dst->WorldPos.y - src->WorldPos.y) * t,
                            src->WorldPos.z + (dst->WorldPos.z - src->WorldPos.z) * t};
                Line3D(batch, *this, mSolidTex, b.WorldPos + n * 1.5f, arm + n * 1.5f, vp, aspect, 2.0f,
                       Color{0.9f, 0.8f, 0.4f, 0.85f}, -14.0f);
                if (b.SorterHeldItem.Kind != ItemKind::None && items)
                {
                    auto apr = Project(arm + n * 2.0f, vp, aspect);
                    if (apr.Ok)
                    {
                        Vec2 iuv0, iuv1;
                        DSPArt::CellUv(8, 8, (int)b.SorterHeldItem.Kind, iuv0, iuv1);
                        f32 isz = Math::Clamp(4.0f * pscale / apr.Depth, 2.0f, 26.0f);
                        batch.AddUv(items, iuv0, iuv1, apr.Pos, {isz, isz}, Color{1, 1, 1, 1}, 0.0f, -14.0f);
                    }
                }
            }
        }

        // 进度条
        if (showBars && isMachine && b.Progress > 0.01f)
        {
            f32 barW = Math::Clamp(sz * 0.75f, 14.0f, 60.0f);
            Vec2 barPos = {pr.Pos.x - barW * 0.5f, pr.Pos.y - sz * 0.62f};
            batch.Add(mSolidTex, {barPos.x + barW * 0.5f, barPos.y + 2.0f}, {barW + 2.0f, 5.0f}, Color{0.05f, 0.07f, 0.10f, 0.8f}, 0.0f, -12.0f);
            batch.Add(mSolidTex, {barPos.x + barW * 0.5f * b.Progress, barPos.y + 2.0f}, {barW * b.Progress, 3.0f},
                      Color{0.25f, 0.95f, 1.0f, 0.95f}, 0.0f, -12.0f);
        }
    }

    // 物流飞船
    for (const LogisticsShip& s : mFactory.ActiveShips())
    {
        auto pr = Project(s.CurrentPos, vp, aspect);
        if (!pr.Ok) continue;
        f32 px = Math::Clamp(8.0f * pscale / pr.Depth, 3.0f, 40.0f);
        if (fx)
        {
            batch.AddUv(fx, {0.75f, 0.0f}, {1.0f, 0.5f}, pr.Pos, {px, px * 0.6f}, Color{1, 1, 1, 1}, 0.0f, -3.0f);
        }
        batch.Add(mGlowTex, pr.Pos, {px * 1.8f, px * 1.8f}, Color{0.25f, 0.8f, 1.0f, 0.5f}, 0.0f, -4.0f, RHI::BlendMode::Additive);
    }
}

// ---------------------------------------------------------------------------
// 8) 机甲与无人机
// ---------------------------------------------------------------------------
void Game::RenderMecha(Engine::SpriteBatch& batch, const Mat4& vp, f32 aspect, const Planet* planet)
{
    (void)planet;
    f32 fov = mUniverse.Camera().FovDeg();
    f32 pscale = ProjScale(fov);
    auto mechaTex = DSPArt::TexMecha();
    auto fx = DSPArt::TexFx();
    if (!mechaTex) return;

    Vec3 mpos = mMecha.Position();
    auto pr = Project(mpos, vp, aspect);
    if (!pr.Ok) return;
    f32 sz = Math::Clamp(7.0f * pscale / pr.Depth, 8.0f, 180.0f);

    // 阴影
    Vec3 ground = mpos;
    {
        f32 gl = sqrtf(mpos.x * mpos.x + mpos.y * mpos.y + mpos.z * mpos.z);
        if (gl > 1.0f) ground = {mpos.x / gl * (gl - mMecha.Altitude()), mpos.y / gl * (gl - mMecha.Altitude()), mpos.z / gl * (gl - mMecha.Altitude())};
    }
    auto gpr = Project(ground, vp, aspect);
    if (gpr.Ok && mMecha.Altitude() > 0.5f)
    {
        f32 ssz = sz * Clamp01(1.0f - mMecha.Altitude() / 90.0f) * 0.8f + sz * 0.2f;
        batch.Add(mGlowTex, gpr.Pos, {ssz, ssz * 0.7f}, Color{0.0f, 0.0f, 0.0f, 0.35f}, 0.0f, -41.0f);
    }

    // 朝向: 精灵 "上方向" 对齐星球表面法线 —— 脚始终踩向地面,
    // 与移动方向/相机方位角无关 (之前对齐速度或相机朝向, 脚会朝移动方向乱转)
    Vec3 mup = mpos;
    {
        f32 mpl = sqrtf(mpos.x * mpos.x + mpos.y * mpos.y + mpos.z * mpos.z);
        if (mpl > 1.0f) mup = {mpos.x / mpl, mpos.y / mpl, mpos.z / mpl};
        else mup = {0.0f, 1.0f, 0.0f};
    }
    f32 ang = mSpriteRot;   // 退化角度 (相机正对头顶) 时沿用上次朝向, 避免抖动
    {
        auto p1 = Project(mpos, vp, aspect);
        Vec3 head = {mpos.x + mup.x * 3.0f, mpos.y + mup.y * 3.0f, mpos.z + mup.z * 3.0f};
        auto p2 = Project(head, vp, aspect);
        if (p1.Ok && p2.Ok)
        {
            f32 dx = p2.Pos.x - p1.Pos.x, dy = p2.Pos.y - p1.Pos.y;
            if (dx * dx + dy * dy > 4.0f) ang = atan2f(dy, dx) + Math::HALF_PI;
        }
    }
    mSpriteRot = ang;

    Color mechaCol = mMecha.IsLowEnergy() ? Color{1.0f, 0.5f, 0.45f, 1.0f} : Color{1, 1, 1, 1};
    batch.Add(mechaTex, pr.Pos, {sz * 0.75f, sz}, mechaCol, ang, -8.0f);

    // 核心辉光
    if (mGlowTex)
    {
        Color core = mMecha.IsLowEnergy() ? Color{1.0f, 0.25f, 0.2f, 0.7f} : Color{0.25f, 0.9f, 1.0f, 0.55f + 0.2f * sinf(mElapsedTime * 3.0f)};
        batch.Add(mGlowTex, pr.Pos, {sz * 0.5f, sz * 0.5f}, core, 0.0f, -7.0f, RHI::BlendMode::Additive);
    }

    // 推进器尾焰
    f32 burn = mMecha.ThrusterIntensity();
    if (mGlowTex && burn > 0.02f && mMecha.Mode() != MechaMode::GroundWalk)
    {
        f32 flame = sz * (0.9f + burn * 1.3f);
        Vec2 off = {sinf(ang) * sz * 0.42f, cosf(ang) * sz * 0.42f};
        batch.Add(mGlowTex, {pr.Pos.x - off.x * 0.3f, pr.Pos.y - off.y * 0.3f}, {sz * 0.5f, flame},
                  Color{0.25f, 0.85f, 1.0f, 0.75f * burn}, ang + Math::PI, -9.0f, RHI::BlendMode::Additive);
        batch.Add(mGlowTex, {pr.Pos.x - off.x * 0.55f, pr.Pos.y - off.y * 0.55f}, {sz * 0.26f, flame * 0.55f},
                  Color{0.85f, 0.95f, 1.0f, 0.8f * burn}, ang + Math::PI, -9.0f, RHI::BlendMode::Additive);
    }

    // 再入等离子
    f32 reentry = mMecha.ReentryIntensity();
    if (mGlowTex && reentry > 0.02f)
    {
        batch.Add(mGlowTex, pr.Pos, {sz * 2.6f, sz * 2.6f}, Color{1.0f, 0.4f, 0.1f, 0.7f * reentry}, mElapsedTime * 7.0f, -6.0f, RHI::BlendMode::Additive);
        batch.Add(mGlowTex, {pr.Pos.x + sinf(ang) * sz * 0.7f, pr.Pos.y + cosf(ang) * sz * 0.7f}, {sz * 0.8f, sz * 1.6f},
                  Color{1.0f, 0.6f, 0.15f, 0.65f * reentry}, ang, -6.0f, RHI::BlendMode::Additive);
    }

    // 无人机
    for (const ConstructionDrone& d : mMecha.Drones())
    {
        if (!d.Active) continue;
        auto dpr = Project(d.Pos, vp, aspect);
        if (!dpr.Ok) continue;
        f32 dsz = Math::Clamp(2.2f * pscale / dpr.Depth, 2.0f, 18.0f);
        if (fx) batch.AddUv(fx, {0.5f, 0.0f}, {0.75f, 0.5f}, dpr.Pos, {dsz, dsz}, Color{1, 1, 1, 1}, mElapsedTime * 3.0f, -5.0f);
        if (!d.Returning)
        {
            Line3D(batch, *this, mSolidTex, d.Pos, d.TargetPos, vp, aspect, 1.4f,
                   Color{0.25f, 1.0f, 0.55f, 0.7f}, -5.0f, RHI::BlendMode::Additive);
        }
    }
}

// ---------------------------------------------------------------------------
// 9) 发射特效
// ---------------------------------------------------------------------------
void Game::RenderLaunchFx(Engine::SpriteBatch& batch, const Mat4& vp, f32 aspect)
{
    if (mLaunchFx.IsEmpty()) return;
    f32 fov = mUniverse.Camera().FovDeg();
    f32 pscale = ProjScale(fov);
    auto fx = DSPArt::TexFx();
    if (!fx || !mGlowTex) return;

    for (const LaunchFx& lf : mLaunchFx)
    {
        f32 t = Clamp01(lf.T);
        // 贝塞尔: 起点 → 抬升中点 → 恒星
        Vec3 mid = {(lf.From.x + lf.To.x) * 0.5f, (lf.From.y + lf.To.y) * 0.5f + 500.0f, (lf.From.z + lf.To.z) * 0.5f};
        f32 it = 1.0f - t;
        Vec3 p = {it * it * lf.From.x + 2 * it * t * mid.x + t * t * lf.To.x,
                  it * it * lf.From.y + 2 * it * t * mid.y + t * t * lf.To.y,
                  it * it * lf.From.z + 2 * it * t * mid.z + t * t * lf.To.z};
        Vec3 d = {2 * it * (mid.x - lf.From.x) + 2 * t * (lf.To.x - mid.x),
                  2 * it * (mid.y - lf.From.y) + 2 * t * (lf.To.y - mid.y),
                  2 * it * (mid.z - lf.From.z) + 2 * t * (lf.To.z - mid.z)};
        auto pr = Project(p, vp, aspect);
        if (!pr.Ok) continue;
        f32 px = Math::Clamp((lf.Rocket ? 8.0f : 5.0f) * pscale / pr.Depth, 3.0f, 40.0f);
        f32 ang = atan2f(d.y, d.x) + Math::HALF_PI;

        // 尾焰拖尾
        batch.Add(mGlowTex, pr.Pos, {px * 1.6f, px * 3.2f}, lf.Rocket ? Color{1.0f, 0.6f, 0.2f, 0.75f} : Color{0.3f, 0.9f, 1.0f, 0.65f},
                  ang, -2.0f, RHI::BlendMode::Additive);
        Vec2 uv0 = lf.Rocket ? Vec2{0.25f, 0.0f} : Vec2{0.0f, 0.0f};
        Vec2 uv1 = lf.Rocket ? Vec2{0.5f, 0.5f} : Vec2{0.25f, 0.5f};
        batch.AddUv(fx, uv0, uv1, pr.Pos, {px, px * 1.4f}, Color{1, 1, 1, 1}, ang, -1.0f);
    }
}

// ---------------------------------------------------------------------------
// 10) 镜头光晕
// ---------------------------------------------------------------------------
void Game::RenderLensFlare(Engine::SpriteBatch& batch, f32 aspect)
{
    if (!mSunOnScreen || !mGlowTex) return;
    mSunOnScreen = false;
    f32 halfW = 360.0f * aspect;
    Vec2 c = {0.0f, 0.0f};
    Vec2 s = mSunScreen;
    // 太阳在屏幕外太远则跳过
    if (fabsf(s.x) > halfW * 1.4f || fabsf(s.y) > 420.0f) return;

    auto flare = DSPArt::TexFlare();
    if (flare)
    {
        batch.Add(flare, s, {520.0f, 42.0f}, Color{1.0f, 0.9f, 0.75f, 0.30f}, 0.0f, 18.0f, RHI::BlendMode::Additive);
    }
    struct Ghost { f32 T; f32 Size; Color Tint; };
    Ghost ghosts[] =
    {
        {0.35f, 34.0f, Color{0.9f, 0.5f, 0.3f, 0.10f}},
        {0.65f, 22.0f, Color{0.3f, 0.8f, 1.0f, 0.09f}},
        {1.05f, 52.0f, Color{0.6f, 0.4f, 1.0f, 0.08f}},
        {1.45f, 16.0f, Color{1.0f, 0.8f, 0.4f, 0.12f}},
    };
    for (const Ghost& g : ghosts)
    {
        Vec2 p = {c.x + (c.x - s.x) * g.T, c.y + (c.y - s.y) * g.T};
        batch.Add(mGlowTex, p, {g.Size, g.Size}, g.Tint, 0.0f, 18.0f, RHI::BlendMode::Additive);
    }
}

} // namespace DSP
