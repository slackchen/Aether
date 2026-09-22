#pragma once

#include "Core.h"
#include "Container/Function.h"
#include "Renderer2D.h"
#include "System.h"
#include "Timer.h"

namespace Aether::Engine {

//
// Engine-owned frame loop. Replaces the per-demo FrameLoop body:
//
//   Tick() {
//     renderer.Tick() until ready (startup callback fires once)
//     scratch reset, timer update, input snapshot capture
//     phases: Input -> Simulation -> RenderPrep
//     BeginFrame -> RenderSubmit -> UI -> EndFrame
//   }
//
// The loop initializes the job system on construction. The renderer is not
// owned; it must outlive the loop.
//
class EngineLoop
{
public:
    explicit EngineLoop(Renderer2D& renderer);
    ~EngineLoop();

    EngineLoop(const EngineLoop&) = delete;
    EngineLoop& operator=(const EngineLoop&) = delete;

    SystemScheduler& Scheduler() { return mScheduler; }
    Blackboard& World() { return mBlackboard; }
    Timer& GetTimer() { return mTimer; }

    // Invoked once when the renderer first becomes ready; GPU resources can
    // be created there. Register systems and publish world state in it.
    void SetStartupCallback(Function<void()> callback) { mStartup = std::move(callback); }

    // One frame; call from Platform::RunMainLoop.
    void Tick();

private:
    Renderer2D* mRenderer;
    Timer mTimer;
    SystemScheduler mScheduler;
    Blackboard mBlackboard;
    InputSnapshot mInputSnapshot;
    Function<void()> mStartup;
    u64 mFrameIndex = 0;
    bool mStarted = false;
    bool mOwnsJobs = false;
};

} // namespace Aether::Engine
