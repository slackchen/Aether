#include "D3D11Internal.h"
#include <cstdio>

namespace Aether::D3D11 {

D3D11CommandEncoder::D3D11CommandEncoder(ComPtr<ID3D11DeviceContext> context, f32 width, f32 height)
    : mContext(std::move(context))
    , mViewportWidth(width)
    , mViewportHeight(height)
{
}

void D3D11CommandEncoder::BeginRenderPass(const RHI::RenderPassDesc& desc)
{
    ID3D11RenderTargetView* rtv = nullptr;
    ID3D11DepthStencilView* dsv = nullptr;
    if (desc.ColorAttachmentCount > 0 && desc.ColorAttachments)
    {
        const RHI::RenderPassColorAttachment& color = desc.ColorAttachments[0];
        D3D11TextureView* view = static_cast<D3D11TextureView*>(color.View);
        rtv = view ? view->mRtv.Get() : nullptr;
        if (rtv)
        {
            if (color.LoadOp == RHI::LoadOp::Clear)
            {
                mContext->ClearRenderTargetView(rtv, color.ClearColor);
            }
        }
    }
    if (desc.DepthAttachment && desc.DepthAttachment->View)
    {
        D3D11TextureView* dview = static_cast<D3D11TextureView*>(desc.DepthAttachment->View);
        dsv = dview->mDsv.Get();
        if (dsv && desc.DepthAttachment->LoadOp == RHI::LoadOp::Clear)
        {
            mContext->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, desc.DepthAttachment->ClearDepth, 0);
        }
    }
    mContext->OMSetRenderTargets(1, &rtv, dsv);
    D3D11_VIEWPORT vp = {};
    vp.Width = mViewportWidth;
    vp.Height = mViewportHeight;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    mContext->RSSetViewports(1, &vp);
}

void D3D11CommandEncoder::EndRenderPass() {}

void D3D11CommandEncoder::SetPipeline(RHI::RHIRenderPipeline* pipeline)
{
    D3D11RenderPipeline* p = static_cast<D3D11RenderPipeline*>(pipeline);
    if (!p)
    {
        printf("ENC: SetPipeline(NULL)\n");
        fflush(stdout);
        return;
    }
    static u32 sPipelineLogs = 0;
    if (sPipelineLogs < 6)
    {
        printf("ENC: pipeline vs=%d ps=%d il=%d rs=%d bs=%d stride=%u\n",
               p->mVertexShader != nullptr, p->mPixelShader != nullptr, p->mInputLayout != nullptr,
               p->mRasterizer != nullptr, p->mBlend != nullptr, p->mVertexStride);
        fflush(stdout);
        sPipelineLogs++;
    }
    mContext->IASetPrimitiveTopology(p->mTopology);
    mContext->IASetInputLayout(p->mInputLayout.Get());
    mContext->VSSetShader(p->mVertexShader.Get(), nullptr, 0);
    mContext->PSSetShader(p->mPixelShader.Get(), nullptr, 0);
    mContext->RSSetState(p->mRasterizer.Get());
    mContext->OMSetBlendState(p->mBlend.Get(), nullptr, 0xFFFFFFFF);
    mContext->OMSetDepthStencilState(p->mDepthStencil.Get(), 0);
    mVertexStride = p->mVertexStride;
}

void D3D11CommandEncoder::SetBindGroup(u32 groupIndex, RHI::RHIBindGroup* bindGroup)
{
    if (groupIndex != 0) return;
    D3D11BindGroup* bg = static_cast<D3D11BindGroup*>(bindGroup);
    if (!bg) return;

    static u32 sBindGroupLogs = 0;
    if (sBindGroupLogs < 6)
    {
        printf("BG: cb=%d srv=%d smp=%d\n", bg->mConstantBuffer != nullptr,
               bg->mShaderResourceView != nullptr, bg->mSampler != nullptr);
        fflush(stdout);
        sBindGroupLogs++;
    }

    if (bg->mConstantBuffer)
    {
        ID3D11Buffer* cb = bg->mConstantBuffer.Get();
        mContext->VSSetConstantBuffers(0, 1, &cb);
        mContext->PSSetConstantBuffers(0, 1, &cb);
    }
    if (bg->mShaderResourceView)
    {
        ID3D11ShaderResourceView* srv = bg->mShaderResourceView.Get();
        mContext->PSSetShaderResources(1, 1, &srv);
    }
    if (bg->mSampler)
    {
        ID3D11SamplerState* smp = bg->mSampler.Get();
        mContext->PSSetSamplers(2, 1, &smp);
    }
}

void D3D11CommandEncoder::SetVertexBuffer(u32 slot, RHI::RHIBuffer* buffer, u64 offset)
{
    D3D11Buffer* buf = static_cast<D3D11Buffer*>(buffer);
    if (!buf || !buf->mBuffer) return;
    ID3D11Buffer* vb = buf->mBuffer.Get();
    UINT stride = mVertexStride;
    UINT off = (UINT)offset;
    mContext->IASetVertexBuffers(slot, 1, &vb, &stride, &off);
}

void D3D11CommandEncoder::SetIndexBuffer(RHI::RHIBuffer* buffer, u64 offset, RHI::IndexFormat format)
{
    D3D11Buffer* buf = static_cast<D3D11Buffer*>(buffer);
    if (!buf || !buf->mBuffer) return;
    DXGI_FORMAT fmt = format == RHI::IndexFormat::Uint16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
    mContext->IASetIndexBuffer(buf->mBuffer.Get(), fmt, (UINT)offset);
}

void D3D11CommandEncoder::Draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance)
{
    static u32 sDrawLogs = 0;
    if (sDrawLogs < 6)
    {
        printf("ENC: draw %u verts %u inst\n", vertexCount, instanceCount);
        fflush(stdout);
        sDrawLogs++;
    }
    mContext->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
}

void D3D11CommandEncoder::DrawIndexed(u32 indexCount, u32 instanceCount, u32 firstIndex, i32 vertexOffset, u32 firstInstance)
{
    static u32 sDrawIndexedLogs = 0;
    if (sDrawIndexedLogs < 6)
    {
        printf("ENC: DrawIndexed %u idx %u inst\n", indexCount, instanceCount);
        fflush(stdout);
        sDrawIndexedLogs++;
    }
    mContext->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void D3D11CommandEncoder::Finish() {}

void D3D11CommandEncoder::Submit() {}

}
