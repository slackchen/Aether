#include "D3D11Internal.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace Aether::D3D11 {

D3D11Swapchain::D3D11Swapchain(ComPtr<ID3D11Device> device, ComPtr<IDXGISwapChain1> swapchain, HWND hwnd, u32 width, u32 height)
    : mDevice(std::move(device))
    , mSwapchain(std::move(swapchain))
    , mHwnd(hwnd)
    , mWidth(width)
    , mHeight(height)
{
}

RHI::RHITextureView* D3D11Swapchain::GetCurrentView()
{
    if (!mSwapchain) return nullptr;

    auto writeResizeLog = [](const char* fmt, ...) {
        FILE* log = fopen("d3d11_resize.log", "a");
        if (!log) return;
        va_list args;
        va_start(args, fmt);
        vfprintf(log, fmt, args);
        va_end(args);
        fclose(log);
    };

    u32 cw = mWidth, ch = mHeight;
    if (mHwnd)
    {
        RECT client = {};
        GetClientRect(mHwnd, &client);
        if (client.right - client.left > 0 && client.bottom - client.top > 0)
        {
            cw = (u32)(client.right - client.left);
            ch = (u32)(client.bottom - client.top);
        }
    }

    static D3D11TextureView sColorViewWrapper;
    if (!mRtv || mRtvWidth != cw || mRtvHeight != ch)
    {
        mBackBuffer.Reset();
        mRtv.Reset();
        sColorViewWrapper.mSrv.Reset();
        sColorViewWrapper.mRtv.Reset();
        ComPtr<ID3D11DeviceContext> ctx;
        mDevice->GetImmediateContext(&ctx);
        if (ctx)
        {
            ID3D11RenderTargetView* nullRtv[1] = {nullptr};
            ctx->OMSetRenderTargets(1, nullRtv, nullptr);
            ctx->Flush();
        }
        writeResizeLog("resize attempt %ux%u\n", cw, ch);
        HRESULT hr = mSwapchain->ResizeBuffers(2, cw, ch, DXGI_FORMAT_UNKNOWN, 0);
        if (FAILED(hr))
        {
            writeResizeLog("ResizeBuffers(%u,%u) failed 0x%08lx\n", cw, ch, (unsigned long)hr);
            printf("D3D11: swapchain ResizeBuffers failed (0x%08lx)\n", (unsigned long)hr);
            return nullptr;
        }
        if (mSwapchain->GetBuffer(0, IID_PPV_ARGS(&mBackBuffer)) != S_OK)
        {
            writeResizeLog("GetBuffer(0) failed\n");
            printf("D3D11: failed to get back buffer\n");
            return nullptr;
        }
        D3D11_TEXTURE2D_DESC td = {};
        mBackBuffer->GetDesc(&td);
        cw = td.Width;
        ch = td.Height;
        if (mDevice->CreateRenderTargetView(mBackBuffer.Get(), nullptr, &mRtv) != S_OK)
        {
            writeResizeLog("CreateRenderTargetView failed\n");
            printf("D3D11: failed to create render target view\n");
            return nullptr;
        }
        writeResizeLog("resize OK %ux%u\n", cw, ch);
        mRtvWidth = cw;
        mRtvHeight = ch;
        mWidth = cw;
        mHeight = ch;
        mConfigured = true;
        EnsureDepthBuffer();
        printf("D3D11: swapchain resized to %ux%u\n", mWidth, mHeight);
        fflush(stdout);
    }

    sColorViewWrapper.mSrv.Reset();
    sColorViewWrapper.mRtv = mRtv;
    return &sColorViewWrapper;
}

void D3D11Swapchain::EnsureDepthBuffer()
{
    if (!mDevice) return;
    if (mDsv && mDepthBuffer)
    {
        D3D11_TEXTURE2D_DESC dd = {};
        mDepthBuffer->GetDesc(&dd);
        if (dd.Width == mWidth && dd.Height == mHeight) return;
    }
    mDepthBuffer.Reset();
    mDsv.Reset();

    D3D11_TEXTURE2D_DESC td = {};
    td.Width = mWidth;
    td.Height = mHeight;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_D32_FLOAT;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    if (FAILED(mDevice->CreateTexture2D(&td, nullptr, &mDepthBuffer)))
    {
        printf("D3D11: failed to create depth buffer\n");
        return;
    }
    if (FAILED(mDevice->CreateDepthStencilView(mDepthBuffer.Get(), nullptr, &mDsv)))
    {
        printf("D3D11: failed to create depth stencil view\n");
        mDepthBuffer.Reset();
        return;
    }
}

RHI::RHITextureView* D3D11Swapchain::GetDepthView()
{
    EnsureDepthBuffer();
    if (!mDsv) return nullptr;
    static D3D11TextureView sDepthViewWrapper;
    sDepthViewWrapper.mSrv.Reset();
    sDepthViewWrapper.mRtv.Reset();
    sDepthViewWrapper.mDsv = mDsv;
    return &sDepthViewWrapper;
}

void D3D11Swapchain::ToggleFullscreen()
{
    if (!mSwapchain || !mHwnd) return;

    FILE* log = fopen("d3d11_resize.log", "a");
    if (log)
    {
        fprintf(log, "ToggleFullscreen called borderless=%d\n", (int)mBorderless);
        fclose(log);
    }

    if (!mBorderless)
    {
        if (mSavedStyle == 0)
        {
            mSavedStyle = GetWindowLongPtr(mHwnd, GWL_STYLE);
            GetWindowRect(mHwnd, &mSavedRect);
        }
        RECT work = {};
        if (SystemParametersInfo(SPI_GETWORKAREA, 0, &work, 0) && work.right > work.left && work.bottom > work.top)
        {
            SetWindowLongPtr(mHwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
            SetWindowPos(mHwnd, HWND_TOP, work.left, work.top,
                         work.right - work.left, work.bottom - work.top,
                         SWP_FRAMECHANGED | SWP_SHOWWINDOW);
            log = fopen("d3d11_resize.log", "a");
            if (log)
            {
                fprintf(log, "  -> borderless %d,%d %ux%u\n", work.left, work.top,
                        work.right - work.left, work.bottom - work.top);
                fclose(log);
            }
        }
        mBorderless = true;
    }
    else
    {
        SetWindowLongPtr(mHwnd, GWL_STYLE, mSavedStyle);
        SetWindowPos(mHwnd, HWND_NOTOPMOST, mSavedRect.left, mSavedRect.top,
                     mSavedRect.right - mSavedRect.left,
                     mSavedRect.bottom - mSavedRect.top,
                     SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        mSavedStyle = 0;
        mBorderless = false;
    }

    mBackBuffer.Reset();
    mRtv.Reset();
    mDepthBuffer.Reset();
    mDsv.Reset();
    mRtvWidth = 0;
    mRtvHeight = 0;
    mWidth = 0;
    mHeight = 0;

    log = fopen("d3d11_resize.log", "a");
    if (log)
    {
        fprintf(log, "  -> reset done\n");
        fclose(log);
    }
}

bool D3D11Swapchain::Present()
{
    if (!mSwapchain) return false;

    HRESULT hr = mSwapchain->Present(1, 0);
    if (FAILED(hr))
    {
        printf("D3D11: present failed (0x%08lx)\n", (unsigned long)hr);
        return false;
    }
    return true;
}

}
