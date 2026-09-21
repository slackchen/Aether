#include "SpriteBatch.h"
#include "Math/Math.h"
#include "Shaders.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace Aether::Engine {

bool SpriteBatch::Init(RHI::RHIDevice* device, RHI::Format colorFormat)
{
    mDevice = device;

    RHI::BufferDesc vertexBufferDesc;
    vertexBufferDesc.Size = MAX_SPRITES * VERTICES_PER_SPRITE * FLOATS_PER_VERTEX * sizeof(f32);
    vertexBufferDesc.Usage = (u32)RHI::BufferUsage::Vertex | (u32)RHI::BufferUsage::CopyDst;
    // 每帧更新的顶点缓冲用 HostVisible, 走 Map DISCARD (驱动重命名, 无同步停顿)
    vertexBufferDesc.MemoryType = RHI::BufferMemoryType::HostVisible;
    mVertexBuffer = mDevice->CreateBuffer(vertexBufferDesc, nullptr);
    if (!mVertexBuffer)
    {
        printf("SpriteBatch: failed to create vertex buffer\n");
        return false;
    }

    Array<u32> indices;
    indices.Resize(MAX_SPRITES * 6);
    for (u32 s = 0; s < MAX_SPRITES; s++)
    {
        u32 base = s * 4;
        indices[s * 6 + 0] = base + 0;
        indices[s * 6 + 1] = base + 1;
        indices[s * 6 + 2] = base + 2;
        indices[s * 6 + 3] = base + 2;
        indices[s * 6 + 4] = base + 1;
        indices[s * 6 + 5] = base + 3;
    }
    RHI::BufferDesc indexBufferDesc;
    indexBufferDesc.Size = indices.Count() * sizeof(u32);
    indexBufferDesc.Usage = (u32)RHI::BufferUsage::Index | (u32)RHI::BufferUsage::CopyDst;
    indexBufferDesc.MemoryType = RHI::BufferMemoryType::DeviceLocal;
    mIndexBuffer = mDevice->CreateBuffer(indexBufferDesc, indices.Data());
    if (!mIndexBuffer)
    {
        printf("SpriteBatch: failed to create index buffer\n");
        return false;
    }

    RHI::BufferDesc uniformBufferDesc;
    uniformBufferDesc.Size = sizeof(f32) * 16;
    uniformBufferDesc.Usage = (u32)RHI::BufferUsage::Uniform | (u32)RHI::BufferUsage::CopyDst;
    uniformBufferDesc.MemoryType = RHI::BufferMemoryType::HostVisible;
    mUniformBuffer = mDevice->CreateBuffer(uniformBufferDesc, nullptr);
    if (!mUniformBuffer)
    {
        printf("SpriteBatch: failed to create uniform buffer\n");
        return false;
    }

    RHI::BindGroupLayoutDesc layoutDesc;
    layoutDesc.Entries = {{0, RHI::BindGroupEntryKind::Uniform},
                          {1, RHI::BindGroupEntryKind::Texture},
                          {2, RHI::BindGroupEntryKind::Sampler}};
    mBindGroupLayoutDesc = layoutDesc;

    RHI::SamplerDesc samplerDesc;
    samplerDesc.MagFilter = RHI::TextureFilter::Linear;
    samplerDesc.MinFilter = RHI::TextureFilter::Linear;
    samplerDesc.AddressModeU = RHI::TextureAddressMode::ClampToEdge;
    samplerDesc.AddressModeV = RHI::TextureAddressMode::ClampToEdge;
    mSampler = mDevice->CreateSampler(samplerDesc);
    if (!mSampler)
    {
        printf("SpriteBatch: failed to create sampler\n");
        return false;
    }

    RHI::ColorTargetState colorTarget;
    colorTarget.Format = colorFormat;

    Array<RHI::VertexAttribute> attributes;
    attributes.Add({0, 0, RHI::Format::Float32x2});
    attributes.Add({1, 8, RHI::Format::Float32x2});
    attributes.Add({2, 16, RHI::Format::Float32x4});
    RHI::VertexBufferLayout vertexLayout;
    vertexLayout.Stride = FLOATS_PER_VERTEX * sizeof(f32);
    vertexLayout.AttributeCount = attributes.Count();
    vertexLayout.Attributes = attributes.Data();

    const Shaders::ShaderSources& sources = Shaders::UnlitTexture();
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
        printf("SpriteBatch: failed to create shaders\n");
        return false;
    }

    const RHI::BlendMode blendModes[2] = {RHI::BlendMode::Alpha, RHI::BlendMode::Additive};
    for (u32 i = 0; i < 2; i++)
    {
        RHI::BlendMode blend = blendModes[i];
        RHI::RenderPipelineDesc pipelineDesc;
        pipelineDesc.VertexShader = vs.Get();
        pipelineDesc.FragmentShader = fs.Get();
        pipelineDesc.PrimitiveTopology = RHI::PrimitiveTopology::TriangleList;
        pipelineDesc.VertexLayout = vertexLayout;
        pipelineDesc.ColorTargetCount = 1;
        pipelineDesc.ColorTargets = &colorTarget;
        pipelineDesc.FrontFace = RHI::FrontFace::CCW;
        pipelineDesc.CullMode = RHI::CullMode::None;
        pipelineDesc.BlendMode = blend;
        pipelineDesc.BindGroupLayouts.Add(layoutDesc);

        RefPtr<RHI::RHIRenderPipeline> pipeline = mDevice->CreateRenderPipeline(pipelineDesc);
        if (!pipeline)
        {
            printf("SpriteBatch: failed to create pipeline\n");
            return false;
        }
        if (blend == RHI::BlendMode::Alpha)
        {
            mAlphaPipeline = pipeline;
        }
        else
        {
            mAdditivePipeline = pipeline;
        }
    }

    return true;
}

void SpriteBatch::Clear()
{
    mSprites.Clear();
}

void SpriteBatch::Add(RefPtr<RHI::RHITexture> texture, const Math::Vec2& position, const Math::Vec2& size,
                      const Math::Color& color, f32 rotation, f32 layer, RHI::BlendMode blend)
{
    if (!texture) return;
    if (mSprites.Count() >= MAX_SPRITES) return;
    Sprite& sprite = mSprites.EmplaceAdd();
    sprite.Texture = std::move(texture);
    sprite.Position = position;
    sprite.Size = size;
    sprite.Color = color;
    sprite.Rotation = rotation;
    sprite.Layer = layer;
    sprite.Blend = blend;
}

void SpriteBatch::AddUv(RefPtr<RHI::RHITexture> texture, const Math::Vec2& uv0, const Math::Vec2& uv1,
                        const Math::Vec2& position, const Math::Vec2& size, const Math::Color& color,
                        f32 rotation, f32 layer, RHI::BlendMode blend)
{
    if (!texture) return;
    if (mSprites.Count() >= MAX_SPRITES) return;
    Sprite& sprite = mSprites.EmplaceAdd();
    sprite.Texture = std::move(texture);
    sprite.Uv0 = uv0;
    sprite.Uv1 = uv1;
    sprite.Position = position;
    sprite.Size = size;
    sprite.Color = color;
    sprite.Rotation = rotation;
    sprite.Layer = layer;
    sprite.Blend = blend;
}

void SpriteBatch::AddQuad(RefPtr<RHI::RHITexture> texture,
                          const Math::Vec2& p0, const Math::Vec2& p1, const Math::Vec2& p2, const Math::Vec2& p3,
                          const Math::Color& color, f32 layer,
                          const Math::Vec2& uv0, const Math::Vec2& uv1,
                          RHI::BlendMode blend)
{
    if (!texture || mSprites.Count() >= MAX_SPRITES) return;
    Sprite& sprite = mSprites.EmplaceAdd();
    sprite.Texture = std::move(texture);
    sprite.Color = color;
    sprite.Layer = layer;
    sprite.Blend = blend;
    sprite.Uv0 = uv0;
    sprite.Uv1 = uv1;
    sprite.IsCustomQuad = true;
    sprite.QuadPts[0] = p0;
    sprite.QuadPts[1] = p1;
    sprite.QuadPts[2] = p2;
    sprite.QuadPts[3] = p3;
}

void SpriteBatch::BuildQuad(const Sprite& sprite, f32* verts)
{
    const f32 uvs[4][2] = {{sprite.Uv0.x, sprite.Uv0.y},
                           {sprite.Uv1.x, sprite.Uv0.y},
                           {sprite.Uv0.x, sprite.Uv1.y},
                           {sprite.Uv1.x, sprite.Uv1.y}};

    if (sprite.IsCustomQuad)
    {
        for (u32 i = 0; i < 4; i++)
        {
            verts[i * 8 + 0] = sprite.QuadPts[i].x;
            verts[i * 8 + 1] = sprite.QuadPts[i].y;
            verts[i * 8 + 2] = uvs[i][0];
            verts[i * 8 + 3] = uvs[i][1];
            verts[i * 8 + 4] = sprite.Color.r;
            verts[i * 8 + 5] = sprite.Color.g;
            verts[i * 8 + 6] = sprite.Color.b;
            verts[i * 8 + 7] = sprite.Color.a;
        }
        return;
    }

    f32 hx = sprite.Size.x * 0.5f;
    f32 hy = sprite.Size.y * 0.5f;
    f32 c = cosf(sprite.Rotation);
    f32 s = sinf(sprite.Rotation);

    const f32 local[4][2] = {{-hx, -hy}, {hx, -hy}, {-hx, hy}, {hx, hy}};

    for (u32 i = 0; i < 4; i++)
    {
        f32 rx = local[i][0] * c - local[i][1] * s;
        f32 ry = local[i][0] * s + local[i][1] * c;
        verts[i * 8 + 0] = sprite.Position.x + rx;
        verts[i * 8 + 1] = sprite.Position.y + ry;
        verts[i * 8 + 2] = uvs[i][0];
        verts[i * 8 + 3] = uvs[i][1];
        verts[i * 8 + 4] = sprite.Color.r;
        verts[i * 8 + 5] = sprite.Color.g;
        verts[i * 8 + 6] = sprite.Color.b;
        verts[i * 8 + 7] = sprite.Color.a;
    }
}

void SpriteBatch::Render(RHI::RHICommandEncoder* encoder, const Math::Mat4& vp)
{
    if (!mDevice || mSprites.IsEmpty()) return;

    mDevice->UpdateBuffer(mUniformBuffer.Get(), 0, vp.m, sizeof(vp.m));

    const u32 total = Math::Min(mSprites.Count(), MAX_SPRITES);

    // 按层排序索引 (不整块拷贝 Sprite, 避免每帧引用计数抖动)。
    // 插入排序保持稳定: 同层 Sprite 保持插入顺序。
    mSortScratch.Resize(total);
    for (u32 i = 0; i < total; i++) mSortScratch[i] = i;
    for (u32 i = 1; i < total; i++)
    {
        u32 key = mSortScratch[i];
        u32 j = i;
        while (j > 0 && mSprites[mSortScratch[j - 1]].Layer > mSprites[key].Layer)
        {
            mSortScratch[j] = mSortScratch[j - 1];
            j--;
        }
        mSortScratch[j] = key;
    }

    const u32 floatsPerSprite = VERTICES_PER_SPRITE * FLOATS_PER_VERTEX;
    mVertexScratch.Resize(total * floatsPerSprite);
    for (u32 i = 0; i < total; i++)
    {
        BuildQuad(mSprites[mSortScratch[i]], &mVertexScratch[i * floatsPerSprite]);
    }
    mDevice->UpdateBuffer(mVertexBuffer.Get(), 0, mVertexScratch.Data(),
                          (u64)total * floatsPerSprite * sizeof(f32));

    u32 start = 0;
    while (start < total)
    {
        const Sprite& first = mSprites[mSortScratch[start]];
        u32 end = start + 1;
        while (end < total)
        {
            const Sprite& next = mSprites[mSortScratch[end]];
            if (next.Texture.Get() != first.Texture.Get() || next.Blend != first.Blend) break;
            end++;
        }

        u32 count = end - start;

        const RHI::RHITexture* texKey = first.Texture.Get();
        RefPtr<RHI::RHIBindGroup>* bindGroup = mBindGroupCache.Find(texKey);
        if (!bindGroup)
        {
            Array<RHI::BindGroupEntry> bgEntries;
            bgEntries.Add({0, mUniformBuffer.Get(), nullptr, nullptr, 0, 0});
            bgEntries.Add({1, nullptr, first.Texture->GetView(), nullptr, 0, 0});
            bgEntries.Add({2, nullptr, nullptr, mSampler.Get(), 0, 0});
            RefPtr<RHI::RHIBindGroup> created = mDevice->CreateBindGroup(mBindGroupLayoutDesc, bgEntries);
            if (!created)
            {
                start = end;
                continue;
            }
            mBindGroupCache.Add(texKey, created);
            bindGroup = mBindGroupCache.Find(texKey);
        }
        encoder->SetBindGroup(0, bindGroup->Get());
        encoder->SetPipeline(first.Blend == RHI::BlendMode::Additive ? mAdditivePipeline.Get() : mAlphaPipeline.Get());
        encoder->SetVertexBuffer(0, mVertexBuffer.Get(), 0);
        encoder->SetIndexBuffer(mIndexBuffer.Get(), 0);
        encoder->DrawIndexed(count * 6, 1, start * 6, 0, 0);

        start = end;
    }
}

}
