#include "platform/platform.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace aether::platform {

void init_window(u32 width, u32 height, const char* title) {
    (void)width;
    (void)height;
    (void)title;
}

void* native_window_handle() {
    return nullptr;
}

bool should_exit() {
    return false;
}

void request_exit() {
}

double now_seconds() {
#ifdef __EMSCRIPTEN__
    return emscripten_get_now() * 0.001;
#else
    return 0.0;
#endif
}

void run_main_loop(void (*frame_callback)(void*), void* user_data) {
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(frame_callback, user_data, 0, 1);
#else
    (void)frame_callback;
    (void)user_data;
#endif
}

f32 get_and_reset_mouse_wheel() {
    return 0.0f;
}

}

