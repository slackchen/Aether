#include "d3d11/d3d11_internal.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace aether::d3d11 {

D3D11Swapchain::D3D11Swapchain(ComPtr<ID3D11Device> dev, ComPtr<IDXGISwapChain1> sc, HWND hwnd, u32 width, u32 height)
    : device(std::move(dev)), swapchain(std::move(sc)), hwnd(hwnd), w(width), h(height) {}

rhi::RHITextureView* D3D11Swapchain::get_current_view() {
    if (!swapchain) return nullptr;

    auto logf = [](const char* fmt, ...) {
        FILE* log = fopen("d3d11_resize.log", "a");
        if (!log) return;
        va_list args;
        va_start(args, fmt);
        vfprintf(log, fmt, args);
        va_end(args);
        fclose(log);
    };

    u32 cw = w, ch = h;
    if (hwnd) {
        RECT client = {};
        GetClientRect(hwnd, &client);
        if (client.right - client.left > 0 && client.bottom - client.top > 0) {
            cw = (u32)(client.right - client.left);
            ch = (u32)(client.bottom - client.top);
        }
    }

    static D3D11TextureView wrapper;
    if (!rtv || rtv_w != cw || rtv_h != ch) {
        backbuffer.Reset();
        rtv.Reset();
        wrapper.srv.Reset();
        wrapper.rtv.Reset();
        ComPtr<ID3D11DeviceContext> ctx;
        device->GetImmediateContext(&ctx);
        if (ctx) {
            ID3D11RenderTargetView* null_rtv[1] = {nullptr};
            ctx->OMSetRenderTargets(1, null_rtv, nullptr);
            ctx->Flush();
        }
        logf("resize attempt %ux%u\n", cw, ch);
        HRESULT hr = swapchain->ResizeBuffers(2, cw, ch, DXGI_FORMAT_UNKNOWN, 0);
        if (FAILED(hr)) {
            logf("ResizeBuffers(%u,%u) failed 0x%08lx\n", cw, ch, (unsigned long)hr);
            printf("D3D11: swapchain ResizeBuffers failed (0x%08lx)\n", (unsigned long)hr);
            return nullptr;
        }
        if (swapchain->GetBuffer(0, IID_PPV_ARGS(&backbuffer)) != S_OK) {
            logf("GetBuffer(0) failed\n");
            printf("D3D11: failed to get back buffer\n");
            return nullptr;
        }
        D3D11_TEXTURE2D_DESC td = {};
        backbuffer->GetDesc(&td);
        cw = td.Width;
        ch = td.Height;
        if (device->CreateRenderTargetView(backbuffer.Get(), nullptr, &rtv) != S_OK) {
            logf("CreateRenderTargetView failed\n");
            printf("D3D11: failed to create render target view\n");
            return nullptr;
        }
        logf("resize OK %ux%u\n", cw, ch);
        rtv_w = cw;
        rtv_h = ch;
        w = cw;
        h = ch;
        configured = true;
        printf("D3D11: swapchain resized to %ux%u\n", w, h);
        fflush(stdout);
    }

    wrapper.srv.Reset();
    wrapper.rtv = rtv;
    return &wrapper;
}

void D3D11Swapchain::toggle_fullscreen() {
    if (!swapchain || !hwnd) return;

    FILE* log = fopen("d3d11_resize.log", "a");
    if (log) {
        fprintf(log, "toggle_fullscreen called borderless_=%d\n", (int)borderless_);
        fclose(log);
    }

    if (!borderless_) {
        if (saved_style_ == 0) {
            saved_style_ = GetWindowLongPtr(hwnd, GWL_STYLE);
            GetWindowRect(hwnd, &saved_rect_);
        }
        RECT work = {};
        if (SystemParametersInfo(SPI_GETWORKAREA, 0, &work, 0) && work.right > work.left && work.bottom > work.top) {
            SetWindowLongPtr(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
            SetWindowPos(hwnd, HWND_TOP, work.left, work.top,
                         work.right - work.left, work.bottom - work.top,
                         SWP_FRAMECHANGED | SWP_SHOWWINDOW);
            log = fopen("d3d11_resize.log", "a");
            if (log) {
                fprintf(log, "  -> borderless %d,%d %ux%u\n", work.left, work.top,
                        work.right - work.left, work.bottom - work.top);
                fclose(log);
            }
        }
        borderless_ = true;
    } else {
        SetWindowLongPtr(hwnd, GWL_STYLE, saved_style_);
        SetWindowPos(hwnd, HWND_NOTOPMOST, saved_rect_.left, saved_rect_.top,
                     saved_rect_.right - saved_rect_.left,
                     saved_rect_.bottom - saved_rect_.top,
                     SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        saved_style_ = 0;
        borderless_ = false;
    }

    backbuffer.Reset();
    rtv.Reset();
    rtv_w = 0;
    rtv_h = 0;
    w = 0;
    h = 0;

    log = fopen("d3d11_resize.log", "a");
    if (log) {
        fprintf(log, "  -> reset done\n");
        fclose(log);
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
