#include "Starfield.h"
#include "Shaders.h"

#include <cstdio>
#include <cstring>

namespace Aether::Engine {

bool Starfield::Init(RHI::RHIDevice* device, RHI::Format colorFormat)
{
    mDevice = device;

    const f32 positions[3][2] = {{-1.0f, -1.0f}, {3.0f, -1.0f}, {-1.0f, 3.0f}};
    RHI::BufferDesc vertexBufferDesc;
    vertexBufferDesc.Size = sizeof(positions);
    vertexBufferDesc.Usage = (u32)RHI::BufferUsage::Vertex | (u32)RHI::BufferUsage::CopyDst;
    vertexBufferDesc.MemoryType = RHI::BufferMemoryType::DeviceLocal;
    mVertexBuffer = mDevice->CreateBuffer(vertexBufferDesc, positions);
    if (!mVertexBuffer)
    {
        printf("Starfield: failed to create vertex buffer\n");
        return false;
    }

    RHI::BufferDesc uniformBufferDesc;
    uniformBufferDesc.Size = sizeof(Params);
    uniformBufferDesc.Usage = (u32)RHI::BufferUsage::Uniform | (u32)RHI::BufferUsage::CopyDst;
    uniformBufferDesc.MemoryType = RHI::BufferMemoryType::DeviceLocal;
    mUniformBuffer = mDevice->CreateBuffer(uniformBufferDesc, nullptr);
    if (!mUniformBuffer)
    {
        printf("Starfield: failed to create uniform buffer\n");
        return false;
    }

    RHI::BindGroupLayoutDesc layoutDesc;
    layoutDesc.Entries = {{0, RHI::BindGroupEntryKind::Uniform}};
    Array<RHI::BindGroupEntry> bgEntries;
    bgEntries.Add({0, mUniformBuffer.Get(), nullptr, nullptr, 0, 0});
    mBindGroup = mDevice->CreateBindGroup(layoutDesc, bgEntries);
    if (!mBindGroup)
    {
        printf("Starfield: failed to create bind group\n");
        return false;
    }

    Array<RHI::VertexAttribute> attributes;
    attributes.Add({0, 0, RHI::Format::Float32x2});
    RHI::VertexBufferLayout vertexLayout;
    vertexLayout.Stride = sizeof(f32) * 2;
    vertexLayout.AttributeCount = attributes.Count();
    vertexLayout.Attributes = attributes.Data();

    const Shaders::ShaderSources& sources = Shaders::Starfield();
    RHI::ShaderModuleDesc vsDesc;
    vsDesc.Code = sources.Vertex;
    vsDesc.CodeSize = std::strlen(sources.Vertex);
    vsDesc.EntryPoint = "vs_main";
    RefPtr<RHI::RHIShader> vs = mDevice->CreateShader(RHI::ShaderStage::Vertex, vsDesc);
    RHI::ShaderModuleDesc fsDesc;
    fsDesc.Code = sources.Fragment;
    fsDesc.CodeSize = std::strlen(sources.Fragment);
    fsDesc.EntryPoint = "fs_main";
    RefPtr<RHI::RHIShader> fs = mDevice->CreateShader(RHI::ShaderStage::Fragment, fsDesc);
    if (!vs || !fs)
    {
        printf("Starfield: failed to create shaders\n");
        return false;
    }

    RHI::ColorTargetState colorTarget;
    colorTarget.Format = colorFormat;

    RHI::RenderPipelineDesc pipelineDesc;
    pipelineDesc.VertexShader = vs.Get();
    pipelineDesc.FragmentShader = fs.Get();
    pipelineDesc.PrimitiveTopology = RHI::PrimitiveTopology::TriangleList;
    pipelineDesc.VertexLayout = vertexLayout;
    pipelineDesc.ColorTargetCount = 1;
    pipelineDesc.ColorTargets = &colorTarget;
    pipelineDesc.FrontFace = RHI::FrontFace::CCW;
    pipelineDesc.CullMode = RHI::CullMode::None;
    pipelineDesc.BlendMode = RHI::BlendMode::Alpha;
    pipelineDesc.BindGroupLayouts.Add(layoutDesc);

    mPipeline = mDevice->CreateRenderPipeline(pipelineDesc);
    if (!mPipeline)
    {
        printf("Starfield: failed to create pipeline\n");
        return false;
    }

    return true;
}

void Starfield::Render(RHI::RHICommandEncoder* encoder, f32 time, f32 speed, f32 width, f32 height)
{
    if (!mDevice) return;

    Params params;
    params.Time = time;
    params.Speed = speed;
    params.Resolution[0] = width;
    params.Resolution[1] = height;
    mDevice->UpdateBuffer(mUniformBuffer.Get(), 0, &params, sizeof(params));

    encoder->SetBindGroup(0, mBindGroup.Get());
    encoder->SetPipeline(mPipeline.Get());
    encoder->SetVertexBuffer(0, mVertexBuffer.Get(), 0);
    encoder->Draw(3, 1, 0, 0);
}

}
