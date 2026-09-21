#include "Game.h"
#include "GameUI.h"

#include "Audio.h"
#include "Container/RefPtr.h"
#include "Core.h"
#include "Input.h"
#include "Platform.h"
#include "Renderer2D.h"
#include "Timer.h"
#include "UI.h"

#include <cstdio>

using namespace Aether;

namespace
{

struct App
{
    Engine::Renderer2D Renderer;
    Engine::Timer Timer;
    UniquePtr<Shmup::Game> Game;
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
            printf("Renderer failed to initialize\n");
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
        app->Game = MakeUnique<Shmup::Game>(&app->Renderer, &app->Timer);
        Shmup::UI::ShowScreen("panel-title", true);
    }

    app->Timer.Update();
    if (Engine::Input::AnyGesture() || Engine::Input::WasPressed(Engine::Key::Confirm))
    {
        Engine::Audio::Unlock();
    }

    app->Game->Update();
    app->Game->Render();
    Engine::Input::Update();
}

}

int main()
{
    Platform::InitWindow(1280, 720, "Aether Shmup");
    Engine::Input::Init();
    Engine::Audio::Init();
    Engine::UI::Init();

    static App sApp;
    gApp = &sApp;
    sApp.Renderer.Init(1280, 720);

    Platform::RunMainLoop(FrameLoop, &sApp);
    return 0;
}
