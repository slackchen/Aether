#include "Platform.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

namespace Aether::Platform {

void InitWindow(u32 width, u32 height, const char* title)
{
    (void)width;
    (void)height;
    (void)title;
}

void* NativeWindowHandle()
{
    return nullptr;
}

bool ShouldExit()
{
    return false;
}

void RequestExit()
{
}

double NowSeconds()
{
#ifdef __EMSCRIPTEN__
    return emscripten_get_now() * 0.001;
#else
    return 0.0;
#endif
}

void RunMainLoop(void (*frameCallback)(void*), void* userData)
{
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(frameCallback, userData, 0, 1);
#else
    (void)frameCallback;
    (void)userData;
#endif
}

f32 GetAndResetMouseWheel()
{
    return 0.0f;
}

}
