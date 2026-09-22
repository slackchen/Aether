#pragma once

#include "Core.h"
#include "Threading/Atomic.h"

namespace Aether::Platform {

//
// Kernel/green mutex for coarse critical sections (queues, cold state).
// Short hot-section locks should prefer SpinLock or atomics.
//
// Opaque storage so no std type leaks into public headers; the concrete
// object is placement-constructed by Private/Threading/Threading.cpp and a
// future native implementation (SRWLOCK on Windows) can change size freely.
//
class Mutex
{
public:
    Mutex();
    ~Mutex();

    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;

    void Lock();
    void Unlock();
    bool TryLock();

private:
    friend struct MutexImpl;

    alignas(CACHE_LINE_SIZE) u8 mStorage[128]{};
};

//
// RAII scoped lock; the engine-side replacement for std::lock_guard.
//
class ScopedLock
{
public:
    explicit ScopedLock(Mutex& mutex)
        : mMutex(&mutex)
    {
        mMutex->Lock();
    }

    ~ScopedLock() { mMutex->Unlock(); }

    ScopedLock(const ScopedLock&) = delete;
    ScopedLock& operator=(const ScopedLock&) = delete;

private:
    Mutex* mMutex;
};

} // namespace Aether::Platform
