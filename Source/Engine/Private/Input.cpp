#include "Input.h"
#include "Platform.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/em_js.h>
#else
#include <windows.h>
#include <xinput.h>
#endif

namespace Aether::Engine {

#ifdef __EMSCRIPTEN__

namespace
{
const char* LegacyKeyToStr(Key key)
{
    switch (key)
    {
        case Key::Up: return "ArrowUp";
        case Key::Down: return "ArrowDown";
        case Key::Left: return "ArrowLeft";
        case Key::Right: return "ArrowRight";
        case Key::Fire: return "KeyJ";
        case Key::Slow: return "ShiftLeft";
        case Key::Pause: return "KeyP";
        case Key::Confirm: return "Enter";
        case Key::Restart: return "KeyR";
        default: return "";
    }
}

const char* KeyCodeToStr(KeyCode code)
{
    switch (code)
    {
        case KeyCode::Up: return "ArrowUp";
        case KeyCode::Down: return "ArrowDown";
        case KeyCode::Left: return "ArrowLeft";
        case KeyCode::Right: return "ArrowRight";
        case KeyCode::A: return "KeyA";
        case KeyCode::B: return "KeyB";
        case KeyCode::C: return "KeyC";
        case KeyCode::D: return "KeyD";
        case KeyCode::E: return "KeyE";
        case KeyCode::F: return "KeyF";
        case KeyCode::G: return "KeyG";
        case KeyCode::H: return "KeyH";
        case KeyCode::I: return "KeyI";
        case KeyCode::J: return "KeyJ";
        case KeyCode::K: return "KeyK";
        case KeyCode::L: return "KeyL";
        case KeyCode::M: return "KeyM";
        case KeyCode::N: return "KeyN";
        case KeyCode::O: return "KeyO";
        case KeyCode::P: return "KeyP";
        case KeyCode::Q: return "KeyQ";
        case KeyCode::R: return "KeyR";
        case KeyCode::S: return "KeyS";
        case KeyCode::T: return "KeyT";
        case KeyCode::U: return "KeyU";
        case KeyCode::V: return "KeyV";
        case KeyCode::W: return "KeyW";
        case KeyCode::X: return "KeyX";
        case KeyCode::Y: return "KeyY";
        case KeyCode::Z: return "KeyZ";
        case KeyCode::Num1: return "Digit1";
        case KeyCode::Num2: return "Digit2";
        case KeyCode::Num3: return "Digit3";
        case KeyCode::Num4: return "Digit4";
        case KeyCode::Num5: return "Digit5";
        case KeyCode::Num6: return "Digit6";
        case KeyCode::Num7: return "Digit7";
        case KeyCode::Num8: return "Digit8";
        case KeyCode::Num9: return "Digit9";
        case KeyCode::Num0: return "Digit0";
        case KeyCode::Space: return "Space";
        case KeyCode::Shift: return "ShiftLeft";
        case KeyCode::Ctrl: return "ControlLeft";
        case KeyCode::Alt: return "AltLeft";
        case KeyCode::Tab: return "Tab";
        case KeyCode::Escape: return "Escape";
        case KeyCode::Enter: return "Enter";
        case KeyCode::Backspace: return "Backspace";
        case KeyCode::Delete: return "Delete";
        default: return "";
    }
}
}

EM_JS(void, JsInputInit, (), {
    if (Module.__aether_input) return;
    var input = {
        keys: {},
        down: {},
        pressed: {},
        released: {},
        prev: {},
        mouseX: 640,
        mouseY: 360,
        mouseDeltaX: 0,
        mouseDeltaY: 0,
        mouseWheel: 0,
        mouseDown: [false, false, false],
        mousePressed: [false, false, false],
        mouseReleased: [false, false, false],
        mousePrev: [false, false, false]
    };
    Module.__aether_input = input;

    window.addEventListener("keydown", function(e) {
        input.keys[e.code] = true;
        if (e.code === "Space" || e.code === "Tab" || e.code.startsWith("Arrow")) {
            e.preventDefault();
        }
    });
    window.addEventListener("keyup", function(e) {
        input.keys[e.code] = false;
    });

    var canvas = document.getElementById("canvas") || document.querySelector("canvas");
    if (canvas) {
        canvas.addEventListener("mousemove", function(e) {
            var rect = canvas.getBoundingClientRect();
            var x = (e.clientX - rect.left) * (1280 / rect.width);
            var y = (e.clientY - rect.top) * (720 / rect.height);
            input.mouseDeltaX += (x - input.mouseX);
            input.mouseDeltaY += (y - input.mouseY);
            input.mouseX = x;
            input.mouseY = y;
        });
        canvas.addEventListener("mousedown", function(e) {
            if (e.button >= 0 && e.button < 3) {
                input.mouseDown[e.button] = true;
            }
        });
        canvas.addEventListener("mouseup", function(e) {
            if (e.button >= 0 && e.button < 3) {
                input.mouseDown[e.button] = false;
            }
        });
        canvas.addEventListener("wheel", function(e) {
            input.mouseWheel += (e.deltaY < 0 ? 1.0 : -1.0);
            e.preventDefault();
        }, { passive: false });
        canvas.addEventListener("contextmenu", function(e) {
            e.preventDefault();
        });
    }
});

EM_JS(void, JsInputPoll, (), {
    var input = Module.__aether_input;
    for (var c in input.keys) {
        var on = !!input.keys[c];
        var was = !!input.prev[c];
        input.pressed[c] = on && !was;
        input.released[c] = !on && was;
        input.down[c] = on;
        input.prev[c] = on;
    }
    for (var b = 0; b < 3; b++) {
        var down = input.mouseDown[b];
        var prev = input.mousePrev[b];
        input.mousePressed[b] = down && !prev;
        input.mouseReleased[b] = !down && prev;
        input.mousePrev[b] = down;
    }
});

EM_JS(int, JsInputState, (const char* map, const char* code), {
    var s = Module.__aether_input[UTF8ToString(map)];
    return (s && s[UTF8ToString(code)]) ? 1 : 0;
});

EM_JS(float, JsGetMouseX, (), { return Module.__aether_input ? Module.__aether_input.mouseX : 640; });
EM_JS(float, JsGetMouseY, (), { return Module.__aether_input ? Module.__aether_input.mouseY : 360; });
EM_JS(float, JsGetMouseDx, (), {
    var dx = Module.__aether_input ? Module.__aether_input.mouseDeltaX : 0;
    if (Module.__aether_input) Module.__aether_input.mouseDeltaX = 0;
    return dx;
});
EM_JS(float, JsGetMouseDy, (), {
    var dy = Module.__aether_input ? Module.__aether_input.mouseDeltaY : 0;
    if (Module.__aether_input) Module.__aether_input.mouseDeltaY = 0;
    return dy;
});
EM_JS(float, JsGetMouseWheel, (), {
    var w = Module.__aether_input ? Module.__aether_input.mouseWheel : 0;
    if (Module.__aether_input) Module.__aether_input.mouseWheel = 0;
    return w;
});
EM_JS(int, JsGetMouseDown, (int btn), { return (Module.__aether_input && Module.__aether_input.mouseDown[btn]) ? 1 : 0; });
EM_JS(int, JsGetMousePressed, (int btn), { return (Module.__aether_input && Module.__aether_input.mousePressed[btn]) ? 1 : 0; });
EM_JS(int, JsGetMouseReleased, (int btn), { return (Module.__aether_input && Module.__aether_input.mouseReleased[btn]) ? 1 : 0; });

EM_JS(int, JsInputAnyGesture, (), {
    for (var c in Module.__aether_input.down) {
        if (Module.__aether_input.down[c]) return 1;
    }
    for (var b = 0; b < 3; b++) {
        if (Module.__aether_input.mouseDown[b]) return 1;
    }
    return 0;
});

EM_JS(void, JsInputUpdate, (), {
    Module.__aether_input.pressed = {};
    Module.__aether_input.released = {};
});

#else

namespace
{

constexpr u32 LEGACY_KEY_COUNT = 9;
constexpr u32 KEY_CODE_COUNT = (u32)KeyCode::Count;

const int LEGACY_VK_LIST[LEGACY_KEY_COUNT][2] = {
    {VK_UP, -1},        // Up
    {VK_DOWN, -1},      // Down
    {VK_LEFT, -1},      // Left
    {VK_RIGHT, -1},     // Right
    {'J', VK_SPACE},    // Fire
    {VK_LSHIFT, -1},    // Slow
    {'P', -1},          // Pause
    {VK_RETURN, -1},    // Confirm
    {'R', -1},          // Restart
};

int KeyCodeToVk(KeyCode code)
{
    switch (code)
    {
        case KeyCode::Up: return VK_UP;
        case KeyCode::Down: return VK_DOWN;
        case KeyCode::Left: return VK_LEFT;
        case KeyCode::Right: return VK_RIGHT;
        case KeyCode::A: return 'A';
        case KeyCode::B: return 'B';
        case KeyCode::C: return 'C';
        case KeyCode::D: return 'D';
        case KeyCode::E: return 'E';
        case KeyCode::F: return 'F';
        case KeyCode::G: return 'G';
        case KeyCode::H: return 'H';
        case KeyCode::I: return 'I';
        case KeyCode::J: return 'J';
        case KeyCode::K: return 'K';
        case KeyCode::L: return 'L';
        case KeyCode::M: return 'M';
        case KeyCode::N: return 'N';
        case KeyCode::O: return 'O';
        case KeyCode::P: return 'P';
        case KeyCode::Q: return 'Q';
        case KeyCode::R: return 'R';
        case KeyCode::S: return 'S';
        case KeyCode::T: return 'T';
        case KeyCode::U: return 'U';
        case KeyCode::V: return 'V';
        case KeyCode::W: return 'W';
        case KeyCode::X: return 'X';
        case KeyCode::Y: return 'Y';
        case KeyCode::Z: return 'Z';
        case KeyCode::Num1: return '1';
        case KeyCode::Num2: return '2';
        case KeyCode::Num3: return '3';
        case KeyCode::Num4: return '4';
        case KeyCode::Num5: return '5';
        case KeyCode::Num6: return '6';
        case KeyCode::Num7: return '7';
        case KeyCode::Num8: return '8';
        case KeyCode::Num9: return '9';
        case KeyCode::Num0: return '0';
        case KeyCode::Space: return VK_SPACE;
        case KeyCode::Shift: return VK_SHIFT;
        case KeyCode::Ctrl: return VK_CONTROL;
        case KeyCode::Alt: return VK_MENU;
        case KeyCode::Tab: return VK_TAB;
        case KeyCode::Escape: return VK_ESCAPE;
        case KeyCode::Enter: return VK_RETURN;
        case KeyCode::Backspace: return VK_BACK;
        case KeyCode::Delete: return VK_DELETE;
        default: return -1;
    }
}

struct NativeInputState
{
    bool LegacyDown[LEGACY_KEY_COUNT] = {};
    bool LegacyPressed[LEGACY_KEY_COUNT] = {};
    bool LegacyReleased[LEGACY_KEY_COUNT] = {};
    bool LegacyPrev[LEGACY_KEY_COUNT] = {};

    bool KeyDown[KEY_CODE_COUNT] = {};
    bool KeyPressed[KEY_CODE_COUNT] = {};
    bool KeyReleased[KEY_CODE_COUNT] = {};
    bool KeyPrev[KEY_CODE_COUNT] = {};

    Math::Vec2 MousePos{640.0f, 360.0f};
    Math::Vec2 MousePrevPos{640.0f, 360.0f};
    Math::Vec2 MouseDelta{0.0f, 0.0f};
    f32 MouseWheel = 0.0f;
    bool MouseDown[3] = {};
    bool MousePressed[3] = {};
    bool MouseReleased[3] = {};
    bool MousePrev[3] = {};
};

NativeInputState gState;

bool VkDown(int vk)
{
    if (vk <= 0) return false;
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

bool WindowFocused()
{
    HWND foreground = GetForegroundWindow();
    HWND ours = (HWND)Aether::Platform::NativeWindowHandle();
    if (!ours) return false;
    HWND active = GetActiveWindow();
    return foreground == ours || active == ours || (GetFocus() != nullptr && GetAncestor(GetFocus(), GA_ROOT) == ours);
}

void PollKeyboard()
{
    if (!WindowFocused()) return;

    for (u32 k = 0; k < LEGACY_KEY_COUNT; k++)
    {
        bool on = false;
        for (int i = 0; i < 2; i++)
        {
            if (VkDown(LEGACY_VK_LIST[k][i])) on = true;
        }
        gState.LegacyPressed[k] = on && !gState.LegacyPrev[k];
        gState.LegacyReleased[k] = !on && gState.LegacyPrev[k];
        gState.LegacyDown[k] = on;
        gState.LegacyPrev[k] = on;
    }

    for (u32 k = 0; k < KEY_CODE_COUNT; k++)
    {
        int vk = KeyCodeToVk((KeyCode)k);
        bool on = VkDown(vk);
        gState.KeyPressed[k] = on && !gState.KeyPrev[k];
        gState.KeyReleased[k] = !on && gState.KeyPrev[k];
        gState.KeyDown[k] = on;
        gState.KeyPrev[k] = on;
    }
}

void PollMouse()
{
    HWND hwnd = (HWND)Aether::Platform::NativeWindowHandle();
    if (!hwnd || !WindowFocused()) return;

    POINT pt;
    if (GetCursorPos(&pt) && ScreenToClient(hwnd, &pt))
    {
        RECT rc;
        GetClientRect(hwnd, &rc);
        f32 clientW = (f32)(rc.right - rc.left);
        f32 clientH = (f32)(rc.bottom - rc.top);
        if (clientW > 0.0f && clientH > 0.0f)
        {
            f32 mx = (f32)pt.x * 1280.0f / clientW;
            f32 my = (f32)pt.y * 720.0f / clientH;
            gState.MouseDelta = {mx - gState.MousePos.x, my - gState.MousePos.y};
            gState.MousePos = {mx, my};
        }
    }

    gState.MouseWheel = Aether::Platform::GetAndResetMouseWheel();

    bool mdown[3] = {
        VkDown(VK_LBUTTON),
        VkDown(VK_RBUTTON),
        VkDown(VK_MBUTTON),
    };
    for (int i = 0; i < 3; i++)
    {
        gState.MousePressed[i] = mdown[i] && !gState.MousePrev[i];
        gState.MouseReleased[i] = !mdown[i] && gState.MousePrev[i];
        gState.MouseDown[i] = mdown[i];
        gState.MousePrev[i] = mdown[i];
    }
}

void PollGamepad()
{
    XINPUT_STATE state;
    if (XInputGetState(0, &state) != ERROR_SUCCESS) return;

    const XINPUT_GAMEPAD& g = state.Gamepad;
    const auto btn = [&g](WORD mask) { return (g.wButtons & mask) != 0; };

    f32 lx = (f32)g.sThumbLX / 32767.0f;
    f32 ly = (f32)g.sThumbLY / 32767.0f;
    if (lx > 0.25f) gState.LegacyDown[(u32)Key::Right] = true;
    else if (lx < -0.25f) gState.LegacyDown[(u32)Key::Left] = true;
    if (ly > 0.25f) gState.LegacyDown[(u32)Key::Up] = true;
    else if (ly < -0.25f) gState.LegacyDown[(u32)Key::Down] = true;

    if (btn(XINPUT_GAMEPAD_DPAD_UP)) gState.LegacyDown[(u32)Key::Up] = true;
    if (btn(XINPUT_GAMEPAD_DPAD_DOWN)) gState.LegacyDown[(u32)Key::Down] = true;
    if (btn(XINPUT_GAMEPAD_DPAD_LEFT)) gState.LegacyDown[(u32)Key::Left] = true;
    if (btn(XINPUT_GAMEPAD_DPAD_RIGHT)) gState.LegacyDown[(u32)Key::Right] = true;

    bool fire = btn(XINPUT_GAMEPAD_A) || btn(XINPUT_GAMEPAD_X) || g.bRightTrigger > 128;
    bool slow = btn(XINPUT_GAMEPAD_B) || btn(XINPUT_GAMEPAD_LEFT_SHOULDER) || g.bLeftTrigger > 128;
    if (fire) gState.LegacyDown[(u32)Key::Fire] = true;
    if (slow) gState.LegacyDown[(u32)Key::Slow] = true;
    if (btn(XINPUT_GAMEPAD_START))
    {
        gState.LegacyDown[(u32)Key::Pause] = true;
        gState.LegacyDown[(u32)Key::Confirm] = true;
    }
    if (btn(XINPUT_GAMEPAD_A)) gState.LegacyDown[(u32)Key::Confirm] = true;
}

}

#endif

void Input::Init()
{
#ifdef __EMSCRIPTEN__
    JsInputInit();
#endif
}

bool Input::IsDown(Key key)
{
#ifdef __EMSCRIPTEN__
    if (key == Key::Fire)
    {
        return JsInputState("down", "KeyJ") != 0 || JsInputState("down", "Space") != 0;
    }
    return JsInputState("down", LegacyKeyToStr(key)) != 0;
#else
    return gState.LegacyDown[(u32)key];
#endif
}

bool Input::WasPressed(Key key)
{
#ifdef __EMSCRIPTEN__
    if (key == Key::Fire)
    {
        return JsInputState("pressed", "KeyJ") != 0 || JsInputState("pressed", "Space") != 0;
    }
    return JsInputState("pressed", LegacyKeyToStr(key)) != 0;
#else
    return gState.LegacyPressed[(u32)key];
#endif
}

bool Input::WasReleased(Key key)
{
#ifdef __EMSCRIPTEN__
    if (key == Key::Fire)
    {
        return JsInputState("released", "KeyJ") != 0 || JsInputState("released", "Space") != 0;
    }
    return JsInputState("released", LegacyKeyToStr(key)) != 0;
#else
    return gState.LegacyReleased[(u32)key];
#endif
}

bool Input::IsKeyDown(KeyCode code)
{
#ifdef __EMSCRIPTEN__
    return JsInputState("down", KeyCodeToStr(code)) != 0;
#else
    return gState.KeyDown[(u32)code];
#endif
}

bool Input::WasKeyPressed(KeyCode code)
{
#ifdef __EMSCRIPTEN__
    return JsInputState("pressed", KeyCodeToStr(code)) != 0;
#else
    return gState.KeyPressed[(u32)code];
#endif
}

bool Input::WasKeyReleased(KeyCode code)
{
#ifdef __EMSCRIPTEN__
    return JsInputState("released", KeyCodeToStr(code)) != 0;
#else
    return gState.KeyReleased[(u32)code];
#endif
}

Math::Vec2 Input::MousePos()
{
#ifdef __EMSCRIPTEN__
    return {JsGetMouseX(), JsGetMouseY()};
#else
    return gState.MousePos;
#endif
}

Math::Vec2 Input::MouseDelta()
{
#ifdef __EMSCRIPTEN__
    return {JsGetMouseDx(), JsGetMouseDy()};
#else
    return gState.MouseDelta;
#endif
}

f32 Input::MouseWheel()
{
#ifdef __EMSCRIPTEN__
    return JsGetMouseWheel();
#else
    return gState.MouseWheel;
#endif
}

bool Input::IsMouseDown(MouseButton btn)
{
#ifdef __EMSCRIPTEN__
    return JsGetMouseDown((int)btn) != 0;
#else
    return gState.MouseDown[(int)btn];
#endif
}

bool Input::WasMousePressed(MouseButton btn)
{
#ifdef __EMSCRIPTEN__
    return JsGetMousePressed((int)btn) != 0;
#else
    return gState.MousePressed[(int)btn];
#endif
}

bool Input::WasMouseReleased(MouseButton btn)
{
#ifdef __EMSCRIPTEN__
    return JsGetMouseReleased((int)btn) != 0;
#else
    return gState.MouseReleased[(int)btn];
#endif
}

bool Input::IsShiftDown()
{
    return IsKeyDown(KeyCode::Shift);
}

bool Input::IsCtrlDown()
{
    return IsKeyDown(KeyCode::Ctrl);
}

bool Input::IsAltDown()
{
    return IsKeyDown(KeyCode::Alt);
}

bool Input::AnyGesture()
{
#ifdef __EMSCRIPTEN__
    return JsInputAnyGesture() != 0;
#else
    for (u32 k = 0; k < LEGACY_KEY_COUNT; k++)
    {
        if (gState.LegacyDown[k]) return true;
    }
    for (int i = 0; i < 3; i++)
    {
        if (gState.MouseDown[i]) return true;
    }
    return false;
#endif
}

void Input::Update()
{
#ifdef __EMSCRIPTEN__
    JsInputUpdate();
    JsInputPoll();
#else
    gState.MouseDelta = {0.0f, 0.0f};
    PollKeyboard();
    PollMouse();
    PollGamepad();
#endif
}

}
