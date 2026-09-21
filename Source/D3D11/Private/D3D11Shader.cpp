#include "D3D11Internal.h"
#include <cstdio>

namespace Aether::D3D11 {

RefPtr<RHI::RHIShader> D3D11Device::CreateShader(RHI::ShaderStage stage, const RHI::ShaderModuleDesc& desc)
{
    if (!mDevice) return nullptr;

    const char* target = stage == RHI::ShaderStage::Vertex ? "vs_5_0" : "ps_5_0";
    const char* entry = desc.EntryPoint ? desc.EntryPoint : "main";

    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> error;
    HRESULT hr = D3DCompile(desc.Code, (SIZE_T)desc.CodeSize, nullptr, nullptr, nullptr,
                            entry, target, 0, 0, &blob, &error);
    if (FAILED(hr))
    {
        if (error)
        {
            printf("D3D11: shader compile failed: %s\n", (const char*)error->GetBufferPointer());
        }
        else
        {
            printf("D3D11: shader compile failed (0x%08lx)\n", (unsigned long)hr);
        }
        return nullptr;
    }

    RefPtr<D3D11Shader> shader = MakeRef<D3D11Shader>();
    shader->mStage = stage;
    shader->mBlob = blob;
    if (stage == RHI::ShaderStage::Vertex)
    {
        if (FAILED(mDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shader->mVertexShader)))
        {
            printf("D3D11: failed to create vertex shader\n");
            return nullptr;
        }
    }
    else
    {
        if (FAILED(mDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shader->mPixelShader)))
        {
            printf("D3D11: failed to create pixel shader\n");
            return nullptr;
        }
    }
    return shader;
}

}
