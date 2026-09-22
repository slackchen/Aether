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
    Count
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

//
// Immutable input state for one frame, captured at frame start by
// Input::CaptureSnapshot. Safe to read from any thread while the frame's
// systems run; mirrors the legacy Input query API.
//
struct InputSnapshot
{
    bool KeyDown[(u32)KeyCode::Count] = {};
    bool KeyPressed[(u32)KeyCode::Count] = {};
    bool KeyReleased[(u32)KeyCode::Count] = {};

    bool AbstractDown[(u32)Key::Count] = {};
    bool AbstractPressed[(u32)Key::Count] = {};
    bool AbstractReleased[(u32)Key::Count] = {};

    Math::Vec2 MousePos{640.0f, 360.0f};
    Math::Vec2 MouseDelta{0.0f, 0.0f};
    f32 MouseWheel = 0.0f;
    bool MouseDown[3] = {};
    bool MousePressed[3] = {};
    bool MouseReleased[3] = {};

    bool IsDown(Key key) const { return AbstractDown[(u32)key]; }
    bool WasPressed(Key key) const { return AbstractPressed[(u32)key]; }
    bool WasReleased(Key key) const { return AbstractReleased[(u32)key]; }

    bool IsKeyDown(KeyCode code) const { return KeyDown[(u32)code]; }
    bool WasKeyPressed(KeyCode code) const { return KeyPressed[(u32)code]; }
    bool WasKeyReleased(KeyCode code) const { return KeyReleased[(u32)code]; }

    Math::Vec2 GetMousePos() const { return MousePos; }
    Math::Vec2 GetMouseDelta() const { return MouseDelta; }
    f32 GetMouseWheel() const { return MouseWheel; }
    bool IsMouseDown(MouseButton btn) const { return MouseDown[(u32)btn]; }
    bool WasMousePressed(MouseButton btn) const { return MousePressed[(u32)btn]; }
    bool WasMouseReleased(MouseButton btn) const { return MouseReleased[(u32)btn]; }

    bool IsShiftDown() const { return IsKeyDown(KeyCode::Shift); }
    bool IsCtrlDown() const { return IsKeyDown(KeyCode::Ctrl); }
    bool IsAltDown() const { return IsKeyDown(KeyCode::Alt); }

    bool AnyGesture() const
    {
        for (u32 k = 0; k < (u32)Key::Count; k++)
        {
            if (AbstractDown[k]) return true;
        }
        for (u32 b = 0; b < 3; b++)
        {
            if (MouseDown[b]) return true;
        }
        return false;
    }
};

class Input
{
public:
    static void Init();

    // Captures native state into an immutable per-frame snapshot. Called by
    // EngineLoop at frame start; also refreshes the legacy query state so
    // unmigrated code keeps working.
    static void CaptureSnapshot(InputSnapshot& out);

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
