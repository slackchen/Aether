#pragma once

#include "Camera.h"
#include "Container/RefPtr.h"
#include "Core.h"
#include "RHI.h"
#include "SpriteBatch.h"
#include "Starfield.h"

namespace Aether::Engine {

class Renderer2D
{
public:
    enum class InitState
    {
        NotStarted,
        DeviceInit,
        DeviceReady,
        SwapchainCreated,
        ResourcesCreated,
        Ready,
        Failed,
    };

    Renderer2D() = default;
    ~Renderer2D() = default;

    bool Init(u32 width, u32 height);
    void Tick();
    void Shutdown();

    bool IsReady() const { return mReady; }
    bool IsFailed() const { return mInitState == InitState::Failed; }
    u32 Width() const { return mWidth; }
    u32 Height() const { return mHeight; }
    f32 Aspect() const { return mHeight > 0 ? (f32)mWidth / (f32)mHeight : 1.0f; }

    bool BeginFrame();
    void EndFrame();

    Camera2D& GetCamera() { return mCamera; }
    SpriteBatch& Sprites() { return mSpriteBatch; }
    Starfield& GetStarfield() { return mStarfield; }
    RHI::RHIDevice* Device() { return mDevice.Get(); }
    RHI::RHICommandEncoder* Encoder() { return mEncoder.Get(); }
    RHI::Format ColorFormat() const { return mSwapchain ? mSwapchain->ColorFormat() : RHI::Format::BGRA8Unorm; }
    RHI::Format DepthFormat() const { return mSwapchain ? mSwapchain->DepthFormat() : RHI::Format::Undefined; }

    void SetClearColor(f32 r, f32 g, f32 b, f32 a);

private:
    void StepInit();

    u32 mWidth = 0;
    u32 mHeight = 0;
    f32 mClearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};

    UniquePtr<RHI::RHIDevice> mDevice;
    RefPtr<RHI::RHISwapchain> mSwapchain;
    UniquePtr<RHI::RHICommandEncoder> mEncoder;
    bool mFrameStarted = false;
    bool mReady = false;

    Camera2D mCamera;
    SpriteBatch mSpriteBatch;
    Starfield mStarfield;
    InitState mInitState = InitState::NotStarted;
};

}
