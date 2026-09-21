#pragma once

#include "Core.h"

namespace Aether::Engine {

class Timer
{
public:
    Timer();

    void Update();

    f32 Delta() const { return mDelta * mTimeScale; }
    f32 UnscaledDelta() const { return mDelta; }
    f32 Time() const { return mTime; }
    f32 TimeScale() const { return mTimeScale; }
    f32 TargetTimeScale() const { return mTargetScale; }

    void SetTimeScale(f32 scale) { mTargetScale = scale; }
    void AddHitStop(f32 duration) { mHitStop = duration > mHitStop ? duration : mHitStop; }

private:
    double mLast = 0.0;
    f32 mDelta = 0.0f;
    f32 mTime = 0.0f;
    f32 mTimeScale = 1.0f;
    f32 mTargetScale = 1.0f;
    f32 mHitStop = 0.0f;
};

}
