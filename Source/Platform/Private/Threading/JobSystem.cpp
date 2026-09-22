#include "Platform.h"
#include "Threading/JobSystem.h"
#include "Threading/Mutex.h"
#include "Threading/Thread.h"

#include <cstdlib>

#if !AETHER_PLATFORM_WEB
#include <chrono>
#include <condition_variable>
#include <mutex>
#endif

namespace Aether::Platform::Jobs {

//
// Implementation notes (desktop):
// - Each worker owns a FIFO/LIFO deque: Push back / owner pops back (LIFO,
//   better cache locality) / thieves pop front (oldest, largest chunks).
// - External submissions (main thread) go to the shared queue; jobs
//   submitted from inside a worker go to that worker's local queue.
// - Idle workers block on a condition variable notified once per submitted
//   job, with a short poll timeout as a backstop. The queues are
//   mutex-guarded; at engine job granularities (>= tens of microseconds)
//   lock cost is negligible versus a hand-rolled lock-free deque, and the
//   JobQueue seam below is where a Chase-Lev swap would go if profiling
//   ever demands it.
//
// Implementation notes (web): everything runs inline on the calling thread.
//

#if AETHER_PLATFORM_WEB

namespace {

bool gInitialized = false;
u8* gScratchBase = nullptr;
u64 gScratchSize = 0;
u64 gScratchOffset = 0;
constexpr u64 WEB_SCRATCH_BYTES = 4 * 1024 * 1024;

} // namespace

void Init(u32 workerOverride)
{
    AETHER_UNUSED(workerOverride);
    gScratchBase = (u8*)std::malloc(WEB_SCRATCH_BYTES);
    gScratchSize = gScratchBase != nullptr ? WEB_SCRATCH_BYTES : 0;
    gInitialized = true;
}

void Shutdown()
{
    std::free(gScratchBase);
    gScratchBase = nullptr;
    gScratchSize = 0;
    gScratchOffset = 0;
    gInitialized = false;
}

bool IsInitialized() { return gInitialized; }

u32 WorkerCount() { return 0; }

u32 Parallelism() { return 1; }

u32 ThreadIndex() { return 0; }

void Run(Function<void()> task, JobCounter* counter)
{
    if (counter != nullptr)
    {
        counter->FetchAddAcqRel(1);
    }
    task();
    if (counter != nullptr)
    {
        counter->FetchSubAcqRel(1);
    }
}

void WaitFor(JobCounter* counter, u32 targetValue)
{
    AETHER_ASSERT(counter == nullptr || counter->LoadAcquire() == targetValue);
    AETHER_UNUSED(counter);
    AETHER_UNUSED(targetValue);
}

void* ScratchAlloc(u64 size, u32 alignment)
{
    u64 aligned = (gScratchOffset + alignment - 1) & ~((u64)alignment - 1);
    AETHER_ASSERT(aligned + size <= gScratchSize);
    if (aligned + size > gScratchSize)
    {
        return nullptr;
    }
    gScratchOffset = aligned + size;
    return gScratchBase + aligned;
}

void ScratchReset()
{
    gScratchOffset = 0;
}

#else // !AETHER_PLATFORM_WEB

namespace {

constexpr u32 MAX_WORKERS = 32;
constexpr u32 MAX_THREADS = MAX_WORKERS + 1;
constexpr u64 SCRATCH_BLOCK_BYTES = 1024 * 1024;

struct Job
{
    Function<void()> Task;
    JobCounter* Counter;
};

//
// Mutex-guarded deque. Push/PopBack/StealFront; isolated so a lock-free
// implementation can replace it wholesale later.
//
class JobQueue
{
public:
    void Push(Job* job)
    {
        ScopedLock lock(mLock);
        mItems.Add(job);
    }

    Job* PopBack()
    {
        ScopedLock lock(mLock);
        if (mItems.IsEmpty())
        {
            return nullptr;
        }
        Job* job = mItems.Last();
        mItems.RemoveAt(mItems.Count() - 1);
        return job;
    }

    Job* StealFront()
    {
        ScopedLock lock(mLock);
        if (mItems.IsEmpty())
        {
            return nullptr;
        }
        Job* job = mItems.First();
        mItems.RemoveAtSwap(0);
        return job;
    }

private:
    Mutex mLock;
    Array<Job*> mItems;
};

bool gInitialized = false;
bool gShutdownStarted = false;
u32 gWorkerCount = 0;

JobQueue gSharedQueue;
JobQueue gLocalQueues[MAX_WORKERS];
Array<UniquePtr<Thread>> gWorkerThreads;

// Worker sleep/wake. notify_one per submitted job; poll timeout backstop.
std::mutex gSleepMutex;
std::condition_variable gSleepCond;

thread_local u32 tlsThreadIndex = 0;

//
// Per-thread scratch arena: chain of blocks; the first block is kept across
// ScratchReset calls, later blocks (grown on overflow) are freed.
//
struct ScratchBlock
{
    ScratchBlock* Next;
    u64 Size;
    u8* Base() { return reinterpret_cast<u8*>(this) + sizeof(ScratchBlock); }
};

struct ScratchArena
{
    ScratchBlock* Head = nullptr;
    u64 Offset = 0;

    ScratchBlock* EnsureBlock(u64 needed)
    {
        u64 blockSize = needed > SCRATCH_BLOCK_BYTES ? needed : SCRATCH_BLOCK_BYTES;
        ScratchBlock* block =
            (ScratchBlock*)std::malloc(sizeof(ScratchBlock) + blockSize);
        AETHER_ASSERT(block != nullptr);
        if (block != nullptr)
        {
            block->Next = nullptr;
            block->Size = blockSize;
        }
        return block;
    }

    void* Alloc(u64 size, u32 alignment)
    {
        if (Head == nullptr)
        {
            Head = EnsureBlock(size);
            Offset = 0;
        }
        u64 base = (u64)Head->Base();
        u64 aligned = (base + Offset + alignment - 1) & ~((u64)alignment - 1);
        u64 rel = aligned - base;
        if (rel + size <= Head->Size)
        {
            Offset = rel + size;
            return (void*)aligned;
        }
        // Overflow: grow a new block and abandon the remainder of the old one.
        ScratchBlock* grown = EnsureBlock(size);
        if (grown == nullptr)
        {
            return nullptr;
        }
        grown->Next = Head;
        Head = grown;
        Offset = size;
        return Head->Base();
    }

    void Reset()
    {
        if (Head == nullptr)
        {
            return;
        }
        ScratchBlock* doomed = Head->Next;
        while (doomed != nullptr)
        {
            ScratchBlock* next = doomed->Next;
            std::free(doomed);
            doomed = next;
        }
        Head->Next = nullptr;
        Offset = 0;
    }

    void FreeAll()
    {
        ScratchBlock* block = Head;
        while (block != nullptr)
        {
            ScratchBlock* next = block->Next;
            std::free(block);
            block = next;
        }
        Head = nullptr;
        Offset = 0;
    }
};

ScratchArena gScratch[MAX_THREADS];

void NotifyOneWorker()
{
    std::lock_guard<std::mutex> lock(gSleepMutex);
    gSleepCond.notify_one();
}

void ExecuteJob(Job* job)
{
    job->Task();
    if (job->Counter != nullptr)
    {
        job->Counter->FetchSubAcqRel(1);
    }
    delete job;
}

// Finds the next runnable job: own local queue (LIFO), shared queue, then
// stealing from other workers (round-robin from a pseudo-random start).
Job* TryNextJob(u32 threadIndex)
{
    if (threadIndex > 0)
    {
        Job* local = gLocalQueues[threadIndex - 1].PopBack();
        if (local != nullptr)
        {
            return local;
        }
    }
    Job* shared = gSharedQueue.StealFront();
    if (shared != nullptr)
    {
        return shared;
    }
    if (gWorkerCount > 1)
    {
        u32 start = ((threadIndex + 1) * 7919u) % gWorkerCount;
        for (u32 i = 0; i < gWorkerCount; i++)
        {
            u32 victim = (start + i) % gWorkerCount;
            if (victim + 1 == threadIndex)
            {
                continue;
            }
            Job* stolen = gLocalQueues[victim].StealFront();
            if (stolen != nullptr)
            {
                return stolen;
            }
        }
    }
    return nullptr;
}

void WorkerLoop(u32 index)
{
    tlsThreadIndex = index + 1;
    while (!gShutdownStarted)
    {
        Job* job = TryNextJob(index + 1);
        if (job != nullptr)
        {
            ExecuteJob(job);
            continue;
        }
        {
            std::unique_lock<std::mutex> lock(gSleepMutex);
            gSleepCond.wait_for(lock, std::chrono::milliseconds(8));
        }
    }
}

struct JobSystemShutdown
{
    ~JobSystemShutdown()
    {
        if (gInitialized)
        {
            Shutdown();
        }
    }
};
JobSystemShutdown gAutoShutdown;

} // namespace

void Init(u32 workerOverride)
{
    AETHER_ASSERT(!gInitialized);
    if (gInitialized)
    {
        return;
    }

    u32 cores = CpuInfo::LogicalCoreCount();
    u32 workers = workerOverride > 0 ? workerOverride : (cores > 1 ? cores - 1 : 1);
    if (workers > MAX_WORKERS)
    {
        workers = MAX_WORKERS;
    }

    gShutdownStarted = false;
    gWorkerCount = workers;
    gWorkerThreads.Reserve(workers);
    for (u32 i = 0; i < workers; i++)
    {
        auto thread = MakeUnique<Thread>();
        thread->Run([i]() { WorkerLoop(i); }, "AetherWorker");
        gWorkerThreads.Add(std::move(thread));
    }
    gInitialized = true;
}

void Shutdown()
{
    if (!gInitialized)
    {
        return;
    }
    gShutdownStarted = true;
    {
        std::lock_guard<std::mutex> lock(gSleepMutex);
        gSleepCond.notify_all();
    }
    for (auto& thread : gWorkerThreads)
    {
        thread->Join();
    }
    gWorkerThreads.Clear();
    gWorkerCount = 0;
    for (u32 i = 0; i < MAX_THREADS; i++)
    {
        gScratch[i].FreeAll();
    }
    gInitialized = false;
}

bool IsInitialized() { return gInitialized; }

u32 WorkerCount() { return gWorkerCount; }

u32 Parallelism() { return gWorkerCount + 1; }

u32 ThreadIndex() { return tlsThreadIndex; }

void Run(Function<void()> task, JobCounter* counter)
{
    AETHER_ASSERT(gInitialized);
    if (counter != nullptr)
    {
        counter->FetchAddAcqRel(1);
    }
    auto* job = new Job{std::move(task), counter};

    if (tlsThreadIndex > 0)
    {
        gLocalQueues[tlsThreadIndex - 1].Push(job);
    }
    else
    {
        gSharedQueue.Push(job);
    }
    NotifyOneWorker();
}

void WaitFor(JobCounter* counter, u32 targetValue)
{
    AETHER_ASSERT(counter != nullptr);
    while (counter->LoadAcquire() != targetValue)
    {
        Job* job = TryNextJob(tlsThreadIndex);
        if (job != nullptr)
        {
            ExecuteJob(job);
        }
        else
        {
            Thread::YieldCpu();
        }
    }
}

void* ScratchAlloc(u64 size, u32 alignment)
{
    AETHER_ASSERT(alignment != 0 && (alignment & (alignment - 1)) == 0);
    return gScratch[tlsThreadIndex].Alloc(size, alignment);
}

void ScratchReset()
{
    for (u32 i = 0; i < MAX_THREADS; i++)
    {
        gScratch[i].Reset();
    }
}

#endif // AETHER_PLATFORM_WEB

} // namespace Aether::Platform::Jobs
