#pragma once

#include "Core.h"

namespace Aether::Platform {

void InitWindow(u32 width, u32 height, const char* title);
void* NativeWindowHandle();
bool ShouldExit();
void RequestExit();
double NowSeconds();
void RunMainLoop(void (*frameCallback)(void*), void* userData);
f32 GetAndResetMouseWheel();

#ifdef _WIN32
void SetAltEnterCallback(void (*callback)());
#endif

}
