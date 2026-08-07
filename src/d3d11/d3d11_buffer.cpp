#include "d3d11/d3d11_internal.h"
#include <cstring>

namespace aether::d3d11 {

void* D3D11Buffer::map() {
    if (mapped_) return cpu_data.data();
    if (cpu_data.empty()) {
        cpu_data.resize((size_t)buffer_size, 0);
    }
    mapped_ = true;
    dirty_ = false;
    return cpu_data.data();
}

void D3D11Buffer::unmap() {
    if (!mapped_) return;
    mapped_ = false;
    if (dirty_ && buffer && ctx && !cpu_data.empty()) {
        ctx->UpdateSubresource(buffer.Get(), 0, nullptr, cpu_data.data(), 0, 0);
        dirty_ = false;
    }
}

}
