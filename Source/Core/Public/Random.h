#pragma once

#include "Core.h"

namespace Aether {

//
// xorshift64* PRNG. Small, fast, deterministic. Replaces std::mt19937 +
// uniform distributions for game purposes.
//
class Random
{
public:
    explicit Random(u64 seed = 0x9e3779b97f4a7c15ull)
    {
        SetSeed(seed);
    }

    void SetSeed(u64 seed)
    {
        mState = seed ? seed : 0x9e3779b97f4a7c15ull;
    }

    u64 NextU64()
    {
        mState ^= mState >> 12;
        mState ^= mState << 25;
        mState ^= mState >> 27;
        return mState * 0x2545f4914f6cdd1dull;
    }

    u32 NextU32() { return (u32)(NextU64() >> 32); }

    // Uniform float in [0, 1).
    f32 NextF32() { return (f32)(NextU64() >> 40) * (1.0f / 16777216.0f); }

    // Uniform double in [0, 1).
    f64 NextF64() { return (f64)(NextU64() >> 11) * (1.0 / 9007199254740992.0); }

    // Uniform integer in [lo, hi] (inclusive).
    i32 Range(i32 lo, i32 hi)
    {
        AETHER_ASSERT(lo <= hi);
        return lo + (i32)(NextU64() % (u64)(hi - lo + 1));
    }

    i64 Range(i64 lo, i64 hi)
    {
        AETHER_ASSERT(lo <= hi);
        return lo + (i64)(NextU64() % (u64)(hi - lo + 1));
    }

    // Uniform float in [lo, hi).
    f32 Range(f32 lo, f32 hi)
    {
        AETHER_ASSERT(lo <= hi);
        return lo + (hi - lo) * NextF32();
    }

    f64 Range(f64 lo, f64 hi)
    {
        AETHER_ASSERT(lo <= hi);
        return lo + (hi - lo) * NextF64();
    }

private:
    u64 mState;
};

}
