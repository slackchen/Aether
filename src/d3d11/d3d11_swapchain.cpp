#include "d3d11/d3d11_internal.h"
#include <cstdio>
#include <cstring>

namespace aether::d3d11 {

D3D11Swapchain::D3D11Swapchain(ComPtr<ID3D11Device> dev, ComPtr<IDXGISwapChain1> sc, HWND hwnd, u32 width, u32 height)
    : device(std::move(dev)), swapchain(std::move(sc)), hwnd(hwnd), w(width), h(height) {}

rhi::RHITextureView* D3D11Swapchain::get_current_view() {
    if (!swapchain) return nullptr;

    u32 cw = w, ch = h;
    if (hwnd) {
        RECT client = {};
        GetClientRect(hwnd, &client);
        if (client.right - client.left > 0 && client.bottom - client.top > 0) {
            cw = (u32)(client.right - client.left);
            ch = (u32)(client.bottom - client.top);
        }
    }

    if (!rtv || rtv_w != cw || rtv_h != ch) {
        backbuffer.Reset();
        rtv.Reset();
        if (swapchain->ResizeBuffers(2, cw, ch, DXGI_FORMAT_B8G8R8A8_UNORM, 0) != S_OK) {
            printf("D3D11: swapchain ResizeBuffers failed\n");
            return nullptr;
        }
        if (swapchain->GetBuffer(0, IID_PPV_ARGS(&backbuffer)) != S_OK) {
            printf("D3D11: failed to get back buffer\n");
            return nullptr;
        }
        if (device->CreateRenderTargetView(backbuffer.Get(), nullptr, &rtv) != S_OK) {
            printf("D3D11: failed to create render target view\n");
            return nullptr;
        }
        rtv_w = cw;
        rtv_h = ch;
        w = cw;
        h = ch;
        configured = true;
    }

    static D3D11TextureView wrapper;
    wrapper.srv.Reset();
    wrapper.rtv = rtv;
    return &wrapper;
}

void D3D11Swapchain::toggle_fullscreen() {
    if (!swapchain) return;
    BOOL is_fullscreen = FALSE;
    swapchain->GetFullscreenState(&is_fullscreen, nullptr);
    HRESULT hr = swapchain->SetFullscreenState(is_fullscreen ? FALSE : TRUE, nullptr);
    if (FAILED(hr)) {
        printf("D3D11: SetFullscreenState failed (0x%08lx)\n", (unsigned long)hr);
        return;
    }
    backbuffer.Reset();
    rtv.Reset();
    rtv_w = 0;
    rtv_h = 0;
    if (is_fullscreen && hwnd) {
        RECT client = {};
        GetClientRect(hwnd, &client);
        w = (u32)(client.right - client.left);
        h = (u32)(client.bottom - client.top);
    }
}

bool D3D11Swapchain::present() {
    if (!swapchain) return false;

    static u64 s_frame = 0;
    static ComPtr<ID3D11Texture2D> staging;
    if (backbuffer && !staging) {
        D3D11_TEXTURE2D_DESC td = {};
        backbuffer->GetDesc(&td);
        td.Usage = D3D11_USAGE_STAGING;
        td.BindFlags = 0;
        td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        device->CreateTexture2D(&td, nullptr, &staging);
    }
    if (backbuffer && staging && s_frame % 30 == 0) {
        ComPtr<ID3D11DeviceContext> ctx;
        device->GetImmediateContext(&ctx);
        ctx->CopyResource(staging.Get(), backbuffer.Get());
        D3D11_MAPPED_SUBRESOURCE m = {};
        if (SUCCEEDED(ctx->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &m))) {
            u8* p = (u8*)m.pData;
            u8* c = p + (size_t)(h / 2) * m.RowPitch + (size_t)(w / 2) * 4;
            u8* tl = p + (size_t)20 * m.RowPitch + (size_t)20 * 4;
            u8* mr = p + (size_t)(h / 2) * m.RowPitch + (size_t)(w - 20) * 4;
            printf("readback f%llu center=%u,%u,%u tl=%u,%u,%u midright=%u,%u,%u\n",
                   (unsigned long long)s_frame, c[0], c[1], c[2], tl[0], tl[1], tl[2], mr[0], mr[1], mr[2]);
            fflush(stdout);
            ctx->Unmap(staging.Get(), 0);
        }
    }
    s_frame++;

    HRESULT hr = swapchain->Present(1, 0);
    if (FAILED(hr)) {
        printf("D3D11: present failed (0x%08lx)\n", (unsigned long)hr);
        return false;
    }
    return true;
}

}
