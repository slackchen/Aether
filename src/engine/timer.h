#pragma once

#include "core/platform.h"

namespace aether::engine {

class Timer {
public:
    Timer();

    void update();

    f32 delta() const { return delta_ * time_scale_; }
    f32 unscaled_delta() const { return delta_; }
    f32 time() const { return time_; }
    f32 time_scale() const { return time_scale_; }
    f32 target_time_scale() const { return target_scale_; }

    void set_time_scale(f32 scale) { target_scale_ = scale; }
    void add_hit_stop(f32 duration) { hit_stop_ = duration > hit_stop_ ? duration : hit_stop_; }

private:
    double last_ = 0.0;
    f32 delta_ = 0.0f;
    f32 time_ = 0.0f;
    f32 time_scale_ = 1.0f;
    f32 target_scale_ = 1.0f;
    f32 hit_stop_ = 0.0f;
};

}
