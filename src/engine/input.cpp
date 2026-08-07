#include "engine/input.h"

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
const char* key_code(Key key) {
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
}

EM_JS(void, js_input_init, (), {
    if (Module.__aether_input) return;
    var input = {
        keys: {},       // raw keyboard down state (by code)
        down: {},       // merged keyboard+gamepad down state
        pressed: {},    // merged edge: became down this frame
        released: {},   // merged edge: became up this frame
        prev: {}        // down state at end of previous frame
    };
    Module.__aether_input = input;
    document.addEventListener("keydown", function(e) {
        var code = e.code;
        input.keys[code] = true;
        if (code === "KeyJ" || code === "Space" || code === "ArrowUp" ||
            code === "ArrowDown" || code === "ArrowLeft" || code === "ArrowRight") {
            e.preventDefault();
        }
    });
    document.addEventListener("keyup", function(e) {
        input.keys[e.code] = false;
    });
});

EM_JS(void, js_input_poll, (), {
    var input = Module.__aether_input;
    var padDown = {};
    function set(code) { padDown[code] = true; }
    function axis(codeNeg, codePos, v) {
        if (v > 0.25) set(codePos);
        else if (v < -0.25) set(codeNeg);
    }
    var pads = (navigator.getGamepads ? navigator.getGamepads() : []) || [];
    for (var i = 0; i < pads.length; i++) {
        var gp = pads[i];
        if (!gp || !gp.connected) continue;
        var b = gp.buttons || [];
        var a = gp.axes || [];
        var button = function(n) { return b[n] && b[n].pressed ? true : false; };
        if (a.length >= 2) {
            axis("ArrowLeft", "ArrowRight", a[0]);
            axis("ArrowUp", "ArrowDown", a[1]);
        }
        if (button(12)) set("ArrowUp");
        if (button(13)) set("ArrowDown");
        if (button(14)) set("ArrowLeft");
        if (button(15)) set("ArrowRight");
        if (button(0) || button(2) || button(7)) set("KeyJ");
        if (button(1) || button(4) || button(6)) set("ShiftLeft");
        if (button(9)) set("KeyP");
        if (button(9) || button(0)) set("Enter");
    }
    var codes = {};
    for (var c in input.keys) codes[c] = 1;
    for (var c in padDown) codes[c] = 1;
    for (var c in input.prev) codes[c] = 1;
    for (var c in codes) {
        var on = !!(input.keys[c] || padDown[c]);
        var was = !!input.prev[c];
        if (on && !was) input.pressed[c] = true;
        if (!on && was) input.released[c] = true;
        input.down[c] = on;
        input.prev[c] = on;
    }
});

EM_JS(int, js_input_state, (const char* map, const char* code), {
    var s = Module.__aether_input[UTF8ToString(map)];
    return (s && s[UTF8ToString(code)]) ? 1 : 0;
});

EM_JS(int, js_input_any_gesture, (), {
    for (var c in Module.__aether_input.down) {
        if (Module.__aether_input.down[c]) return 1;
    }
    return 0;
});

EM_JS(void, js_input_update, (), {
    Module.__aether_input.pressed = {};
    Module.__aether_input.released = {};
});

#else

namespace {

constexpr u32 kKeyCount = 9;

const u32 kKeyIndex(Key key) {
    return (u32)key;
}

const int kVkList[kKeyCount][2] = {
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

struct NativeInputState {
    bool down[kKeyCount] = {};
    bool pressed[kKeyCount] = {};
    bool released[kKeyCount] = {};
    bool prev[kKeyCount] = {};
};

NativeInputState g_state;

bool vk_down(int vk) {
    if (vk <= 0) return false;
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

void poll_keyboard(bool keys[kKeyCount]) {
    for (u32 k = 0; k < kKeyCount; k++) {
        for (int i = 0; i < 2; i++) {
            if (vk_down(kVkList[k][i])) {
                keys[k] = true;
            }
        }
    }
}

void poll_gamepad(bool keys[kKeyCount]) {
    XINPUT_STATE state;
    if (XInputGetState(0, &state) != ERROR_SUCCESS) return;

    const XINPUT_GAMEPAD& g = state.Gamepad;
    const auto btn = [&g](WORD mask) { return (g.wButtons & mask) != 0; };

    f32 lx = (f32)g.sThumbLX / 32767.0f;
    f32 ly = (f32)g.sThumbLY / 32767.0f;
    if (lx > 0.25f) keys[kKeyIndex(Key::Right)] = true;
    else if (lx < -0.25f) keys[kKeyIndex(Key::Left)] = true;
    if (ly > 0.25f) keys[kKeyIndex(Key::Up)] = true;
    else if (ly < -0.25f) keys[kKeyIndex(Key::Down)] = true;

    if (btn(XINPUT_GAMEPAD_DPAD_UP)) keys[kKeyIndex(Key::Up)] = true;
    if (btn(XINPUT_GAMEPAD_DPAD_DOWN)) keys[kKeyIndex(Key::Down)] = true;
    if (btn(XINPUT_GAMEPAD_DPAD_LEFT)) keys[kKeyIndex(Key::Left)] = true;
    if (btn(XINPUT_GAMEPAD_DPAD_RIGHT)) keys[kKeyIndex(Key::Right)] = true;

    bool fire = btn(XINPUT_GAMEPAD_A) || btn(XINPUT_GAMEPAD_X) || g.bRightTrigger > 128;
    bool slow = btn(XINPUT_GAMEPAD_B) || btn(XINPUT_GAMEPAD_LEFT_SHOULDER) || g.bLeftTrigger > 128;
    if (fire) keys[kKeyIndex(Key::Fire)] = true;
    if (slow) keys[kKeyIndex(Key::Slow)] = true;
    if (btn(XINPUT_GAMEPAD_START)) {
        keys[kKeyIndex(Key::Pause)] = true;
        keys[kKeyIndex(Key::Confirm)] = true;
    }
    if (btn(XINPUT_GAMEPAD_A)) keys[kKeyIndex(Key::Confirm)] = true;
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
    return js_input_state("down", key_code(key)) != 0;
#else
    return g_state.down[kKeyIndex(key)];
#endif
}

bool Input::was_pressed(Key key) {
#ifdef __EMSCRIPTEN__
    if (key == Key::Fire) {
        return js_input_state("pressed", "KeyJ") != 0 || js_input_state("pressed", "Space") != 0;
    }
    return js_input_state("pressed", key_code(key)) != 0;
#else
    return g_state.pressed[kKeyIndex(key)];
#endif
}

bool Input::was_released(Key key) {
#ifdef __EMSCRIPTEN__
    if (key == Key::Fire) {
        return js_input_state("released", "KeyJ") != 0 || js_input_state("released", "Space") != 0;
    }
    return js_input_state("released", key_code(key)) != 0;
#else
    return g_state.released[kKeyIndex(key)];
#endif
}

bool Input::any_gesture() {
#ifdef __EMSCRIPTEN__
    return js_input_any_gesture() != 0;
#else
    for (u32 k = 0; k < kKeyCount; k++) {
        if (g_state.down[k]) return true;
    }
    return false;
#endif
}

void Input::update() {
#ifdef __EMSCRIPTEN__
    js_input_update();
    js_input_poll();
#else
    bool keys[kKeyCount] = {};
    poll_keyboard(keys);
    poll_gamepad(keys);
    for (u32 k = 0; k < kKeyCount; k++) {
        bool on = keys[k];
        bool was = g_state.prev[k];
        g_state.pressed[k] = on && !was;
        g_state.released[k] = !on && was;
        g_state.down[k] = on;
        g_state.prev[k] = on;
    }
#endif
}

}
