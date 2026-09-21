#include "Renderer.h"
#include "Math/Mat4.h"
#include "Platform.h"
#include "Shaders.h"

#include <cstdio>
#include <cstring>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

namespace Aether::Engine {

Renderer::Renderer() = default;
Renderer::~Renderer() = default;

bool Renderer::Init(const RendererConfig& config)
{
    mConfig = config;
    mWidth = config.Width;
    mHeight = config.Height;
    mInitState = InitState::DeviceInit;
    StepInit();
    return mInitState != InitState::Failed;
}

void Renderer::StepInit()
{
    switch (mInitState)
    {
        case InitState::NotStarted:
            break;
        case InitState::DeviceInit:
        {
            switch (mConfig.Backend)
            {
#ifdef __EMSCRIPTEN__
                case RHI::BackendType::WebGPU:
                    mDevice = RHI::CreateWebGPUDevice();
                    break;
#else
                case RHI::BackendType::D3D11:
                    mDevice = RHI::CreateD3D11Device();
                    break;
#endif
                default:
                    printf("Unsupported backend type\n");
                    mInitState = InitState::Failed;
                    return;
            }
            if (!mDevice)
            {
                printf("Failed to create RHI device\n");
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
                printf("Renderer device initialization failed\n");
                mInitState = InitState::Failed;
                return;
            }
            if (!mDevice->IsReady())
            {
                break;
            }
#ifdef __EMSCRIPTEN__
            double canvasWidth, canvasHeight;
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
                printf("Failed to create swapchain\n");
                mInitState = InitState::Failed;
                return;
            }
            mInitState = InitState::SwapchainCreated;
            [[fallthrough]];
        }
        case InitState::SwapchainCreated:
        {
            for (Entity& entity : mScene->Entities())
            {
                if (entity.Mesh && !entity.Mesh->IsUploaded())
                {
                    entity.Mesh->Upload(mDevice.Get());
                }
            }
            CreatePipeline();
            if (!mPipeline)
            {
                mInitState = InitState::Failed;
                return;
            }
            mInitState = InitState::PipelineCreated;
            [[fallthrough]];
        }
        case InitState::PipelineCreated:
        {
            printf("Renderer initialized: %ux%u\n", mWidth, mHeight);
            mReady = true;
            mInitState = InitState::Ready;
            break;
        }
        case InitState::Ready:
        case InitState::Failed:
            break;
    }
}

void Renderer::Tick()
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

void Renderer::Shutdown()
{
    if (mDevice)
    {
        mDevice->WaitIdle();
    }
    mReady = false;
    mPipeline.Reset();
    mBindGroup.Reset();
    mUniformBuffer.Reset();
    mVertexShader.Reset();
    mFragmentShader.Reset();
    mSwapchain.Reset();
    mEncoder.Reset();
    mDevice.Reset();
}

void Renderer::CreatePipeline()
{
    if (!mScene || mScene->Entities().IsEmpty())
    {
        printf("Renderer: no scene entities to render\n");
        return;
    }

    Mesh* mesh = mScene->Entities().First().Mesh.Get();
    if (!mesh || !mesh->IsUploaded())
    {
        printf("Renderer: no uploaded mesh to build pipeline for\n");
        return;
    }

    const Shaders::ShaderSources& sources = Shaders::UnlitColor();

    RHI::ShaderModuleDesc vsDesc;
    vsDesc.Code = sources.Vertex;
    vsDesc.CodeSize = std::strlen(sources.Vertex);
    vsDesc.EntryPoint = "vs_main";
    mVertexShader = mDevice->CreateShader(RHI::ShaderStage::Vertex, vsDesc);
    if (!mVertexShader) return;

    RHI::ShaderModuleDesc fsDesc;
    fsDesc.Code = sources.Fragment;
    fsDesc.CodeSize = std::strlen(sources.Fragment);
    fsDesc.EntryPoint = "fs_main";
    mFragmentShader = mDevice->CreateShader(RHI::ShaderStage::Fragment, fsDesc);
    if (!mFragmentShader) return;

    RHI::ColorTargetState colorTarget;
    colorTarget.Format = mSwapchain->ColorFormat();

    RHI::RenderPipelineDesc pipelineDesc;
    pipelineDesc.VertexShader = mVertexShader.Get();
    pipelineDesc.FragmentShader = mFragmentShader.Get();
    pipelineDesc.PrimitiveTopology = RHI::PrimitiveTopology::TriangleList;
    pipelineDesc.VertexLayout = mesh->VertexLayout();
    pipelineDesc.ColorTargetCount = 1;
    pipelineDesc.ColorTargets = &colorTarget;
    pipelineDesc.FrontFace = RHI::FrontFace::CCW;
    pipelineDesc.CullMode = RHI::CullMode::None;
    pipelineDesc.DepthStencil = nullptr;

    // The unlit WGSL reads its transform from group 0 binding 0; the same
    // layout must appear both here and in the bind group created below.
    RHI::BindGroupLayoutDesc layoutDesc;
    layoutDesc.Entries = {{0, RHI::BindGroupEntryKind::Uniform}};
    pipelineDesc.BindGroupLayouts.Add(layoutDesc);

    mPipeline = mDevice->CreateRenderPipeline(pipelineDesc);
    if (!mPipeline)
    {
        printf("Failed to create render pipeline\n");
        return;
    }

    CreateUniforms();
}

void Renderer::CreateUniforms()
{
    RHI::BufferDesc uniformBufferDesc;
    uniformBufferDesc.Size = sizeof(f32) * 16;
    uniformBufferDesc.Usage = (u32)RHI::BufferUsage::Uniform | (u32)RHI::BufferUsage::CopyDst;
    uniformBufferDesc.MemoryType = RHI::BufferMemoryType::DeviceLocal;
    mUniformBuffer = mDevice->CreateBuffer(uniformBufferDesc, nullptr);
    if (!mUniformBuffer)
    {
        printf("Failed to create uniform buffer\n");
        return;
    }

    RHI::BindGroupLayoutDesc layoutDesc;
    layoutDesc.Entries = {{0, RHI::BindGroupEntryKind::Uniform}};
    Array<RHI::BindGroupEntry> entries;
    entries.Add({0, mUniformBuffer.Get(), nullptr, nullptr, 0, 0});
    mBindGroup = mDevice->CreateBindGroup(layoutDesc, entries);
    if (!mBindGroup)
    {
        printf("Failed to create bind group\n");
    }
}

bool Renderer::BeginFrame()
{
    if (!mReady || mFrameStarted) return false;
    mEncoder = mDevice->CreateCommandEncoder();
    if (!mEncoder) return false;

    RHI::RHITextureView* colorView = mSwapchain->GetCurrentView();
    if (!colorView) return false;

    RHI::RenderPassColorAttachment colorAttachment;
    colorAttachment.View = colorView;
    colorAttachment.LoadOp = RHI::LoadOp::Clear;
    colorAttachment.StoreOp = RHI::StoreOp::Store;
    colorAttachment.ClearColor[0] = mClearColor[0];
    colorAttachment.ClearColor[1] = mClearColor[1];
    colorAttachment.ClearColor[2] = mClearColor[2];
    colorAttachment.ClearColor[3] = mClearColor[3];

    RHI::RenderPassDepthAttachment depthAttachment;
    depthAttachment.View = mSwapchain->GetDepthView();
    depthAttachment.LoadOp = RHI::LoadOp::Clear;
    depthAttachment.StoreOp = RHI::StoreOp::Store;
    depthAttachment.ClearDepth = 1.0f;

    RHI::RenderPassDesc passDesc;
    passDesc.ColorAttachmentCount = 1;
    passDesc.ColorAttachments = &colorAttachment;
    passDesc.DepthAttachment = depthAttachment.View ? &depthAttachment : nullptr;

    mEncoder->BeginRenderPass(passDesc);
    mFrameStarted = true;
    return true;
}

void Renderer::Draw()
{
    if (!mReady || !mFrameStarted || !mScene || !mUniformBuffer || !mBindGroup) return;

    f32 aspect = (mHeight > 0) ? (f32)mWidth / (f32)mHeight : 1.0f;
    Math::Mat4 viewProjection = mCamera.ViewProjection(aspect);

    for (Entity& entity : mScene->Entities())
    {
        Mesh* mesh = entity.Mesh.Get();
        if (!mesh || !mesh->IsUploaded()) continue;

        Math::Mat4 mvp = viewProjection * entity.Transform.ModelMatrix();
        mDevice->UpdateBuffer(mUniformBuffer.Get(), 0, mvp.m, sizeof(mvp.m));

        mEncoder->SetBindGroup(0, mBindGroup.Get());
        mEncoder->SetPipeline(mPipeline.Get());
        mEncoder->SetVertexBuffer(0, mesh->VertexBuffer().Get(), 0);
        if (mesh->HasIndices())
        {
            mEncoder->SetIndexBuffer(mesh->IndexBuffer().Get(), 0);
            mEncoder->DrawIndexed(mesh->IndexCount(), 1, 0, 0, 0);
        }
        else
        {
            mEncoder->Draw(mesh->VertexCount(), 1, 0, 0);
        }
    }
}

void Renderer::EndFrame()
{
    if (!mFrameStarted) return;

    mEncoder->EndRenderPass();
    mEncoder->Finish();
    mEncoder->Submit();
    mSwapchain->Present();
    mEncoder.Reset();
    mFrameStarted = false;
}

void Renderer::SetClearColor(f32 r, f32 g, f32 b, f32 a)
{
    mClearColor[0] = r;
    mClearColor[1] = g;
    mClearColor[2] = b;
    mClearColor[3] = a;
}

}
