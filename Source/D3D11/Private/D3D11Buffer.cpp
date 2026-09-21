#include "D3D11Internal.h"

namespace Aether::D3D11 {

void* D3D11Buffer::Map()
{
    if (mMapped) return mCpuData.Data();
    if (mCpuData.IsEmpty())
    {
        mCpuData.Resize((u32)mSize);
    }
    mMapped = true;
    mDirty = false;
    return mCpuData.Data();
}

void D3D11Buffer::Unmap()
{
    if (!mMapped) return;
    mMapped = false;
    if (mDirty && mBuffer && mContext && !mCpuData.IsEmpty())
    {
        mContext->UpdateSubresource(mBuffer.Get(), 0, nullptr, mCpuData.Data(), 0, 0);
        mDirty = false;
    }
}

}
