#include "WebGPUInternal.h"

namespace Aether::WebGPU {

WebGPUBuffer::~WebGPUBuffer()
{
    if (mBuffer)
    {
        if (mMapped)
        {
            wgpuBufferUnmap(mBuffer);
        }
        wgpuBufferDestroy(mBuffer);
        wgpuBufferRelease(mBuffer);
    }
}

void* WebGPUBuffer::Map()
{
    if (mMapped) return mMappedData;
    bool canMap = ((u32)mUsageFlags & ((u32)WGPUBufferUsage_MapRead | (u32)WGPUBufferUsage_MapWrite)) != 0;
    if (!canMap)
    {
        return nullptr;
    }
    mMappedData = wgpuBufferGetMappedRange(mBuffer, 0, mBufferSize);
    mMapped = true;
    return mMappedData;
}

void WebGPUBuffer::Unmap()
{
    if (!mMapped) return;
    wgpuBufferUnmap(mBuffer);
    mMappedData = nullptr;
    mMapped = false;
}

}
