#include "d3d11/d3d11_internal.h"
#include <cstdio>
#include <vector>

namespace aether::d3d11 {

std::shared_ptr<rhi::RHIShader> D3D11Device::create_shader(rhi::ShaderStage stage, const rhi::ShaderModuleDesc& desc) {
    if (!device) return nullptr;

    const char* target = stage == rhi::ShaderStage::Vertex ? "vs_5_0" : "ps_5_0";
    const char* entry = desc.entry_point ? desc.entry_point : "main";

    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> error;
    HRESULT hr = D3DCompile(desc.code, (SIZE_T)desc.code_size, nullptr, nullptr, nullptr,
                            entry, target, 0, 0, &blob, &error);
    if (FAILED(hr)) {
        if (error) {
            printf("D3D11: shader compile failed: %s\n", (const char*)error->GetBufferPointer());
        } else {
            printf("D3D11: shader compile failed (0x%08lx)\n", (unsigned long)hr);
        }
        return nullptr;
    }

    auto shader = std::make_shared<D3D11Shader>();
    shader->stage = stage;
    shader->blob = blob;
    if (stage == rhi::ShaderStage::Vertex) {
        if (FAILED(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shader->vs))) {
            printf("D3D11: failed to create vertex shader\n");
            return nullptr;
        }
    } else {
        if (FAILED(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shader->ps))) {
            printf("D3D11: failed to create pixel shader\n");
            return nullptr;
        }
    }
    return shader;
}

}
