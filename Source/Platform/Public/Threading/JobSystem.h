#pragma once

#include "Container/Array.h"
#include "Container/Function.h"
#include "Container/RefPtr.h"
#include "Core.h"
#include "Threading/Atomic.h"
#include "Threading/Thread.h"

namespace Aether::Platform::Jobs {

//
// Fixed-size job scheduler with work stealing.
//
// - Init() spawns (logical cores - 1) worker threads (overridable); the
//   thread that calls WaitFor participates in execution as well.
// - Jobs are coarse-grained: expect tens of microseconds or more. Submitting
//   tiny loops directly is wasteful; use ParallelFor, which splits ranges
//   into grain-sized chunks.
// - JobCounter counts outstanding jobs (Run increments, completion
//   decrements); WaitFor blocks (helping with other jobs) until it reaches
//   the target value, 0 by default.
// - ScratchAlloc hands out per-thread linear-arena memory, cache-line
//   aligned. ScratchReset rewinds every arena and must be called from the
//   main thread at a point where no jobs are in flight (frame start).
// - On the single-threaded web build every entry point executes inline on
//   the calling thread; no worker threads are created.
//

using JobCounter = Atomic<u32>;

void Init(u32 workerOverride = 0);
void Shutdown();
bool IsInitialized();

// Number of worker threads (excludes the calling thread).
u32 WorkerCount();

// Degree of usable parallelism: workers + the calling thread.
u32 Parallelism();

// 0 for the main/external thread, 1..WorkerCount() inside workers. Valid in
// job bodies; used to index per-thread state such as sprite bins.
u32 ThreadIndex();

// Executes task once on a worker (or inline on web). Decrements counter
// after the task returns.
void Run(Function<void()> task, JobCounter* counter = nullptr);

// Blocks the calling thread (executing any available jobs while waiting)
// until *counter reaches targetValue.
void WaitFor(JobCounter* counter, u32 targetValue = 0);

// Submits count independent jobs invoking fn(i) for i in [0, count).
template<typename F>
void Dispatch(u32 count, F&& fn, JobCounter* counter = nullptr)
{
    for (u32 i = 0; i < count; i++)
    {
        Run([i, &fn]() { fn(i); }, counter);
    }
}

// Splits [begin, end) into grain-sized ranges submitted as separate jobs;
// each invocation receives fn(rangeBegin, rangeEnd, threadIndex). If the
// range is small or parallelism is 1, runs inline without touching counter.
// fn and captured state must outlive the jobs when an external counter is
// passed (the caller must WaitFor it); with the default it waits internally.
template<typename F>
void ParallelFor(u64 begin, u64 end, u64 grain, F&& fn, JobCounter* counter = nullptr)
{
    if (end <= begin)
    {
        return;
    }
    if (grain == 0)
    {
        grain = 1;
    }
    u64 total = end - begin;
    u64 chunkCount = (total + grain - 1) / grain;
    if (chunkCount <= 1 || Parallelism() <= 1)
    {
        fn(begin, end, ThreadIndex());
        return;
    }

    JobCounter local;
    JobCounter* useCounter = counter != nullptr ? counter : &local;
    for (u64 first = begin; first < end;)
    {
        u64 last = first + grain;
        if (last > end)
        {
            last = end;
        }
        u64 rangeBegin = first;
        u64 rangeEnd = last;
        Run([rangeBegin, rangeEnd, &fn]() { fn(rangeBegin, rangeEnd, ThreadIndex()); },
            useCounter);
        first = last;
    }
    if (counter == nullptr)
    {
        WaitFor(&local);
    }
}

// Per-thread linear scratch arena. Alignment must be a power of two.
void* ScratchAlloc(u64 size, u32 alignment = CACHE_LINE_SIZE);

// Rewinds all arenas; only call from the main thread with no jobs in flight.
void ScratchReset();

} // namespace Aether::Platform::Jobs
