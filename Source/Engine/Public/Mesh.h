#pragma once

#include "Container/Array.h"
#include "Container/RefPtr.h"
#include "Core.h"
#include "RHI.h"

namespace Aether::Engine {

class Mesh : public RefCounted
{
public:
    void SetVertices(const f32* data, u32 vertexCount, u32 strideBytes,
                     const Array<RHI::VertexAttribute>& attributes);
    void SetIndices(const u32* data, u32 indexCount);

    bool Upload(RHI::RHIDevice* device);
    bool IsUploaded() const { return mVertexBuffer.IsValid(); }

    RefPtr<RHI::RHIBuffer> VertexBuffer() const { return mVertexBuffer; }
    RefPtr<RHI::RHIBuffer> IndexBuffer() const { return mIndexBuffer; }
    const RHI::VertexBufferLayout& VertexLayout() const { return mVertexLayout; }
    u32 VertexCount() const { return mVertexCount; }
    u32 IndexCount() const { return mIndices.Count(); }
    bool HasIndices() const { return !mIndices.IsEmpty(); }

private:
    Array<f32> mVertices;
    Array<u32> mIndices;
    u32 mVertexCount = 0;
    RHI::VertexBufferLayout mVertexLayout;
    Array<RHI::VertexAttribute> mAttributes;
    RefPtr<RHI::RHIBuffer> mVertexBuffer;
    RefPtr<RHI::RHIBuffer> mIndexBuffer;
};

}
