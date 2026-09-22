#pragma once

#include "Core.h"
#include "Renderer2D.h"
#include "Timer.h"
#include "Universe.h"
#include "Mecha.h"
#include "Factory.h"
#include "Power.h"
#include "TechTree.h"
#include "Blueprint.h"
#include "DSPUI.h"
#include "World3D.h"
#include "Container/Array.h"
#include "Container/RefPtr.h"
#include "RHI.h"
#include "System.h"

namespace DSP {

using Aether::f32;
using Aether::i32;
using Aether::u8;
using Aether::u32;
using Aether::Math::Mat4;
using Aether::Math::Vec2;
using Aether::Math::Vec3;
using Aether::Array;
using Aether::RefPtr;

namespace Engine = Aether::Engine;

class Game {
public:
    Game(Aether::Engine::Renderer2D* renderer, Aether::Engine::Timer* timer);
    ~Game();

    // 把拆分后的 System 注册进调度器 (在渲染器就绪后调用)。
    void RegisterSystems(Aether::Engine::SystemScheduler& scheduler);

    // 3D 世界坐标 → 2D 屏幕坐标 (中心原点, y 向下); 供静态绘制辅助使用
    struct Proj {
        Vec2 Pos{0.0f, 0.0f};
        f32 Depth = 1.0f;
        bool Ok = false;
    };
    Proj Project(const Vec3& worldP, const Mat4& vp, f32 aspect) const;

private:
    // --- Simulation 系统 (依赖由 Reads/Writes 标签声明给调度器) ---
    void SysPlayerInput(Aether::Engine::SystemContext& ctx);
    void SysUniverse(Aether::Engine::SystemContext& ctx);
    void SysMecha(Aether::Engine::SystemContext& ctx);
    void SysFactory(Aether::Engine::SystemContext& ctx);
    void SysPower(Aether::Engine::SystemContext& ctx);
    void SysTech(Aether::Engine::SystemContext& ctx);
    void SysDyson(Aether::Engine::SystemContext& ctx);
    void SysCameraAmbience(Aether::Engine::SystemContext& ctx);

    // --- 渲染: RenderPrep 并行构建 (CPU), RenderSubmit 主线程提交 ---
    void SysRenderPrep(Aether::Engine::SystemContext& ctx);
    void SysRenderSubmit(Aether::Engine::SystemContext& ctx);

    // --- 输入与建造 ---
    void HandleInput(f32 dt);
    void UpdateCursor(const Vec3& rayO, const Vec3& rayD);
    void TryPlace(u32 tileKey, bool dragChain);
    bool TryDismantle(u32 tileKey);
    void InitStartingFactory();
    u32 FindSpawnTile();

    // --- 渲染分层 ---
    // Pass A (2D): 深空背景 + 恒星 → 3D pass: 地形/水面/大气/戴森球 (深度缓冲)
    // Pass B (2D): 工厂/机甲/矿脉/UI
    void RenderSpace(Aether::Engine::SpriteBatch& batch, const Mat4& vp, f32 aspect);
    void RenderSkyDome(Aether::Engine::SpriteBatch& batch, const Planet* planet);
    void RenderSun(Aether::Engine::SpriteBatch& batch, const Mat4& vp, f32 aspect);
    void BuildDysonMesh();            // 并行重建戴森球顶点 (RenderPrep 阶段)
    void EnsureTerrainMeshes(Planet* planet);
    void RebuildFlatViewMesh();
    void RenderVeins(Aether::Engine::SpriteBatch& batch, const Mat4& vp, f32 aspect, const Planet* planet);
    void RenderCursorGhost(Aether::Engine::SpriteBatch& batch, const Mat4& vp, f32 aspect, const Planet* planet);
    void RenderFactory(Aether::Engine::SpriteBatch& batch, const Mat4& vp, f32 aspect, const Planet* planet);
    void ProcessBuildingSprite(Aether::Engine::SpriteBatch& batch, const Building& b,
                               const Mat4& vp, f32 aspect, const Planet* planet,
                               f32 pscale, const Vec3& eyeDir, f32 horizonCos, bool showBars,
                               const RefPtr<Aether::RHI::RHITexture>& btex,
                               const RefPtr<Aether::RHI::RHITexture>& items);
    void RenderMecha(Aether::Engine::SpriteBatch& batch, const Mat4& vp, f32 aspect, const Planet* planet);
    void RenderLaunchFx(Aether::Engine::SpriteBatch& batch, const Mat4& vp, f32 aspect);
    void RenderLensFlare(Aether::Engine::SpriteBatch& batch, f32 aspect);

    void UpdateLaunchFx(f32 dt);

    Aether::Engine::Renderer2D* mRenderer = nullptr;
    Aether::Engine::Timer* mTimer = nullptr;

    World3D mWorld3D;
    Planet* mBuiltPlanet = nullptr;   // 已构建地形网格的星球 (切换星球时重建)
    bool mFlatView = false;           // G: 平面地形调试视图

    Universe mUniverse;
    Mecha mMecha;
    FactorySystem mFactory;
    PowerGrid mPower;
    TechTreeManager mTechTree;
    BlueprintManager mBlueprints;
    DSPUI mUI;

    // 深空背景独立批次: Pass A 与 Pass B 分开冲刷, Pass B 才能吃满多线程 bin
    Aether::Engine::SpriteBatch mSpaceBatch;

    // 戴森球动态网格: RenderPrep 并行重建, RenderSubmit 上传
    Array<World3D::Vertex> mDysonLines;
    Array<World3D::Vertex> mDysonTris;
    Array<World3D::Vertex> mDysonSailParts[Aether::Engine::SpriteBatch::MAX_BINS];

    // 帧渲染缓存 (prep 填充, submit 读取)
    f32 mRcAspect = 1.0f;
    Mat4 mRcVp3d;
    Mat4 mRcVp2d;

    f32 mSimDt = 0.0f;                // PlayerInput 系统钳制后的本帧 dt

    // 光标
    u32 mCursorTile = 0;
    Vec3 mCursorWorldPos{0.0f, 0.0f, 0.0f};
    bool mCursorOnPlanet = false;
    i32 mBuildDir = 0;              // 0=+u 1=-u 2=+v 3=-v
    bool mBeltDragging = false;     // 拖拽铺带
    u32 mBeltLastTile = 0;
    f32 mClickCooldown = 0.0f;
    f32 mSpriteRot = 0.0f;          // 机甲精灵朝向 (对齐表面法线, 帧间平滑)

    // 太阳与昼夜
    Vec3 mSunPos{-2200.0f, 900.0f, -2800.0f};
    Vec3 mSunDir{1.0f, 0.0f, 0.0f};
    Vec2 mSunScreen{0.0f, 0.0f};
    bool mSunOnScreen = false;
    f32 mPlanetRotationSeed = 0.7f;
    f32 mElapsedTime = 0.0f;

    // 发射特效
    struct LaunchFx {
        Vec3 From{0.0f, 0.0f, 0.0f};
        Vec3 To{0.0f, 0.0f, 0.0f};
        f32 T = 0.0f;
        f32 Dur = 3.0f;
        bool Rocket = false;
    };
    Array<LaunchFx> mLaunchFx;
    u32 mPrevSails = 0;
    u32 mPrevRockets = 0;

    // 星空缓存
    struct BgStar { f32 Theta, Phi, Size, Phase; u8 Warm; };
    Array<BgStar> mBgStars;
    bool mStarsInit = false;

    // 基础纹理
    RefPtr<Aether::RHI::RHITexture> mSolidTex;
    RefPtr<Aether::RHI::RHITexture> mGlowTex;
};

} // namespace DSP
