#include "core/platform.h"
#include "engine/audio.h"
#include "engine/input.h"
#include "engine/renderer2d.h"
#include "engine/timer.h"
#include "engine/ui.h"
#include "platform/platform.h"
#include "demo/flight_shmup/game.h"
#include "demo/flight_shmup/game_ui.h"

#include <cstdio>
#include <memory>

namespace {

struct App {
    aether::engine::Renderer2D renderer;
    aether::engine::Timer timer;
    std::unique_ptr<shmup::Game> game;
    bool started = false;
};

App* g_app = nullptr;

void frame_loop(void* user_data) {
    auto* app = static_cast<App*>(user_data);

    app->renderer.tick();
    if (!app->renderer.is_ready()) {
        if (app->renderer.is_failed()) {
            printf("Renderer failed to initialize\n");
            aether::platform::request_exit();
        }
        return;
    }

    static bool alt_registered = false;
    if (!alt_registered) {
        alt_registered = true;
#ifdef _WIN32
        aether::platform::set_alt_enter_callback([] {
            if (g_app && g_app->renderer.device()) {
                g_app->renderer.device()->toggle_fullscreen();
            }
        });
#endif
    }

    if (!app->started) {
        app->started = true;
        app->game = std::make_unique<shmup::Game>(&app->renderer, &app->timer);
        shmup::ui::show_screen("panel-title", true);
    }

    app->timer.update();
    if (aether::engine::Input::any_gesture() || aether::engine::Input::was_pressed(aether::engine::Key::Confirm)) {
        aether::engine::Audio::unlock();
    }

    app->game->update();
    app->game->render();
    aether::engine::Input::update();
}

}

int main() {
    aether::platform::init_window(1280, 720, "Aether Shmup");
    aether::engine::Input::init();
    aether::engine::Audio::init();
    aether::engine::ui::init();

    static App app;
    g_app = &app;
    app.renderer.init(1280, 720);

    aether::platform::run_main_loop(frame_loop, &app);
    return 0;
}
