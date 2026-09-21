#pragma once

#include "Core.h"
#include "Math/Vec2.h"

namespace Aether::Engine {

enum class Key
{
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

enum class MouseButton
{
    Left = 0,
    Right = 1,
    Middle = 2,
};

enum class KeyCode
{
    Up, Down, Left, Right,
    A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9, Num0,
    Space, Shift, Ctrl, Alt, Tab, Escape, Enter, Backspace, Delete,
    Count
};

class Input
{
public:
    static void Init();

    // Legacy Key API
    static bool IsDown(Key key);
    static bool WasPressed(Key key);
    static bool WasReleased(Key key);

    // Extended Key API
    static bool IsKeyDown(KeyCode code);
    static bool WasKeyPressed(KeyCode code);
    static bool WasKeyReleased(KeyCode code);

    // Mouse API
    static Math::Vec2 MousePos();
    static Math::Vec2 MouseDelta();
    static f32 MouseWheel();
    static bool IsMouseDown(MouseButton btn);
    static bool WasMousePressed(MouseButton btn);
    static bool WasMouseReleased(MouseButton btn);

    // Modifiers
    static bool IsShiftDown();
    static bool IsCtrlDown();
    static bool IsAltDown();

    static void Update();
    static bool AnyGesture();
};

}
