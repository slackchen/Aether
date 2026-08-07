#include "d3d11/d3d11_internal.h"
#include <cstdio>

namespace aether::d3d11 {

D3D11CommandEncoder::D3D11CommandEncoder(ComPtr<ID3D11DeviceContext> context, f32 width, f32 height)
    : ctx(std::move(context)), viewport_w(width), viewport_h(height) {}

void D3D11CommandEncoder::begin_render_pass(const rhi::RenderPassDesc& desc) {
    ID3D11RenderTargetView* rtv = nullptr;
    if (desc.color_attachment_count > 0 && desc.color_attachments) {
        auto* view = static_cast<D3D11TextureView*>(desc.color_attachments[0].view);
        rtv = view ? view->rtv.Get() : nullptr;
        if (rtv) {
            if (desc.color_attachments[0].load_op == rhi::LoadOp::Clear) {
                ctx->ClearRenderTargetView(rtv, desc.color_attachments[0].clear_color);
            }
            ctx->OMSetRenderTargets(1, &rtv, nullptr);
        }
    }
    D3D11_VIEWPORT vp = {};
    vp.Width = viewport_w;
    vp.Height = viewport_h;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    ctx->RSSetViewports(1, &vp);
}

void D3D11CommandEncoder::end_render_pass() {}

void D3D11CommandEncoder::set_pipeline(rhi::RHIRenderPipeline* pipeline) {
    auto* p = static_cast<D3D11RenderPipeline*>(pipeline);
    if (!p) {
        printf("ENC: set_pipeline(NULL)\n");
        fflush(stdout);
        return;
    }
    static u32 s_count = 0;
    if (s_count < 6) {
        printf("ENC: pipeline vs=%d ps=%d il=%d rs=%d bs=%d stride=%u\n",
               p->vs != nullptr, p->ps != nullptr, p->input_layout != nullptr,
               p->rasterizer != nullptr, p->blend != nullptr, p->vertex_stride);
        fflush(stdout);
        s_count++;
    }
    ctx->IASetPrimitiveTopology(p->topology);
    ctx->IASetInputLayout(p->input_layout.Get());
    ctx->VSSetShader(p->vs.Get(), nullptr, 0);
    ctx->PSSetShader(p->ps.Get(), nullptr, 0);
    ctx->RSSetState(p->rasterizer.Get());
    ctx->OMSetBlendState(p->blend.Get(), nullptr, 0xFFFFFFFF);
    vertex_stride = p->vertex_stride;
}

void D3D11CommandEncoder::set_bind_group(u32 group_index, rhi::RHIBindGroup* bind_group) {
    if (group_index != 0) return;
    auto* bg = static_cast<D3D11BindGroup*>(bind_group);
    if (!bg) return;

    static u32 s_n = 0;
    if (s_n < 6) {
        printf("BG: cb=%d srv=%d smp=%d\n", bg->constant_buffer != nullptr,
               bg->srv != nullptr, bg->sampler != nullptr);
        fflush(stdout);
        s_n++;
    }

    if (bg->constant_buffer) {
        ID3D11Buffer* cb = bg->constant_buffer.Get();
        ctx->VSSetConstantBuffers(0, 1, &cb);
        ctx->PSSetConstantBuffers(0, 1, &cb);
    }
    if (bg->srv) {
        ID3D11ShaderResourceView* srv = bg->srv.Get();
        ctx->PSSetShaderResources(1, 1, &srv);
    }
    if (bg->sampler) {
        ID3D11SamplerState* smp = bg->sampler.Get();
        ctx->PSSetSamplers(2, 1, &smp);
    }
}

void D3D11CommandEncoder::set_vertex_buffer(u32 slot, rhi::RHIBuffer* buffer, u64 offset) {
    auto* buf = static_cast<D3D11Buffer*>(buffer);
    if (!buf || !buf->buffer) return;
    ID3D11Buffer* vb = buf->buffer.Get();
    UINT stride = vertex_stride;
    UINT off = (UINT)offset;
    ctx->IASetVertexBuffers(slot, 1, &vb, &stride, &off);
}

void D3D11CommandEncoder::set_index_buffer(rhi::RHIBuffer* buffer, u64 offset, rhi::IndexFormat format) {
    auto* buf = static_cast<D3D11Buffer*>(buffer);
    if (!buf || !buf->buffer) return;
    DXGI_FORMAT fmt = format == rhi::IndexFormat::Uint16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
    ctx->IASetIndexBuffer(buf->buffer.Get(), fmt, (UINT)offset);
}

void D3D11CommandEncoder::draw(u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance) {
    static u32 s_count = 0;
    if (s_count < 6) {
        printf("ENC: draw %u verts %u inst\n", vertex_count, instance_count);
        fflush(stdout);
        s_count++;
    }
    ctx->DrawInstanced(vertex_count, instance_count, first_vertex, first_instance);
}

void D3D11CommandEncoder::draw_indexed(u32 index_count, u32 instance_count, u32 first_index, i32 vertex_offset, u32 first_instance) {
    static u32 s_count = 0;
    if (s_count < 6) {
        printf("ENC: draw_indexed %u idx %u inst\n", index_count, instance_count);
        fflush(stdout);
        s_count++;
    }
    ctx->DrawIndexedInstanced(index_count, instance_count, first_index, vertex_offset, first_instance);
}

void D3D11CommandEncoder::finish() {}

void D3D11CommandEncoder::submit() {}

}
