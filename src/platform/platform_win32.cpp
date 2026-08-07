#include "platform/platform.h"

#ifndef __EMSCRIPTEN__

#include <windows.h>
#include <cstdio>

namespace aether::platform {

namespace {

constexpr const char* kWindowClass = "AetherWindow";

HWND g_hwnd = nullptr;
HINSTANCE g_instance = nullptr;
bool g_should_exit = false;
void (*g_alt_enter_callback)() = nullptr;

LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            if (wparam == VK_RETURN && (GetAsyncKeyState(VK_MENU) & 0x8000)) {
                if (g_alt_enter_callback) {
                    g_alt_enter_callback();
                    return 0;
                }
            }
            return DefWindowProc(hwnd, msg, wparam, lparam);
        default:
            return DefWindowProc(hwnd, msg, wparam, lparam);
    }
}

}  // namespace

void init_window(u32 width, u32 height, const char* title) {
    if (g_hwnd) return;

    g_instance = GetModuleHandle(nullptr);

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = window_proc;
    wc.hInstance = g_instance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = kWindowClass;
    RegisterClassEx(&wc);

    RECT rect = {0, 0, (LONG)width, (LONG)height};
    AdjustWindowRectEx(&rect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                       FALSE, WS_EX_APPWINDOW);

    g_hwnd = CreateWindowEx(
        WS_EX_APPWINDOW, kWindowClass, title ? title : "Aether",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        nullptr, nullptr, g_instance, nullptr);

    if (!g_hwnd) {
        printf("Platform: failed to create window (error %lu)\n", GetLastError());
        return;
    }

    ShowWindow(g_hwnd, SW_SHOW);
    SetForegroundWindow(g_hwnd);
}

void* native_window_handle() {
    return g_hwnd;
}

bool should_exit() {
    return g_should_exit;
}

void request_exit() {
    g_should_exit = true;
    if (g_hwnd) {
        PostMessage(g_hwnd, WM_CLOSE, 0, 0);
    }
}

double now_seconds() {
    static LARGE_INTEGER freq = {};
    if (freq.QuadPart == 0) {
        QueryPerformanceFrequency(&freq);
    }
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)freq.QuadPart;
}

void set_alt_enter_callback(void (*callback)()) {
    g_alt_enter_callback = callback;
}

void run_main_loop(void (*frame_callback)(void*), void* user_data) {
    if (!g_hwnd) {
        printf("Platform: no window, aborting main loop\n");
        return;
    }

    MSG msg = {};
    while (!g_should_exit) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                g_should_exit = true;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (g_should_exit) break;
        if (frame_callback) {
            frame_callback(user_data);
        }
    }
}

}

#endif
