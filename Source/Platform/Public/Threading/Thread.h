#pragma once

#include "Container/Function.h"
#include "Core.h"
#include "Threading/Atomic.h"

namespace Aether::Platform {

//
// Priority hint handed to Thread::Run / SetCurrentThreadPriority.
//
enum class ThreadPriority
{
    Normal,
    AboveNormal,
    Background,
    // MMCSS "Audio" boost on Windows (event-driven pro-audio threads);
    // maps to a normal priority elsewhere.
    Audio,
};

//
// OS thread wrapper. The handle is created by Run() and consumed by Join() or
// Detach(); the destructor asserts the thread is no longer owned.
//
// This is the only thread type engine code may create. It wraps std::thread
// today; a platform-specific implementation (native handles, fiber-backed
// workers) can replace Private/Threading/Threading.cpp wholesale.
//
class Thread
{
public:
    Thread();
    ~Thread();

    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;

    // Spawns the thread, optionally naming it and boosting it before entry.
    void Run(Function<void()> entry, const char* name = nullptr,
             ThreadPriority priority = ThreadPriority::Normal);

    bool Joinable() const;
    void Join();
    void Detach();

    // Applies to the calling thread (usable from inside Run entry points).
    static void SetCurrentThreadName(const char* name);
    static void SetCurrentThreadPriority(ThreadPriority priority);

    static void SleepMillis(u32 millis);
    static void YieldCpu();

private:
    friend struct ThreadImpl;

    // Opaque storage for the underlying handle; large enough for std::thread
    // on all supported platforms, cache-line aligned against false sharing.
    alignas(CACHE_LINE_SIZE) u8 mStorage[16]{};
};

} // namespace Aether::Platform
