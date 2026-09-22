#include "EngineLoop.h"

#include "Platform.h"
#include "Threading/JobSystem.h"
#include "UI.h"

#include <cstdio>

namespace Aether::Engine {

namespace {

//
// Engine-owned systems. All capture `loop` by reference; the loop owns the
// scheduler and outlives every frame.
//

// Phase::Input, first: fills ctx time fields. Game systems in later phases
// read ctx.Delta; Input-phase systems must declare Tags::Time to be ordered.
void TimerSystem(EngineLoop& loop, SystemContext& ctx)
{
    loop.GetTimer().Update();
    ctx.Delta = loop.GetTimer().Delta();
    ctx.UnscaledDelta = loop.GetTimer().UnscaledDelta();
    ctx.Elapsed = loop.GetTimer().Time();
}

// Phase::Input: fills the frame's immutable input snapshot. Readers in the
// same phase must declare Tags::Input; later phases are already ordered.
void InputSystem(InputSnapshot& snapshot, SystemContext& ctx)
{
    Input::CaptureSnapshot(snapshot);
}

// Phase::FrameBegin: opens the render pass. RenderSubmit/UI systems must
// early-out when ctx.FrameActive is false (e.g. resize race).
void FrameBeginSystem(Renderer2D& renderer, SystemContext& ctx)
{
    ctx.FrameActive = renderer.BeginFrame();
}

// Phase::UI (optional, SetAutoUI): draws facade UI into a fresh batch and
// flushes it screen-space. Games that integrate UI::Draw into their own
// batch disable this via SetAutoUI(false).
void UISystem(Renderer2D& renderer, SystemContext& ctx)
{
    if (!ctx.FrameActive)
    {
        return;
    }
    SpriteBatch& batch = renderer.Sprites();
    batch.Clear();
    UI::Draw(&renderer);
    Camera2D& cam = renderer.GetCamera();
    Camera2D saved = cam;
    cam.Position = {0.0f, 0.0f};
    cam.Zoom = 1.0f;
    batch.Render(renderer.Encoder(), cam.ViewProjection(renderer.Aspect()));
    cam = saved;
}

// Phase::FrameEnd, last: closes the render pass and presents. EndFrame is
// self-guarding when the pass was never opened.
void FrameEndSystem(Renderer2D& renderer, SystemContext& ctx)
{
    renderer.EndFrame();
}

} // namespace

EngineLoop::EngineLoop(Renderer2D& renderer)
    : mRenderer(&renderer)
{
    if (!Platform::Jobs::IsInitialized())
    {
        Platform::Jobs::Init();
        mOwnsJobs = true;
    }
    RegisterEngineSystems();
}

EngineLoop::~EngineLoop()
{
    if (mOwnsJobs && Platform::Jobs::IsInitialized())
    {
        Platform::Jobs::Shutdown();
    }
}

void EngineLoop::RegisterEngineSystems()
{
    // Registration order is semantic order: engine systems run ahead of any
    // system the startup callback registers.
    SystemDef timer;
    timer.Name = "EngineTimer";
    timer.SysPhase = Phase::Input;
    timer.Update = [this](SystemContext& ctx) { TimerSystem(*this, ctx); };
    timer.Writes = {TypeIdOf<Tags::Time>()};
    mScheduler.Register(std::move(timer));

    SystemDef input;
    input.Name = "EngineInput";
    input.SysPhase = Phase::Input;
    input.Update = [this](SystemContext& ctx) { InputSystem(mInputSnapshot, ctx); };
    input.Writes = {TypeIdOf<Tags::Input>()};
    mScheduler.Register(std::move(input));

    SystemDef frameBegin;
    frameBegin.Name = "EngineFrameBegin";
    frameBegin.SysPhase = Phase::FrameBegin;
    frameBegin.Update = [this](SystemContext& ctx) { FrameBeginSystem(*mRenderer, ctx); };
    mScheduler.Register(std::move(frameBegin));

    SystemDef ui;
    ui.Name = "EngineUI";
    ui.SysPhase = Phase::UI;
    ui.Update = [this](SystemContext& ctx) {
        if (mAutoUI)
        {
            UISystem(*mRenderer, ctx);
        }
    };
    mScheduler.Register(std::move(ui));

    SystemDef frameEnd;
    frameEnd.Name = "EngineFrameEnd";
    frameEnd.SysPhase = Phase::FrameEnd;
    frameEnd.Update = [this](SystemContext& ctx) { FrameEndSystem(*mRenderer, ctx); };
    mScheduler.Register(std::move(frameEnd));
}

void EngineLoop::Tick()
{
    mRenderer->Tick();
    if (!mRenderer->IsReady())
    {
        if (mRenderer->IsFailed())
        {
            printf("EngineLoop: renderer failed to initialize\n");
            Platform::RequestExit();
        }
        return;
    }

    if (!mStarted)
    {
        mStarted = true;
        if (mStartup.IsValid())
        {
            mStartup();
        }
        mScheduler.Build();
    }

    // Between frames no jobs are in flight; rewind the per-thread arenas.
    Platform::Jobs::ScratchReset();

    SystemContext ctx;
    ctx.FrameIndex = mFrameIndex;
    ctx.Input = &mInputSnapshot;
    ctx.World = &mBlackboard;

    for (u32 p = 0; p < (u32)Phase::Count; p++)
    {
        mScheduler.RunPhase((Phase)p, ctx);
    }

    ++mFrameIndex;
}

} // namespace Aether::Engine
