#pragma once

#include "core/platform.h"

namespace aether::engine {

enum class Key {
    Up,
    Down,
    Left,
    Right,
    Fire,
    Slow,
    Pause,
    Confirm,
    Restart,
};

class Input {
public:
    static void init();

    static bool is_down(Key key);
    static bool was_pressed(Key key);
    static bool was_released(Key key);

    static void update();
    static bool any_gesture();
};

}
