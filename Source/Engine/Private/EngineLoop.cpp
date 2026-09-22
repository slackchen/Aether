#include "EngineLoop.h"

#include "Platform.h"
#include "Threading/JobSystem.h"

#include <cstdio>

namespace Aether::Engine {

EngineLoop::EngineLoop(Renderer2D& renderer)
    : mRenderer(&renderer)
{
    if (!Platform::Jobs::IsInitialized())
    {
        Platform::Jobs::Init();
        mOwnsJobs = true;
    }
}

EngineLoop::~EngineLoop()
{
    if (mOwnsJobs && Platform::Jobs::IsInitialized())
    {
        Platform::Jobs::Shutdown();
    }
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

    mTimer.Update();
    Input::CaptureSnapshot(mInputSnapshot);

    SystemContext ctx;
    ctx.Delta = mTimer.Delta();
    ctx.UnscaledDelta = mTimer.UnscaledDelta();
    ctx.Elapsed = mTimer.Time();
    ctx.FrameIndex = mFrameIndex;
    ctx.Input = &mInputSnapshot;
    ctx.World = &mBlackboard;

    mScheduler.RunPhase(Phase::Input, ctx);
    mScheduler.RunPhase(Phase::Simulation, ctx);
    mScheduler.RunPhase(Phase::RenderPrep, ctx);

    if (mRenderer->BeginFrame())
    {
        mScheduler.RunPhase(Phase::RenderSubmit, ctx);
        mScheduler.RunPhase(Phase::UI, ctx);
        mRenderer->EndFrame();
    }

    ++mFrameIndex;
}

} // namespace Aether::Engine
