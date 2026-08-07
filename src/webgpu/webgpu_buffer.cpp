#include "webgpu/webgpu_internal.h"

namespace aether::webgpu {

WebGPUBuffer::~WebGPUBuffer() {
    if (buffer) {
        if (is_mapped) {
            wgpuBufferUnmap(buffer);
        }
        wgpuBufferDestroy(buffer);
        wgpuBufferRelease(buffer);
    }
}

void* WebGPUBuffer::map() {
    if (is_mapped) return mapped_data;
    bool can_map = ((u32)usage_flags & ((u32)WGPUBufferUsage_MapRead | (u32)WGPUBufferUsage_MapWrite)) != 0;
    if (!can_map) {
        return nullptr;
    }
    mapped_data = wgpuBufferGetMappedRange(buffer, 0, buffer_size);
    is_mapped = true;
    return mapped_data;
}

void WebGPUBuffer::unmap() {
    if (!is_mapped) return;
    wgpuBufferUnmap(buffer);
    mapped_data = nullptr;
    is_mapped = false;
}

}
