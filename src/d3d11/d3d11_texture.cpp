#include "d3d11/d3d11_internal.h"
#include <cstdio>

namespace aether::d3d11 {

rhi::RHITextureView* D3D11Texture::get_view() {
    return texture_view.get();
}

}
