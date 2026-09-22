#pragma once

#include "Core.h"
#include "Container/Function.h"
#include "Renderer2D.h"
#include "System.h"
#include "Timer.h"

namespace Aether::Engine {

//
// Engine-owned frame loop. The loop itself owns no per-frame logic: every
// engine service (timer, input capture, frame begin/end, default UI) is a
// system registered into the same scheduler the game uses - the loop only
// gates renderer readiness, rewinds job scratch and walks the phases.
//
// The renderer is not owned; it must outlive the loop. The loop initializes
// the job system on construction.
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

    // The engine draws facade UI (flash text/tooltips/progress bars + a
    // sprite-batch flush) in the UI phase by default. Disable when the game
    // integrates UI::Draw into its own batch (e.g. DysonSphere).
    void SetAutoUI(bool enabled) { mAutoUI = enabled; }

    // Invoked once when the renderer first becomes ready; GPU resources can
    // be created there. Register game systems and publish world state in it
    // (engine systems are registered ahead of it, in the constructor).
    void SetStartupCallback(Function<void()> callback) { mStartup = std::move(callback); }

    // One frame; call from Platform::RunMainLoop.
    void Tick();

private:
    void RegisterEngineSystems();

    Renderer2D* mRenderer;
    Timer mTimer;
    SystemScheduler mScheduler;
    Blackboard mBlackboard;
    InputSnapshot mInputSnapshot;
    Function<void()> mStartup;
    u64 mFrameIndex = 0;
    bool mStarted = false;
    bool mOwnsJobs = false;
    bool mAutoUI = true;
};

} // namespace Aether::Engine
