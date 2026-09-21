#include "Renderer2D.h"
#include "Platform.h"

#include <cstdio>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

namespace Aether::Engine {

bool Renderer2D::Init(u32 width, u32 height)
{
    mWidth = width;
    mHeight = height;
    mInitState = InitState::DeviceInit;
    StepInit();
    return !IsFailed();
}

void Renderer2D::StepInit()
{
    switch (mInitState)
    {
        case InitState::NotStarted:
            break;
        case InitState::DeviceInit:
        {
#ifdef __EMSCRIPTEN__
            mDevice = RHI::CreateWebGPUDevice();
#else
            mDevice = RHI::CreateD3D11Device();
#endif
            if (!mDevice)
            {
                printf("Renderer2D: failed to create RHI device\n");
                mInitState = InitState::Failed;
                return;
            }
            if (!mDevice->Init(Aether::Platform::NativeWindowHandle()))
            {
                mInitState = InitState::Failed;
                return;
            }
            mInitState = InitState::DeviceReady;
            [[fallthrough]];
        }
        case InitState::DeviceReady:
        {
            mDevice->Tick();
            if (mDevice->IsFailed())
            {
                mInitState = InitState::Failed;
                return;
            }
            if (!mDevice->IsReady())
            {
                break;
            }
#ifdef __EMSCRIPTEN__
            double canvasWidth = 0, canvasHeight = 0;
            emscripten_get_element_css_size("#canvas", &canvasWidth, &canvasHeight);
            if (canvasWidth > 0 && canvasHeight > 0)
            {
                mWidth = static_cast<u32>(canvasWidth);
                mHeight = static_cast<u32>(canvasHeight);
            }
#endif
            mSwapchain = mDevice->CreateSwapchain(mWidth, mHeight);
            if (!mSwapchain)
            {
                printf("Renderer2D: failed to create swapchain\n");
                mInitState = InitState::Failed;
                return;
            }
            mInitState = InitState::SwapchainCreated;
            [[fallthrough]];
        }
        case InitState::SwapchainCreated:
        {
            if (!mSpriteBatch.Init(mDevice.Get(), mSwapchain->ColorFormat()))
            {
                mInitState = InitState::Failed;
                return;
            }
            if (!mStarfield.Init(mDevice.Get(), mSwapchain->ColorFormat()))
            {
                mInitState = InitState::Failed;
                return;
            }
            mInitState = InitState::ResourcesCreated;
            [[fallthrough]];
        }
        case InitState::ResourcesCreated:
        {
            printf("Renderer2D initialized: %ux%u\n", mWidth, mHeight);
            mReady = true;
            mInitState = InitState::Ready;
            break;
        }
        case InitState::Ready:
        case InitState::Failed:
            break;
    }
}

void Renderer2D::Tick()
{
    if (mDevice)
    {
        mDevice->Tick();
    }
    if (mInitState != InitState::Ready && mInitState != InitState::Failed)
    {
        StepInit();
    }
}

void Renderer2D::Shutdown()
{
    if (mDevice)
    {
        mDevice->WaitIdle();
    }
    mReady = false;
    mEncoder.Reset();
    mSwapchain.Reset();
    mDevice.Reset();
}

bool Renderer2D::BeginFrame()
{
    if (!mReady || mFrameStarted) return false;

    RHI::RHITextureView* colorView = mSwapchain->GetCurrentView();
    if (!colorView) return false;

    mEncoder = mDevice->CreateCommandEncoder();
    if (!mEncoder) return false;

    RHI::RenderPassColorAttachment colorAttachment;
    colorAttachment.View = colorView;
    colorAttachment.LoadOp = RHI::LoadOp::Clear;
    colorAttachment.StoreOp = RHI::StoreOp::Store;
    colorAttachment.ClearColor[0] = mClearColor[0];
    colorAttachment.ClearColor[1] = mClearColor[1];
    colorAttachment.ClearColor[2] = mClearColor[2];
    colorAttachment.ClearColor[3] = mClearColor[3];

    RHI::RenderPassDepthAttachment depthAttachment;
    depthAttachment.View = nullptr;
    if (mSwapchain->DepthFormat() != RHI::Format::Undefined)
    {
        depthAttachment.View = mSwapchain->GetDepthView();
    }

    RHI::RenderPassDesc passDesc;
    passDesc.ColorAttachmentCount = 1;
    passDesc.ColorAttachments = &colorAttachment;
    if (depthAttachment.View)
    {
        depthAttachment.LoadOp = RHI::LoadOp::Clear;
        depthAttachment.StoreOp = RHI::StoreOp::Store;
        depthAttachment.ClearDepth = 1.0f;
        passDesc.DepthAttachment = &depthAttachment;
    }
    else
    {
        passDesc.DepthAttachment = nullptr;
    }

    mEncoder->BeginRenderPass(passDesc);
    mSpriteBatch.Clear();
    mFrameStarted = true;
    return true;
}

void Renderer2D::EndFrame()
{
    if (!mFrameStarted) return;

    mEncoder->EndRenderPass();
    mEncoder->Finish();
    mEncoder->Submit();
    mSwapchain->Present();
    mEncoder.Reset();
    mFrameStarted = false;

#ifdef __EMSCRIPTEN__
    static u32 sFrameCount = 0;
    if (++sFrameCount % 300 == 1)
    {
        printf("Renderer2D frame %u done (%ux%u)\n", sFrameCount, mWidth, mHeight);
    }
#endif
}

void Renderer2D::SetClearColor(f32 r, f32 g, f32 b, f32 a)
{
    mClearColor[0] = r;
    mClearColor[1] = g;
    mClearColor[2] = b;
    mClearColor[3] = a;
}

}
