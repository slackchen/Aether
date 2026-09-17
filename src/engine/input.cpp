#include "engine/input.h"
#include "platform/platform.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/em_js.h>
#else
#include <windows.h>
#include <xinput.h>
#endif

namespace aether::engine {

#ifdef __EMSCRIPTEN__

namespace {
const char* key_code_str(Key key) {
    switch (key) {
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

const char* keycode_to_str(KeyCode code) {
    switch (code) {
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

EM_JS(void, js_input_init, (), {
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

EM_JS(void, js_input_poll, (), {
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

EM_JS(int, js_input_state, (const char* map, const char* code), {
    var s = Module.__aether_input[UTF8ToString(map)];
    return (s && s[UTF8ToString(code)]) ? 1 : 0;
});

EM_JS(float, js_get_mouse_x, (), { return Module.__aether_input ? Module.__aether_input.mouseX : 640; });
EM_JS(float, js_get_mouse_y, (), { return Module.__aether_input ? Module.__aether_input.mouseY : 360; });
EM_JS(float, js_get_mouse_dx, (), {
    var dx = Module.__aether_input ? Module.__aether_input.mouseDeltaX : 0;
    if (Module.__aether_input) Module.__aether_input.mouseDeltaX = 0;
    return dx;
});
EM_JS(float, js_get_mouse_dy, (), {
    var dy = Module.__aether_input ? Module.__aether_input.mouseDeltaY : 0;
    if (Module.__aether_input) Module.__aether_input.mouseDeltaY = 0;
    return dy;
});
EM_JS(float, js_get_mouse_wheel, (), {
    var w = Module.__aether_input ? Module.__aether_input.mouseWheel : 0;
    if (Module.__aether_input) Module.__aether_input.mouseWheel = 0;
    return w;
});
EM_JS(int, js_get_mouse_down, (int btn), { return (Module.__aether_input && Module.__aether_input.mouseDown[btn]) ? 1 : 0; });
EM_JS(int, js_get_mouse_pressed, (int btn), { return (Module.__aether_input && Module.__aether_input.mousePressed[btn]) ? 1 : 0; });
EM_JS(int, js_get_mouse_released, (int btn), { return (Module.__aether_input && Module.__aether_input.mouseReleased[btn]) ? 1 : 0; });

EM_JS(int, js_input_any_gesture, (), {
    for (var c in Module.__aether_input.down) {
        if (Module.__aether_input.down[c]) return 1;
    }
    for (var b = 0; b < 3; b++) {
        if (Module.__aether_input.mouseDown[b]) return 1;
    }
    return 0;
});

EM_JS(void, js_input_update, (), {
    Module.__aether_input.pressed = {};
    Module.__aether_input.released = {};
});

#else

namespace {

constexpr u32 kLegacyKeyCount = 9;
constexpr u32 kKeyCodeCount = (u32)KeyCode::Count;

const int kLegacyVkList[kLegacyKeyCount][2] = {
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

int keycode_to_vk(KeyCode code) {
    switch (code) {
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

struct NativeInputState {
    bool legacy_down[kLegacyKeyCount] = {};
    bool legacy_pressed[kLegacyKeyCount] = {};
    bool legacy_released[kLegacyKeyCount] = {};
    bool legacy_prev[kLegacyKeyCount] = {};

    bool key_down[kKeyCodeCount] = {};
    bool key_pressed[kKeyCodeCount] = {};
    bool key_released[kKeyCodeCount] = {};
    bool key_prev[kKeyCodeCount] = {};

    Vec2 mouse_pos{640.0f, 360.0f};
    Vec2 mouse_prev_pos{640.0f, 360.0f};
    Vec2 mouse_delta{0.0f, 0.0f};
    f32 mouse_wheel = 0.0f;
    bool mouse_down[3] = {};
    bool mouse_pressed[3] = {};
    bool mouse_released[3] = {};
    bool mouse_prev[3] = {};
};

NativeInputState g_state;

bool vk_down(int vk) {
    if (vk <= 0) return false;
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

bool window_focused() {
    HWND foreground = GetForegroundWindow();
    HWND ours = (HWND)aether::platform::native_window_handle();
    if (!ours) return false;
    HWND active = GetActiveWindow();
    return foreground == ours || active == ours || (GetFocus() != nullptr && GetAncestor(GetFocus(), GA_ROOT) == ours);
}

void poll_keyboard() {
    if (!window_focused()) return;

    for (u32 k = 0; k < kLegacyKeyCount; k++) {
        bool on = false;
        for (int i = 0; i < 2; i++) {
            if (vk_down(kLegacyVkList[k][i])) on = true;
        }
        g_state.legacy_pressed[k] = on && !g_state.legacy_prev[k];
        g_state.legacy_released[k] = !on && g_state.legacy_prev[k];
        g_state.legacy_down[k] = on;
        g_state.legacy_prev[k] = on;
    }

    for (u32 k = 0; k < kKeyCodeCount; k++) {
        int vk = keycode_to_vk((KeyCode)k);
        bool on = vk_down(vk);
        g_state.key_pressed[k] = on && !g_state.key_prev[k];
        g_state.key_released[k] = !on && g_state.key_prev[k];
        g_state.key_down[k] = on;
        g_state.key_prev[k] = on;
    }
}

void poll_mouse() {
    HWND hwnd = (HWND)aether::platform::native_window_handle();
    if (!hwnd || !window_focused()) return;

    POINT pt;
    if (GetCursorPos(&pt) && ScreenToClient(hwnd, &pt)) {
        RECT rc;
        GetClientRect(hwnd, &rc);
        f32 client_w = (f32)(rc.right - rc.left);
        f32 client_h = (f32)(rc.bottom - rc.top);
        if (client_w > 0.0f && client_h > 0.0f) {
            f32 mx = (f32)pt.x * 1280.0f / client_w;
            f32 my = (f32)pt.y * 720.0f / client_h;
            g_state.mouse_delta = {mx - g_state.mouse_pos.x, my - g_state.mouse_pos.y};
            g_state.mouse_pos = {mx, my};
        }
    }

    g_state.mouse_wheel = aether::platform::get_and_reset_mouse_wheel();

    bool mdown[3] = {
        vk_down(VK_LBUTTON),
        vk_down(VK_RBUTTON),
        vk_down(VK_MBUTTON),
    };
    for (int i = 0; i < 3; i++) {
        g_state.mouse_pressed[i] = mdown[i] && !g_state.mouse_prev[i];
        g_state.mouse_released[i] = !mdown[i] && g_state.mouse_prev[i];
        g_state.mouse_down[i] = mdown[i];
        g_state.mouse_prev[i] = mdown[i];
    }
}

void poll_gamepad() {
    XINPUT_STATE state;
    if (XInputGetState(0, &state) != ERROR_SUCCESS) return;

    const XINPUT_GAMEPAD& g = state.Gamepad;
    const auto btn = [&g](WORD mask) { return (g.wButtons & mask) != 0; };

    f32 lx = (f32)g.sThumbLX / 32767.0f;
    f32 ly = (f32)g.sThumbLY / 32767.0f;
    if (lx > 0.25f) g_state.legacy_down[(u32)Key::Right] = true;
    else if (lx < -0.25f) g_state.legacy_down[(u32)Key::Left] = true;
    if (ly > 0.25f) g_state.legacy_down[(u32)Key::Up] = true;
    else if (ly < -0.25f) g_state.legacy_down[(u32)Key::Down] = true;

    if (btn(XINPUT_GAMEPAD_DPAD_UP)) g_state.legacy_down[(u32)Key::Up] = true;
    if (btn(XINPUT_GAMEPAD_DPAD_DOWN)) g_state.legacy_down[(u32)Key::Down] = true;
    if (btn(XINPUT_GAMEPAD_DPAD_LEFT)) g_state.legacy_down[(u32)Key::Left] = true;
    if (btn(XINPUT_GAMEPAD_DPAD_RIGHT)) g_state.legacy_down[(u32)Key::Right] = true;

    bool fire = btn(XINPUT_GAMEPAD_A) || btn(XINPUT_GAMEPAD_X) || g.bRightTrigger > 128;
    bool slow = btn(XINPUT_GAMEPAD_B) || btn(XINPUT_GAMEPAD_LEFT_SHOULDER) || g.bLeftTrigger > 128;
    if (fire) g_state.legacy_down[(u32)Key::Fire] = true;
    if (slow) g_state.legacy_down[(u32)Key::Slow] = true;
    if (btn(XINPUT_GAMEPAD_START)) {
        g_state.legacy_down[(u32)Key::Pause] = true;
        g_state.legacy_down[(u32)Key::Confirm] = true;
    }
    if (btn(XINPUT_GAMEPAD_A)) g_state.legacy_down[(u32)Key::Confirm] = true;
}

}

#endif

void Input::init() {
#ifdef __EMSCRIPTEN__
    js_input_init();
#endif
}

bool Input::is_down(Key key) {
#ifdef __EMSCRIPTEN__
    if (key == Key::Fire) {
        return js_input_state("down", "KeyJ") != 0 || js_input_state("down", "Space") != 0;
    }
    return js_input_state("down", key_code_str(key)) != 0;
#else
    return g_state.legacy_down[(u32)key];
#endif
}

bool Input::was_pressed(Key key) {
#ifdef __EMSCRIPTEN__
    if (key == Key::Fire) {
        return js_input_state("pressed", "KeyJ") != 0 || js_input_state("pressed", "Space") != 0;
    }
    return js_input_state("pressed", key_code_str(key)) != 0;
#else
    return g_state.legacy_pressed[(u32)key];
#endif
}

bool Input::was_released(Key key) {
#ifdef __EMSCRIPTEN__
    if (key == Key::Fire) {
        return js_input_state("released", "KeyJ") != 0 || js_input_state("released", "Space") != 0;
    }
    return js_input_state("released", key_code_str(key)) != 0;
#else
    return g_state.legacy_released[(u32)key];
#endif
}

bool Input::is_key_down(KeyCode code) {
#ifdef __EMSCRIPTEN__
    return js_input_state("down", keycode_to_str(code)) != 0;
#else
    return g_state.key_down[(u32)code];
#endif
}

bool Input::was_key_pressed(KeyCode code) {
#ifdef __EMSCRIPTEN__
    return js_input_state("pressed", keycode_to_str(code)) != 0;
#else
    return g_state.key_pressed[(u32)code];
#endif
}

bool Input::was_key_released(KeyCode code) {
#ifdef __EMSCRIPTEN__
    return js_input_state("released", keycode_to_str(code)) != 0;
#else
    return g_state.key_released[(u32)code];
#endif
}

Vec2 Input::mouse_pos() {
#ifdef __EMSCRIPTEN__
    return {js_get_mouse_x(), js_get_mouse_y()};
#else
    return g_state.mouse_pos;
#endif
}

Vec2 Input::mouse_delta() {
#ifdef __EMSCRIPTEN__
    return {js_get_mouse_dx(), js_get_mouse_dy()};
#else
    return g_state.mouse_delta;
#endif
}

f32 Input::mouse_wheel() {
#ifdef __EMSCRIPTEN__
    return js_get_mouse_wheel();
#else
    return g_state.mouse_wheel;
#endif
}

bool Input::is_mouse_down(MouseButton btn) {
#ifdef __EMSCRIPTEN__
    return js_get_mouse_down((int)btn) != 0;
#else
    return g_state.mouse_down[(int)btn];
#endif
}

bool Input::was_mouse_pressed(MouseButton btn) {
#ifdef __EMSCRIPTEN__
    return js_get_mouse_pressed((int)btn) != 0;
#else
    return g_state.mouse_pressed[(int)btn];
#endif
}

bool Input::was_mouse_released(MouseButton btn) {
#ifdef __EMSCRIPTEN__
    return js_get_mouse_released((int)btn) != 0;
#else
    return g_state.mouse_released[(int)btn];
#endif
}

bool Input::is_shift_down() {
    return is_key_down(KeyCode::Shift);
}

bool Input::is_ctrl_down() {
    return is_key_down(KeyCode::Ctrl);
}

bool Input::is_alt_down() {
    return is_key_down(KeyCode::Alt);
}

bool Input::any_gesture() {
#ifdef __EMSCRIPTEN__
    return js_input_any_gesture() != 0;
#else
    for (u32 k = 0; k < kLegacyKeyCount; k++) {
        if (g_state.legacy_down[k]) return true;
    }
    for (int i = 0; i < 3; i++) {
        if (g_state.mouse_down[i]) return true;
    }
    return false;
#endif
}

void Input::update() {
#ifdef __EMSCRIPTEN__
    js_input_update();
    js_input_poll();
#else
    g_state.mouse_delta = {0.0f, 0.0f};
    poll_keyboard();
    poll_mouse();
    poll_gamepad();
#endif
}

}
