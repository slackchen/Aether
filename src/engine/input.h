#pragma once

#include "core/platform.h"
#include "core/math.h"

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

enum class MouseButton {
    Left = 0,
    Right = 1,
    Middle = 2,
};

enum class KeyCode {
    Up, Down, Left, Right,
    A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9, Num0,
    Space, Shift, Ctrl, Alt, Tab, Escape, Enter, Backspace, Delete,
    Count
};

class Input {
public:
    static void init();

    // Legacy Key API
    static bool is_down(Key key);
    static bool was_pressed(Key key);
    static bool was_released(Key key);

    // Extended Key API
    static bool is_key_down(KeyCode code);
    static bool was_key_pressed(KeyCode code);
    static bool was_key_released(KeyCode code);

    // Mouse API
    static Vec2 mouse_pos();
    static Vec2 mouse_delta();
    static f32 mouse_wheel();
    static bool is_mouse_down(MouseButton btn);
    static bool was_mouse_pressed(MouseButton btn);
    static bool was_mouse_released(MouseButton btn);

    // Modifiers
    static bool is_shift_down();
    static bool is_ctrl_down();
    static bool is_alt_down();

    static void update();
    static bool any_gesture();
};

}

