#include "D3D11Internal.h"

namespace Aether::D3D11 {

RHI::RHITextureView* D3D11Texture::GetView()
{
    return mView.Get();
}

}
