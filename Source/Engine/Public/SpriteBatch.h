#pragma once

#include "Container/Array.h"
#include "Container/HashMap.h"
#include "Container/RefPtr.h"
#include "Core.h"
#include "Math/Color.h"
#include "Math/Mat4.h"
#include "Math/Vec2.h"
#include "RHI.h"
#include "Threading/JobSystem.h"

namespace Aether::Engine {

struct Sprite
{
    RefPtr<RHI::RHITexture> Texture;
    Math::Vec2 Position{0.0f, 0.0f};
    Math::Vec2 Size{1.0f, 1.0f};
    Math::Color Color{1.0f, 1.0f, 1.0f, 1.0f};
    f32 Rotation = 0.0f;
    f32 Layer = 0.0f;
    RHI::BlendMode Blend = RHI::BlendMode::Alpha;
    Math::Vec2 Uv0{0.0f, 0.0f};
    Math::Vec2 Uv1{1.0f, 1.0f};
    bool IsCustomQuad = false;
    Math::Vec2 QuadPts[4];
};

//
// Accumulating sprite batch. Add/AddUv/AddQuad append to the calling
// thread's bin (Jobs::ThreadIndex), so render-prep running inside
// ParallelFor can push sprites from any worker without locks; the main
// thread's bin is bin 0. Render() merges bins in ascending bin order
// (deterministic), sorts by layer, builds vertices in parallel and submits
// on the calling thread.
//
class SpriteBatch
{
public:
    static constexpr u32 MAX_SPRITES = 16384;
    static constexpr u32 VERTICES_PER_SPRITE = 4;
    static constexpr u32 FLOATS_PER_VERTEX = 8;
    static constexpr u32 MAX_BINS = 64;

    bool Init(RHI::RHIDevice* device, RHI::Format colorFormat);
    void Clear();
    void Add(RefPtr<RHI::RHITexture> texture, const Math::Vec2& position, const Math::Vec2& size,
             const Math::Color& color, f32 rotation = 0.0f, f32 layer = 0.0f,
             RHI::BlendMode blend = RHI::BlendMode::Alpha);
    void AddUv(RefPtr<RHI::RHITexture> texture, const Math::Vec2& uv0, const Math::Vec2& uv1,
               const Math::Vec2& position, const Math::Vec2& size, const Math::Color& color,
               f32 rotation = 0.0f, f32 layer = 0.0f, RHI::BlendMode blend = RHI::BlendMode::Alpha);
    void AddQuad(RefPtr<RHI::RHITexture> texture,
                 const Math::Vec2& p0, const Math::Vec2& p1, const Math::Vec2& p2, const Math::Vec2& p3,
                 const Math::Color& color, f32 layer = 0.0f,
                 const Math::Vec2& uv0 = {0.0f, 0.0f}, const Math::Vec2& uv1 = {1.0f, 1.0f},
                 RHI::BlendMode blend = RHI::BlendMode::Alpha);

    void Render(RHI::RHICommandEncoder* encoder, const Math::Mat4& vp);
    u32 SpriteCount() const;

private:
    struct SpriteRef
    {
        u32 Bin;
        u32 Index;
    };

    Array<Sprite>& CurrentBin();
    void BuildQuad(const Sprite& sprite, f32* verts);

    RHI::RHIDevice* mDevice = nullptr;
    RefPtr<RHI::RHIBuffer> mVertexBuffer;
    RefPtr<RHI::RHIBuffer> mIndexBuffer;
    RefPtr<RHI::RHIBuffer> mUniformBuffer;
    RefPtr<RHI::RHISampler> mSampler;
    RefPtr<RHI::RHIRenderPipeline> mAlphaPipeline;
    RefPtr<RHI::RHIRenderPipeline> mAdditivePipeline;
    RHI::BindGroupLayoutDesc mBindGroupLayoutDesc;
    HashMap<const RHI::RHITexture*, RefPtr<RHI::RHIBindGroup>> mBindGroupCache;
    // bin 0 = main/external thread, 1..N = job system workers
    Array<Sprite> mBins[MAX_BINS];
    // render() 复用的临时空间, 避免每帧分配/整块拷贝
    Array<SpriteRef> mMergeScratch;
    Array<f32> mVertexScratch;
};

} // namespace Aether::Engine
