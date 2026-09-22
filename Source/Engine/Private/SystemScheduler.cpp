#include "System.h"

#include "Threading/JobSystem.h"

#include <cstdio>

namespace Aether::Engine {

namespace {

//
// Conflict when the two systems share a blackboard tag with at least one
// side writing it. Read-read sharing is not a conflict.
//
bool Conflicts(const SystemDef& a, const SystemDef& b)
{
    for (u64 write : a.Writes)
    {
        if (b.Reads.Contains(write) || b.Writes.Contains(write))
        {
            return true;
        }
    }
    for (u64 read : a.Reads)
    {
        if (b.Writes.Contains(read))
        {
            return true;
        }
    }
    return false;
}

} // namespace

void SystemScheduler::Register(SystemDef def)
{
    AETHER_ASSERT(!mBuilt);
    AETHER_ASSERT(def.Update.IsValid());
    mSystems.Add(std::move(def));
}

void SystemScheduler::Build()
{
    if (mBuilt)
    {
        return;
    }

    for (u32 phase = 0; phase < (u32)Phase::Count; phase++)
    {
        Array<u32> indices;
        for (u32 i = 0; i < mSystems.Count(); i++)
        {
            if ((u32)mSystems[i].SysPhase == phase)
            {
                indices.Add(i);
            }
        }

        u32 count = indices.Count();
        Array<Array<u32>> successors;
        successors.Resize(count);
        Array<u32> indegree;
        indegree.Resize(count, 0);
        for (u32 u32i = 0; u32i < count; u32i++)
        {
            successors[u32i].Reserve(4);
        }

        // Edges only go from earlier-registered to later-registered systems,
        // so the graph is a DAG by construction; registration order breaks
        // every conflict.
        for (u32 a = 0; a < count; a++)
        {
            for (u32 b = a + 1; b < count; b++)
            {
                if (Conflicts(mSystems[indices[a]], mSystems[indices[b]]))
                {
                    successors[a].Add(b);
                    indegree[b]++;
                }
            }
        }

        // Kahn layering: each wave holds all systems whose predecessors are
        // done; those systems run concurrently. Waves store GLOBAL system
        // indices (successors/indegree are local to this phase).
        Array<Array<u32>>& waves = mWaves[phase];
        Array<bool> placed;
        placed.Resize(count, false);
        u32 placedCount = 0;
        while (placedCount < count)
        {
            Array<u32> waveLocal;
            for (u32 i = 0; i < count; i++)
            {
                if (!placed[i] && indegree[i] == 0)
                {
                    waveLocal.Add(i);
                }
            }
            if (waveLocal.IsEmpty())
            {
                printf("SystemScheduler: dependency cycle in phase %u\n", phase);
                AETHER_ASSERT(false);
                return;
            }
            for (u32 i : waveLocal)
            {
                placed[i] = true;
                placedCount++;
                for (u32 next : successors[i])
                {
                    indegree[next]--;
                }
            }
            Array<u32> wave;
            for (u32 i : waveLocal)
            {
                wave.Add(indices[i]);
            }
            waves.Add(std::move(wave));
        }
    }

    mBuilt = true;
}

void SystemScheduler::RunPhase(Phase phase, SystemContext& ctx)
{
    if (!mBuilt)
    {
        Build();
    }

    const Array<Array<u32>>& waves = mWaves[(u32)phase];
    for (const Array<u32>& wave : waves)
    {
        if (wave.Count() == 1)
        {
            // Singleton waves run inline on the calling thread; most phases
            // are a chain of these and job dispatch would be pure overhead.
            mSystems[wave.First()].Update(ctx);
            continue;
        }

        Platform::Jobs::JobCounter counter;
        for (u32 index : wave)
        {
            SystemDef& system = mSystems[index];
            Platform::Jobs::Run([&system, &ctx]() { system.Update(ctx); }, &counter);
        }
        Platform::Jobs::WaitFor(&counter);
    }
}

} // namespace Aether::Engine
