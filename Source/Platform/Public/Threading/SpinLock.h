#pragma once

#include "Core.h"
#include "Threading/Atomic.h"

namespace Aether::Platform {

//
// Spin lock for very short critical sections (a few dozen cycles) that are
// contested rarely but hit often. Built on Core::Atomic only, so it is
// available on every platform including the single-threaded web build.
//
// Never hold a SpinLock across a syscall, allocation, or callback.
//
class SpinLock
{
public:
    SpinLock() = default;

    SpinLock(const SpinLock&) = delete;
    SpinLock& operator=(const SpinLock&) = delete;

    void Lock()
    {
        // Fast path uncontended, then pause-loop while the flag looks taken,
        // then the full acquire exchange again (test-and-test-and-set).
        while (mFlag.ExchangeAcqRel(1) == 1)
        {
            while (mFlag.LoadRelaxed() == 1)
            {
                CpuPause();
            }
        }
    }

    void Unlock() { mFlag.StoreRelease(0); }

    bool TryLock() { return mFlag.ExchangeAcqRel(1) == 0; }

private:
    static void CpuPause()
    {
#if defined(_M_X64) || defined(_M_IX86) || defined(__x86_64__) || defined(__i386__)
    #if defined(_MSC_VER)
        __asm pause;
    #else
        __builtin_ia32_pause();
    #endif
#elif defined(__aarch64__) || defined(__arm__)
    asm volatile("yield" ::: "memory");
#endif
        // Other arches (wasm): plain loop, contention is rare.
    }

    Atomic<u32> mFlag{0};
};

//
// RAII guard for SpinLock.
//
class ScopedSpinLock
{
public:
    explicit ScopedSpinLock(SpinLock& lock)
        : mLock(&lock)
    {
        mLock->Lock();
    }

    ~ScopedSpinLock() { mLock->Unlock(); }

    ScopedSpinLock(const ScopedSpinLock&) = delete;
    ScopedSpinLock& operator=(const ScopedSpinLock&) = delete;

private:
    SpinLock* mLock;
};

} // namespace Aether::Platform
