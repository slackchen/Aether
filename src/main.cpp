#include "core/platform.h"
#include "engine/renderer.h"
#include "engine/mesh.h"
#include "engine/scene.h"
#include "platform/platform.h"
#include <cstdio>
#include <memory>
#include <vector>

using namespace aether;

static std::unique_ptr<engine::Renderer> g_renderer;
static std::unique_ptr<engine::Scene> g_scene;
static f32 g_demo_angle = 0.0f;

static void frame_callback(void* user_data) {
    (void)user_data;
    if (!g_renderer) return;

    g_demo_angle += 0.6f;
    if (!g_scene->entities().empty()) {
        g_scene->entities()[0].transform.rotation_deg.y = g_demo_angle;
    }

    g_renderer->tick();

    if (g_renderer->is_ready()) {
        if (g_renderer->begin_frame()) {
            g_renderer->draw();
            g_renderer->end_frame();
        }
    }
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    printf("Aether 3D Engine v0.1.0\n");
#ifdef __EMSCRIPTEN__
    printf("Platform: WebGPU\n");
#else
    printf("Platform: D3D11\n");
#endif

    aether::platform::init_window(1280, 720, "Aether Engine");

    std::vector<rhi::VertexAttribute> attributes = {
        {0, 0, rhi::Format::Float32x3},
        {1, sizeof(f32) * 3, rhi::Format::Float32x3},
    };

    const f32 triangle_vertices[] = {
         0.0f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,
         0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,
    };

    auto triangle = std::make_shared<engine::Mesh>();
    triangle->set_vertices(triangle_vertices, 3, sizeof(f32) * 6, attributes);

    g_scene = std::make_unique<engine::Scene>();
    g_scene->add_entity(triangle);

    engine::Camera camera;
    camera.position = {0.0f, 0.0f, -3.0f};

    g_renderer = std::make_unique<engine::Renderer>();
    g_renderer->set_scene(g_scene.get());
    g_renderer->set_camera(camera);

    engine::RendererConfig config;
    config.width = 1280;
    config.height = 720;
    config.title = "Aether Engine";
#ifdef __EMSCRIPTEN__
    config.backend = rhi::BackendType::WebGPU;
#else
    config.backend = rhi::BackendType::D3D11;
#endif

    if (!g_renderer->init(config)) {
        printf("Renderer initialization failed to start\n");
    }

    aether::platform::run_main_loop(frame_callback, nullptr);
    return 0;
}
