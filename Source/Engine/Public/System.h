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
//   Input        - consume the input snapshot, produce intent (camera, commands)
//   Simulation   - advance game state
//   RenderPrep   - build render data from state in parallel (CPU heavy)
//   RenderSubmit - issue draw calls on the main thread (inside BeginFrame/EndFrame)
//   UI           - immediate-mode UI drawing (inside BeginFrame/EndFrame)
//
enum class Phase
{
    Input,
    Simulation,
    RenderPrep,
    RenderSubmit,
    UI,
    Count
};

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
// Read-only per-frame context handed to every system update.
//
struct SystemContext
{
    f32 Delta = 0.0f;
    f32 UnscaledDelta = 0.0f;
    f32 Elapsed = 0.0f;
    u64 FrameIndex = 0;
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
