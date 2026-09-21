#include "Mesh.h"

namespace Aether::Engine {

void Mesh::SetVertices(const f32* data, u32 vertexCount, u32 strideBytes,
                       const Array<RHI::VertexAttribute>& attributes)
{
    const u32 floatCount = vertexCount * strideBytes / sizeof(f32);
    mVertices.Resize(floatCount);
    for (u32 i = 0; i < floatCount; ++i)
    {
        mVertices[i] = data[i];
    }
    mVertexCount = vertexCount;
    mVertexLayout.Stride = strideBytes;
    mVertexLayout.AttributeCount = attributes.Count();
    mAttributes = attributes;
    mVertexLayout.Attributes = mAttributes.Data();
}

void Mesh::SetIndices(const u32* data, u32 indexCount)
{
    mIndices.Resize(indexCount);
    for (u32 i = 0; i < indexCount; ++i)
    {
        mIndices[i] = data[i];
    }
}

bool Mesh::Upload(RHI::RHIDevice* device)
{
    if (!device || mVertices.IsEmpty()) return false;

    RHI::BufferDesc vertexBufferDesc;
    vertexBufferDesc.Size = mVertices.Count() * sizeof(f32);
    vertexBufferDesc.Usage = (u32)RHI::BufferUsage::Vertex | (u32)RHI::BufferUsage::CopyDst;
    vertexBufferDesc.MemoryType = RHI::BufferMemoryType::DeviceLocal;
    mVertexBuffer = device->CreateBuffer(vertexBufferDesc, mVertices.Data());
    if (!mVertexBuffer) return false;

    if (!mIndices.IsEmpty())
    {
        RHI::BufferDesc indexBufferDesc;
        indexBufferDesc.Size = mIndices.Count() * sizeof(u32);
        indexBufferDesc.Usage = (u32)RHI::BufferUsage::Index | (u32)RHI::BufferUsage::CopyDst;
        indexBufferDesc.MemoryType = RHI::BufferMemoryType::DeviceLocal;
        mIndexBuffer = device->CreateBuffer(indexBufferDesc, mIndices.Data());
        if (!mIndexBuffer) return false;
    }
    return true;
}

}
