#pragma once

#include "Camera.h"
#include "Container/RefPtr.h"
#include "Core.h"
#include "RHI.h"
#include "Scene.h"

namespace Aether::Engine {

struct RendererConfig
{
    u32 Width = 1280;
    u32 Height = 720;
    const char* Title = "Aether Engine";
    RHI::BackendType Backend = RHI::BackendType::WebGPU;
};

class Renderer
{
public:
    Renderer();
    ~Renderer();

    bool Init(const RendererConfig& config);
    void Shutdown();
    void Tick();

    bool BeginFrame();
    void EndFrame();
    void Draw();

    void SetScene(Scene* scene) { mScene = scene; }
    void SetCamera(const Camera& camera) { mCamera = camera; }

    void SetClearColor(f32 r, f32 g, f32 b, f32 a = 1.0f);
    bool IsReady() const { return mReady; }
    RHI::RHIDevice* Device() { return mDevice.Get(); }
    u32 Width() const { return mWidth; }
    u32 Height() const { return mHeight; }

private:
    enum class InitState
    {
        NotStarted,
        DeviceInit,
        DeviceReady,
        SwapchainCreated,
        PipelineCreated,
        Ready,
        Failed,
    };

    void StepInit();
    void CreatePipeline();
    void CreateUniforms();

    UniquePtr<RHI::RHIDevice> mDevice;
    RefPtr<RHI::RHISwapchain> mSwapchain;
    RefPtr<RHI::RHIShader> mVertexShader;
    RefPtr<RHI::RHIShader> mFragmentShader;
    RefPtr<RHI::RHIRenderPipeline> mPipeline;
    RefPtr<RHI::RHIBuffer> mUniformBuffer;
    RefPtr<RHI::RHIBindGroup> mBindGroup;
    UniquePtr<RHI::RHICommandEncoder> mEncoder;

    RendererConfig mConfig;
    InitState mInitState = InitState::NotStarted;
    u32 mWidth = 0;
    u32 mHeight = 0;
    f32 mClearColor[4] = {0.1f, 0.1f, 0.15f, 1.0f};
    bool mFrameStarted = false;
    bool mReady = false;

    Scene* mScene = nullptr;
    Camera mCamera;
};

}
