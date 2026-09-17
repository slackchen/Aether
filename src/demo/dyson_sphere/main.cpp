#include "core/platform.h"
#include "engine/audio.h"
#include "engine/input.h"
#include "engine/renderer2d.h"
#include "engine/timer.h"
#include "engine/ui.h"
#include "platform/platform.h"
#include "demo/dyson_sphere/game.h"

#include <cstdio>
#include <memory>

namespace {

struct App {
    aether::engine::Renderer2D renderer;
    aether::engine::Timer timer;
    std::unique_ptr<dsp::Game> game;
    bool started = false;
};

App* g_app = nullptr;

void frame_loop(void* user_data) {
    auto* app = static_cast<App*>(user_data);

    app->renderer.tick();
    if (!app->renderer.is_ready()) {
        if (app->renderer.is_failed()) {
            printf("DSP Renderer failed to initialize\n");
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
        app->game = std::make_unique<dsp::Game>(&app->renderer, &app->timer);
    }

    app->timer.update();
    if (aether::engine::Input::any_gesture() || aether::engine::Input::was_pressed(aether::engine::Key::Confirm)) {
        aether::engine::Audio::unlock();
    }

    app->game->update();
    app->game->render();
    aether::engine::Input::update();

    // 帧耗时统计 (约每 2 秒打印一次平均值/峰值, 便于性能观察)
    static float s_acc = 0.0f, s_peak = 0.0f;
    static unsigned s_frames = 0;
    float ft = app->timer.delta();
    s_acc += ft;
    if (ft > s_peak) s_peak = ft;
    if (++s_frames >= 120) {
        printf("[perf] avg %.2f ms  max %.2f ms  (sprites=%u)\n",
               s_acc * 1000.0f / s_frames, s_peak,
               app->game ? app->renderer.sprites().sprite_count() : 0);
        s_acc = 0.0f;
        s_peak = 0.0f;
        s_frames = 0;
    }
}

}

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    aether::platform::init_window(1280, 720, "Dyson Sphere Program - Aether Engine");
    aether::engine::Input::init();
    aether::engine::Audio::init();
    aether::engine::ui::init();

    static App app;
    g_app = &app;
    app.renderer.init(1280, 720);

    aether::platform::run_main_loop(frame_loop, &app);
    return 0;
}
