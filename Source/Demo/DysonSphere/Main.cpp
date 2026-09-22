#include "Game.h"

#include "Audio.h"
#include "Container/RefPtr.h"
#include "Core.h"
#include "EngineLoop.h"
#include "Input.h"
#include "Platform.h"
#include "Renderer2D.h"
#include "System.h"
#include "Timer.h"
#include "UI.h"

#include <cstdio>

using namespace Aether;

namespace {

struct App {
    Engine::Renderer2D Renderer;
    Engine::EngineLoop Loop;
    UniquePtr<DSP::Game> Game;

    App()
        : Loop(Renderer)
    {
    }
};

App* gApp = nullptr;

void FrameLoop(void* userData)
{
    App* app = static_cast<App*>(userData);

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

    app->Loop.Tick();
}

} // namespace

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
    app.Loop.SetAutoUI(false); // DSPUI 把引擎 UI 集成进自己的批次, 关掉默认 UI 系统
    app.Loop.SetStartupCallback([]() {
        gApp->Game = MakeUnique<DSP::Game>(&gApp->Renderer, &gApp->Loop.GetTimer());
        gApp->Game->RegisterSystems(gApp->Loop.Scheduler());
    });

    Platform::RunMainLoop(FrameLoop, &app);
    return 0;
}
