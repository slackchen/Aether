#include "Container/RefPtr.h"
#include "Core.h"
#include "Mesh.h"
#include "Platform.h"
#include "Renderer.h"
#include "Scene.h"
#include <cstdio>

using namespace Aether;

static UniquePtr<Engine::Renderer> gRenderer;
static UniquePtr<Engine::Scene> gScene;
static f32 gDemoAngle = 0.0f;

static void FrameCallback(void* userData)
{
    (void)userData;
    if (!gRenderer)
        return;

    gDemoAngle += 0.6f;
    if (!gScene->Entities().IsEmpty())
    {
        gScene->Entities()[0].Transform.RotationDeg.y = gDemoAngle;
    }

    gRenderer->Tick();

    if (gRenderer->IsReady())
    {
        if (gRenderer->BeginFrame())
        {
            gRenderer->Draw();
            gRenderer->EndFrame();
        }
    }
}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    printf("Aether 3D Engine v0.1.0\n");
#ifdef __EMSCRIPTEN__
    printf("Platform: WebGPU\n");
#else
    printf("Platform: D3D11\n");
#endif

    Aether::Platform::InitWindow(1280, 720, "Aether Engine");

    Array<RHI::VertexAttribute> attributes;
    attributes.Add({0, 0, RHI::Format::Float32x3});
    attributes.Add({1, sizeof(f32) * 3, RHI::Format::Float32x3});

    const f32 triangleVertices[] = {
         0.0f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,
         0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,
    };

    RefPtr<Engine::Mesh> triangle = MakeRef<Engine::Mesh>();
    triangle->SetVertices(triangleVertices, 3, sizeof(f32) * 6, attributes);

    gScene = MakeUnique<Engine::Scene>();
    gScene->AddEntity(triangle);

    Engine::Camera camera;
    camera.Position = {0.0f, 0.0f, -3.0f};

    gRenderer = MakeUnique<Engine::Renderer>();
    gRenderer->SetScene(gScene.Get());
    gRenderer->SetCamera(camera);

    Engine::RendererConfig config;
    config.Width = 1280;
    config.Height = 720;
    config.Title = "Aether Engine";
#ifdef __EMSCRIPTEN__
    config.Backend = RHI::BackendType::WebGPU;
#else
    config.Backend = RHI::BackendType::D3D11;
#endif

    if (!gRenderer->Init(config))
    {
        printf("Renderer initialization failed to start\n");
    }

    Aether::Platform::RunMainLoop(FrameCallback, nullptr);
    return 0;
}
