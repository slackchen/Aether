#include "Audio.h"
#include "Input.h"
#include "Renderer2D.h"
#include "Timer.h"
#include "UI.h"
#include "Platform.h"
#include "Game.h"
#include "Container/RefPtr.h"

#include <cstdio>

using namespace Aether;

namespace {

struct App {
    Engine::Renderer2D Renderer;
    Engine::Timer Timer;
    UniquePtr<DSP::Game> Game;
    bool Started = false;
};

App* gApp = nullptr;

void FrameLoop(void* userData)
{
    App* app = static_cast<App*>(userData);

    app->Renderer.Tick();
    if (!app->Renderer.IsReady())
    {
        if (app->Renderer.IsFailed())
        {
            printf("DSP Renderer failed to initialize\n");
            Platform::RequestExit();
        }
        return;
    }

    static bool sAltRegistered = false;
    if (!sAltRegistered)
    {
        sAltRegistered = true;
#ifdef _WIN32
        Platform::SetAltEnterCallback([] {
            if (gApp && gApp->Renderer.Device())
            {
                gApp->Renderer.Device()->ToggleFullscreen();
            }
        });
#endif
    }

    if (!app->Started)
    {
        app->Started = true;
        app->Game = MakeUnique<DSP::Game>(&app->Renderer, &app->Timer);
    }

    app->Timer.Update();
    if (Engine::Input::AnyGesture() || Engine::Input::WasPressed(Engine::Key::Confirm))
    {
        Engine::Audio::Unlock();
    }

    app->Game->Update();
    app->Game->Render();
    Engine::Input::Update();

    // 帧耗时统计 (约每 2 秒打印一次平均值/峰值, 便于性能观察)
    static f32 sAcc = 0.0f, sPeak = 0.0f;
    static u32 sFrames = 0;
    f32 ft = app->Timer.Delta();
    sAcc += ft;
    if (ft > sPeak) sPeak = ft;
    if (++sFrames >= 120)
    {
        printf("[perf] avg %.2f ms  max %.2f ms  (sprites=%u)\n",
               sAcc * 1000.0f / sFrames, sPeak,
               app->Game ? app->Renderer.Sprites().SpriteCount() : 0);
        sAcc = 0.0f;
        sPeak = 0.0f;
        sFrames = 0;
    }
}

}

int main()
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    Platform::InitWindow(1280, 720, "Dyson Sphere Program - Aether Engine");
    Engine::Input::Init();
    Engine::Audio::Init();
    Engine::UI::Init();

    static App app;
    gApp = &app;
    app.Renderer.Init(1280, 720);

    Platform::RunMainLoop(FrameLoop, &app);
    return 0;
}
