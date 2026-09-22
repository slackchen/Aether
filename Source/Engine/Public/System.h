#pragma once

#include "Container/Array.h"
#include "Container/Function.h"
#include "Container/HashMap.h"
#include "Core.h"
#include "Input.h"

namespace Aether::Engine {

//
// Frame phases, in execution order. Phases are synchronization points:
// every system of a phase completes before the next phase starts.
//
//   Input        - engine InputSystem fills the input snapshot; game systems
//                  consume it and produce intent (camera, commands)
//   Simulation   - advance game state
//   FrameBegin   - engine FrameBeginSystem: BeginFrame (open render pass,
//                  clear sprite batch)
//   RenderPrep   - build render data in parallel (CPU heavy; sprites
//                  accumulate into per-thread bins AFTER the batch was
//                  cleared by FrameBegin)
//   RenderSubmit - flush batches / issue draw calls (main thread)
//   UI           - immediate-mode UI drawing
//   FrameEnd     - engine FrameEndSystem: EndFrame (submit + present)
//
// Engine-owned services (timer, input, frame begin/end, default UI) are
// themselves systems, registered by EngineLoop before the game registers
// its own - registration order keeps them ahead of game systems.
//
enum class Phase
{
    Input,
    Simulation,
    FrameBegin,
    RenderPrep,
    RenderSubmit,
    UI,
    FrameEnd,
    Count
};

//
// Blackboard tags written by the engine's own systems. Game systems that
// read the corresponding context state during the SAME phase must declare
// these in Reads to be ordered after the engine system (later phases are
// ordered by the phase boundary regardless).
//
namespace Tags {
struct Time;   // ctx.Delta / UnscaledDelta / Elapsed (engine TimerSystem)
struct Input;  // ctx.Input snapshot contents (engine InputSystem)
} // namespace Tags

//
// Stable per-type id (address of a static byte, unique per T per process).
//
template<typename T>
u64 TypeIdOf()
{
    static const char kId = 0;
    return reinterpret_cast<u64>(&kId);
}

//
// Typed service locator. Systems own their data and publish pointers here.
// The scheduler uses the Reads/Writes tags of SystemDef to order access;
// concurrent systems must not touch the same entry unless both declared it
// read-only.
//
class Blackboard
{
public:
    template<typename T>
    T* Get()
    {
        void** entry = mEntries.Find(TypeIdOf<T>());
        return entry != nullptr ? static_cast<T*>(*entry) : nullptr;
    }

    template<typename T>
    void Put(T* value)
    {
        mEntries.Add(TypeIdOf<T>(), reinterpret_cast<void*>(value));
    }

    template<typename T>
    void Remove()
    {
        mEntries.Remove(TypeIdOf<T>());
    }

private:
    HashMap<u64, void*> mEntries;
};

//
// Per-frame context handed to every system update. Delta/Elapsed are filled
// by the engine TimerSystem (Phase::Input); Input points at the snapshot the
// engine InputSystem fills (Phase::Input); FrameActive reports whether
// FrameBeginSystem successfully opened the render pass - RenderSubmit/UI
// systems must early-out when it is false.
//
struct SystemContext
{
    f32 Delta = 0.0f;
    f32 UnscaledDelta = 0.0f;
    f32 Elapsed = 0.0f;
    u64 FrameIndex = 0;
    bool FrameActive = false;
    const InputSnapshot* Input = nullptr;
    Blackboard* World = nullptr;
};

using SystemUpdate = Function<void(SystemContext&)>;

//
// System definition. Two systems in the same phase conflict when they share
// a tag with at least one side writing it; conflicting systems run in
// registration order, non-conflicting systems run concurrently on the job
// system (serially on the web build).
//
struct SystemDef
{
    const char* Name = "";
    Phase SysPhase = Phase::Simulation;
    SystemUpdate Update;
    Array<u64> Reads;
    Array<u64> Writes;
};

class SystemScheduler
{
public:
    // Must be called before Build (i.e. from the startup callback).
    void Register(SystemDef def);

    // Topo-sorts every phase into concurrency waves; call once after all
    // systems are registered (EngineLoop does this automatically).
    void Build();

    // Runs one phase: waves execute sequentially, systems inside a wave
    // execute concurrently and are joined before the next wave.
    void RunPhase(Phase phase, SystemContext& ctx);

    u32 SystemCount() const { return mSystems.Count(); }

private:
    Array<SystemDef> mSystems;
    Array<Array<u32>> mWaves[(u32)Phase::Count];
    bool mBuilt = false;
};

} // namespace Aether::Engine
