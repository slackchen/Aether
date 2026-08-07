#include "engine/timer.h"
#include "platform/platform.h"

namespace aether::engine {

namespace {
double now_seconds() {
    return aether::platform::now_seconds();
}
}

Timer::Timer() {
    last_ = now_seconds();
}

void Timer::update() {
    double now = now_seconds();
    f32 raw = (f32)(now - last_);
    last_ = now;
    if (raw < 0.0f) raw = 0.0f;
    if (raw > 0.1f) raw = 0.1f;
    delta_ = raw;

    if (hit_stop_ > 0.0f) {
        hit_stop_ -= raw;
        if (hit_stop_ < 0.0f) hit_stop_ = 0.0f;
        time_scale_ = 0.0f;
    } else {
        time_scale_ = target_scale_;
    }

    time_ += delta_ * time_scale_;
}

}
