#include "Game.h"
#include "GameUI.h"

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

namespace
{

struct App
{
    Engine::Renderer2D Renderer;
    Engine::EngineLoop Loop;
    UniquePtr<Shmup::Game> Game;

    App()
        : Loop(Renderer)
    {
    }
};

App* gApp = nullptr;

void RegisterSystems(App& app)
{
    Engine::SystemDef audioUnlock;
    audioUnlock.Name = "AudioUnlock";
    audioUnlock.SysPhase = Engine::Phase::Input;
    // 同相位读引擎 InputSystem 写的快照, 必须声明 Reads 才能排到它之后
    audioUnlock.Reads = {Engine::TypeIdOf<Engine::Tags::Input>()};
    audioUnlock.Update = [&app](Engine::SystemContext& ctx) {
        AETHER_UNUSED(app);
        if (ctx.Input->AnyGesture() || ctx.Input->WasPressed(Engine::Key::Confirm))
        {
            Engine::Audio::Unlock();
        }
    };
    app.Loop.Scheduler().Register(std::move(audioUnlock));

    Engine::SystemDef gameUpdate;
    gameUpdate.Name = "Game";
    gameUpdate.SysPhase = Engine::Phase::Simulation;
    gameUpdate.Update = [&app](Engine::SystemContext&) {
        if (app.Game)
        {
            app.Game->Update();
        }
    };
    app.Loop.Scheduler().Register(std::move(gameUpdate));

    Engine::SystemDef gameRender;
    gameRender.Name = "GameRender";
    gameRender.SysPhase = Engine::Phase::RenderSubmit;
    gameRender.Update = [&app](Engine::SystemContext& ctx) {
        if (app.Game && ctx.FrameActive)
        {
            app.Game->Render();
        }
    };
    app.Loop.Scheduler().Register(std::move(gameRender));
}

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
    Platform::InitWindow(1280, 720, "Aether Shmup");
    Engine::Input::Init();
    Engine::Audio::Init();
    Engine::UI::Init();

    static App sApp;
    gApp = &sApp;
    sApp.Renderer.Init(1280, 720);
    sApp.Loop.SetStartupCallback([]() {
        gApp->Game = MakeUnique<Shmup::Game>(&gApp->Renderer, &gApp->Loop.GetTimer());
        Shmup::UI::ShowScreen("panel-title", true);
        RegisterSystems(*gApp);
    });

    Platform::RunMainLoop(FrameLoop, &sApp);
    return 0;
}
