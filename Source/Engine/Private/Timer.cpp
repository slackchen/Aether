#include "Timer.h"
#include "Platform.h"

namespace Aether::Engine {

Timer::Timer()
{
    mLast = Aether::Platform::NowSeconds();
}

void Timer::Update()
{
    double now = Aether::Platform::NowSeconds();
    f32 raw = (f32)(now - mLast);
    mLast = now;
    if (raw < 0.0f) raw = 0.0f;
    if (raw > 0.1f) raw = 0.1f;
    mDelta = raw;

    if (mHitStop > 0.0f)
    {
        mHitStop -= raw;
        if (mHitStop < 0.0f) mHitStop = 0.0f;
        mTimeScale = 0.0f;
    }
    else
    {
        mTimeScale = mTargetScale;
    }

    mTime += mDelta * mTimeScale;
}

}
