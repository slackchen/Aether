#include "Platform.h"

#ifndef __EMSCRIPTEN__

#include <windows.h>
#include <cstdio>

namespace Aether::Platform {

namespace {

constexpr const char* WINDOW_CLASS_NAME = "AetherWindow";

HWND gHwnd = nullptr;
HINSTANCE gInstance = nullptr;
bool gShouldExit = false;
void (*gAltEnterCallback)() = nullptr;
f32 gMouseWheelDelta = 0.0f;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
        case WM_CLOSE:
        {
            PostQuitMessage(0);
            return 0;
        }
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
        case WM_MOUSEWHEEL:
        {
            short delta = GET_WHEEL_DELTA_WPARAM(wparam);
            gMouseWheelDelta += (f32)delta / (f32)WHEEL_DELTA;
            return 0;
        }
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            if (wparam == VK_RETURN && (GetAsyncKeyState(VK_MENU) & 0x8000))
            {
                if (gAltEnterCallback)
                {
                    gAltEnterCallback();
                    return 0;
                }
            }
            return DefWindowProc(hwnd, msg, wparam, lparam);
        }
        default:
        {
            return DefWindowProc(hwnd, msg, wparam, lparam);
        }
    }
}

} // namespace

f32 GetAndResetMouseWheel()
{
    f32 value = gMouseWheelDelta;
    gMouseWheelDelta = 0.0f;
    return value;
}

void InitWindow(u32 width, u32 height, const char* title)
{
    if (gHwnd)
    {
        return;
    }

    gInstance = GetModuleHandle(nullptr);

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = gInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = WINDOW_CLASS_NAME;
    RegisterClassEx(&wc);

    RECT rect = {0, 0, (LONG)width, (LONG)height};
    AdjustWindowRectEx(&rect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                       FALSE, WS_EX_APPWINDOW);

    gHwnd = CreateWindowEx(
        WS_EX_APPWINDOW, WINDOW_CLASS_NAME, title ? title : "Aether",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        nullptr, nullptr, gInstance, nullptr);

    if (!gHwnd)
    {
        printf("Platform: failed to create window (error %lu)\n", GetLastError());
        return;
    }

    ShowWindow(gHwnd, SW_SHOW);
    SetForegroundWindow(gHwnd);
}

void* NativeWindowHandle()
{
    return gHwnd;
}

bool ShouldExit()
{
    return gShouldExit;
}

void RequestExit()
{
    gShouldExit = true;
    if (gHwnd)
    {
        PostMessage(gHwnd, WM_CLOSE, 0, 0);
    }
}

double NowSeconds()
{
    static LARGE_INTEGER freq = {};
    if (freq.QuadPart == 0)
    {
        QueryPerformanceFrequency(&freq);
    }
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)freq.QuadPart;
}

void SetAltEnterCallback(void (*callback)())
{
    gAltEnterCallback = callback;
}

void RunMainLoop(void (*frameCallback)(void*), void* userData)
{
    if (!gHwnd)
    {
        printf("Platform: no window, aborting main loop\n");
        return;
    }

    MSG msg = {};
    while (!gShouldExit)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                gShouldExit = true;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (gShouldExit)
        {
            break;
        }
        if (frameCallback)
        {
            frameCallback(userData);
        }
    }
}

}

#endif
