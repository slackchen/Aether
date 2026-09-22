#pragma once

#include "Core.h"

#include <atomic>

namespace Aether {

//
// Thin wrapper over std::atomic. Core owns this single concurrency primitive
// because the intrusive refcount (RefCounted) sits below every module;
// everything else that touches the OS lives in Platform::Threading. Engine
// code must use Atomic<T> instead of std::atomic directly so the underlying
// implementation can be swapped per platform later.
//
// No implicit conversion / operator overloads: memory ordering is part of the
// call site and must stay visible in the source.
//
template<typename T>
class Atomic
{
public:
    Atomic() = default;

    constexpr Atomic(T value)
        : mValue(value)
    {
    }

    Atomic(const Atomic&) = delete;
    Atomic& operator=(const Atomic&) = delete;

    T LoadRelaxed() const { return mValue.load(std::memory_order_relaxed); }
    T LoadAcquire() const { return mValue.load(std::memory_order_acquire); }
    void StoreRelaxed(T value) { mValue.store(value, std::memory_order_relaxed); }
    void StoreRelease(T value) { mValue.store(value, std::memory_order_release); }

    T ExchangeAcqRel(T value)
    {
        return mValue.exchange(value, std::memory_order_acq_rel);
    }

    T FetchAddRelaxed(T value)
    {
        return mValue.fetch_add(value, std::memory_order_relaxed);
    }

    T FetchAddAcqRel(T value)
    {
        return mValue.fetch_add(value, std::memory_order_acq_rel);
    }

    T FetchSubRelaxed(T value)
    {
        return mValue.fetch_sub(value, std::memory_order_relaxed);
    }

    T FetchSubAcqRel(T value)
    {
        return mValue.fetch_sub(value, std::memory_order_acq_rel);
    }

    // Replaces *this with desired if it still equals expected (updated on
    // failure). Weak variant may fail spuriously; use it inside retry loops.
    bool CompareExchangeStrongAcqRel(T& expected, T desired)
    {
        return mValue.compare_exchange_strong(
            expected, desired, std::memory_order_acq_rel, std::memory_order_acquire);
    }

    bool CompareExchangeWeakAcqRel(T& expected, T desired)
    {
        return mValue.compare_exchange_weak(
            expected, desired, std::memory_order_acq_rel, std::memory_order_acquire);
    }

private:
    std::atomic<T> mValue;
};

//
// Size of a CPU cache line, used to pad shared hot state and align per-thread
// arenas. Platform::CpuInfo::CacheLineSize() reports the real value; this
// constant is the safe architectural fallback (and compile-time pad size).
//
constexpr u32 CACHE_LINE_SIZE = 64;

} // namespace Aether
