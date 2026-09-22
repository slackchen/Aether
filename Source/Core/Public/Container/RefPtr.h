#pragma once

#include "Core.h"
#include "Threading/Atomic.h"

#include <concepts>
#include <utility>

namespace Aether {

//
// Intrusive reference-count base (COM style). Classes managed by RefPtr
// derive from this.
//
class RefCounted
{
public:
    RefCounted() = default;
    RefCounted(const RefCounted&) = delete;
    RefCounted& operator=(const RefCounted&) = delete;

    void AddRef() const { mRefCount.FetchAddRelaxed(1); }

    void Release() const
    {
        if (mRefCount.FetchSubAcqRel(1) == 1)
        {
            delete const_cast<RefCounted*>(this);
        }
    }

    u32 GetRefCount() const { return mRefCount.LoadRelaxed(); }

protected:
    virtual ~RefCounted() = default;

private:
    mutable Atomic<u32> mRefCount{0};
};

//
// Shared owning pointer to a RefCounted object.
//
template<typename T>
class RefPtr
{
public:
    RefPtr() = default;

    RefPtr(std::nullptr_t) {}

    explicit RefPtr(T* pointer)
        : mPointer(pointer)
    {
        if (mPointer)
        {
            mPointer->AddRef();
        }
    }

    RefPtr(const RefPtr& other)
        : RefPtr(other.mPointer)
    {
    }

    template<typename U> requires std::convertible_to<U*, T*>
    RefPtr(const RefPtr<U>& other)
        : RefPtr(other.Get())
    {
    }

    RefPtr(RefPtr&& other) noexcept
        : mPointer(other.mPointer)
    {
        other.mPointer = nullptr;
    }

    template<typename U> requires std::convertible_to<U*, T*>
    RefPtr(RefPtr<U>&& other) noexcept
        : mPointer(other.Release())
    {
    }

    RefPtr& operator=(const RefPtr& other)
    {
        if (this != &other)
        {
            Reset(other.mPointer);
        }
        return *this;
    }

    template<typename U> requires std::convertible_to<U*, T*>
    RefPtr& operator=(const RefPtr<U>& other)
    {
        if ((const void*)this != (const void*)&other)
        {
            Reset(other.Get());
        }
        return *this;
    }

    RefPtr& operator=(RefPtr&& other) noexcept
    {
        if (this != &other)
        {
            if (mPointer)
            {
                mPointer->Release();
            }
            mPointer = other.mPointer;
            other.mPointer = nullptr;
        }
        return *this;
    }

    ~RefPtr()
    {
        if (mPointer)
        {
            mPointer->Release();
        }
    }

    void Reset(T* pointer = nullptr)
    {
        if (pointer == mPointer)
        {
            return;
        }
        if (pointer)
        {
            pointer->AddRef();
        }
        if (mPointer)
        {
            mPointer->Release();
        }
        mPointer = pointer;
    }

    // Releases ownership without decrementing; returns the raw pointer.
    T* Release()
    {
        T* pointer = mPointer;
        mPointer = nullptr;
        return pointer;
    }

    void Swap(RefPtr& other)
    {
        T* tmp = mPointer;
        mPointer = other.mPointer;
        other.mPointer = tmp;
    }

    T* Get() const { return mPointer; }
    T* operator->() const
    {
        AETHER_ASSERT(mPointer);
        return mPointer;
    }
    T& operator*() const
    {
        AETHER_ASSERT(mPointer);
        return *mPointer;
    }

    bool IsValid() const { return mPointer != nullptr; }
    explicit operator bool() const { return mPointer != nullptr; }

    bool operator==(const RefPtr& other) const { return mPointer == other.mPointer; }
    bool operator!=(const RefPtr& other) const { return mPointer != other.mPointer; }
    bool operator==(std::nullptr_t) const { return mPointer == nullptr; }
    bool operator!=(std::nullptr_t) const { return mPointer != nullptr; }

private:
    T* mPointer = nullptr;
};

template<typename T, typename... Args>
RefPtr<T> MakeRef(Args&&... args)
{
    return RefPtr<T>(new T(std::forward<Args>(args)...));
}

// Static downcast between RefPtr types (e.g. interface to backend impl).
template<typename Dst, typename Src>
RefPtr<Dst> StaticCastRef(const RefPtr<Src>& source)
{
    return RefPtr<Dst>(static_cast<Dst*>(source.Get()));
}

template<typename T>
u64 HashValue(const RefPtr<T>& pointer)
{
    return HashValue(pointer.Get());
}

//
// Move-only owning pointer.
//
template<typename T>
class UniquePtr
{
public:
    UniquePtr() = default;

    explicit UniquePtr(T* pointer)
        : mPointer(pointer)
    {
    }

    UniquePtr(std::nullptr_t)
        : mPointer(nullptr)
    {
    }

    UniquePtr(const UniquePtr&) = delete;
    UniquePtr& operator=(const UniquePtr&) = delete;

    UniquePtr(UniquePtr&& other) noexcept
        : mPointer(other.mPointer)
    {
        other.mPointer = nullptr;
    }

    UniquePtr& operator=(UniquePtr&& other) noexcept
    {
        if (this != &other)
        {
            Reset(other.Release());
        }
        return *this;
    }

    // Converting move (Derived -> Base upcast), mirrors RefPtr.
    template<typename U>
        requires std::convertible_to<U*, T*>
    UniquePtr(UniquePtr<U>&& other) noexcept
        : mPointer(other.Release())
    {
    }

    template<typename U>
        requires std::convertible_to<U*, T*>
    UniquePtr& operator=(UniquePtr<U>&& other) noexcept
    {
        if (static_cast<void*>(this) != static_cast<void*>(&other))
        {
            Reset(other.Release());
        }
        return *this;
    }

    ~UniquePtr()
    {
        delete mPointer;
    }

    void Reset(T* pointer = nullptr)
    {
        T* old = mPointer;
        mPointer = pointer;
        delete old;
    }

    // Releases ownership without deleting; returns the raw pointer.
    T* Release()
    {
        T* pointer = mPointer;
        mPointer = nullptr;
        return pointer;
    }

    void Swap(UniquePtr& other)
    {
        T* tmp = mPointer;
        mPointer = other.mPointer;
        other.mPointer = tmp;
    }

    T* Get() const { return mPointer; }
    T* operator->() const
    {
        AETHER_ASSERT(mPointer);
        return mPointer;
    }
    T& operator*() const
    {
        AETHER_ASSERT(mPointer);
        return *mPointer;
    }

    bool IsValid() const { return mPointer != nullptr; }
    explicit operator bool() const { return mPointer != nullptr; }

    bool operator==(std::nullptr_t) const { return mPointer == nullptr; }
    bool operator!=(std::nullptr_t) const { return mPointer != nullptr; }

private:
    T* mPointer = nullptr;
};

template<typename T, typename... Args>
UniquePtr<T> MakeUnique(Args&&... args)
{
    return UniquePtr<T>(new T(std::forward<Args>(args)...));
}

}
