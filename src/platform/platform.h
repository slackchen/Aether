#pragma once

#include "core/platform.h"

namespace aether::platform {

void init_window(u32 width, u32 height, const char* title);
void* native_window_handle();
bool should_exit();
void request_exit();
double now_seconds();
void run_main_loop(void (*frame_callback)(void*), void* user_data);
f32 get_and_reset_mouse_wheel();

#ifdef _WIN32
void set_alt_enter_callback(void (*callback)());
#endif

}

