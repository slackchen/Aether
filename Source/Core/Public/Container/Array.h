#pragma once

#include "Core.h"

#include <initializer_list>
#include <new>
#include <type_traits>
#include <utility>

namespace Aether {

//
// Dynamic array. Removal by RemoveAt keeps order; RemoveAtSwap is O(1) but
// moves the last element into the hole.
//
template<typename T>
class Array
{
public:
    using Iterator = T*;
    using ConstIterator = const T*;

    Array() = default;

    Array(std::initializer_list<T> init)
    {
        Reserve((u32)init.size());
        for (const T& value : init)
        {
            Construct(mData + mCount, value);
            ++mCount;
        }
    }

    Array(const Array& other)
    {
        Reserve(other.mCount);
        for (u32 i = 0; i < other.mCount; ++i)
        {
            Construct(mData + i, other.mData[i]);
        }
        mCount = other.mCount;
    }

    Array(Array&& other) noexcept
        : mData(other.mData)
        , mCount(other.mCount)
        , mCapacity(other.mCapacity)
    {
        other.mData = nullptr;
        other.mCount = 0;
        other.mCapacity = 0;
    }

    Array& operator=(const Array& other)
    {
        if (this != &other)
        {
            DestroyAll();
            Reserve(other.mCount);
            for (u32 i = 0; i < other.mCount; ++i)
            {
                Construct(mData + i, other.mData[i]);
            }
            mCount = other.mCount;
        }
        return *this;
    }

    Array& operator=(Array&& other) noexcept
    {
        if (this != &other)
        {
            DestroyAll();
            mData = other.mData;
            mCount = other.mCount;
            mCapacity = other.mCapacity;
            other.mData = nullptr;
            other.mCount = 0;
            other.mCapacity = 0;
        }
        return *this;
    }

    ~Array()
    {
        DestroyAll();
    }

    void Reserve(u32 capacity)
    {
        if (capacity <= mCapacity)
        {
            return;
        }
        u32 newCapacity = mCapacity ? mCapacity : 8;
        while (newCapacity < capacity)
        {
            newCapacity *= 2;
        }
        AETHER_ASSERT((u64)newCapacity * sizeof(T) < (u64)1 << 31);

        T* newData = static_cast<T*>(::operator new((size_t)newCapacity * sizeof(T)));
        for (u32 i = 0; i < mCount; ++i)
        {
            RelocateTo(newData + i, mData + i);
        }
        ::operator delete(mData);
        mData = newData;
        mCapacity = newCapacity;
    }

    void Add(const T& value)
    {
        Reserve(mCount + 1);
        Construct(mData + mCount, value);
        ++mCount;
    }

    void Add(T&& value)
    {
        Reserve(mCount + 1);
        Construct(mData + mCount, std::move(value));
        ++mCount;
    }

    template<typename... Args>
    T& EmplaceAdd(Args&&... args)
    {
        Reserve(mCount + 1);
        T* element = new (static_cast<void*>(mData + mCount)) T(std::forward<Args>(args)...);
        ++mCount;
        return *element;
    }

    void AddRange(const Array& other)
    {
        Reserve(mCount + other.mCount);
        for (u32 i = 0; i < other.mCount; ++i)
        {
            Construct(mData + mCount + i, other.mData[i]);
        }
        mCount += other.mCount;
    }

    // Stable removal: shifts elements after the index.
    void RemoveAt(u32 index)
    {
        AETHER_ASSERT(index < mCount);
        for (u32 i = index; i + 1 < mCount; ++i)
        {
            mData[i] = std::move(mData[i + 1]);
        }
        mData[mCount - 1].~T();
        --mCount;
    }

    // O(1) removal: moves the last element into the slot. Order is not kept.
    void RemoveAtSwap(u32 index)
    {
        AETHER_ASSERT(index < mCount);
        if (index != mCount - 1)
        {
            mData[index] = std::move(mData[mCount - 1]);
        }
        mData[mCount - 1].~T();
        --mCount;
    }

    void RemoveLast()
    {
        AETHER_ASSERT(mCount > 0);
        mData[mCount - 1].~T();
        --mCount;
    }

    // Removes the first occurrence equal to value. Returns whether anything was removed.
    bool Remove(const T& value)
    {
        i32 index = Find(value);
        if (index >= 0)
        {
            RemoveAt((u32)index);
            return true;
        }
        return false;
    }

    // Stable compaction: removes all elements matching pred, keeps relative order.
    template<typename Pred>
    u32 RemoveIf(Pred pred)
    {
        u32 write = 0;
        for (u32 i = 0; i < mCount; ++i)
        {
            if (pred(mData[i]))
            {
                mData[i].~T();
            }
            else
            {
                if (write != i)
                {
                    mData[write] = std::move(mData[i]);
                }
                ++write;
            }
        }
        u32 removed = mCount - write;
        mCount = write;
        return removed;
    }

    void Insert(u32 index, const T& value)
    {
        AETHER_ASSERT(index <= mCount);
        if (index == mCount)
        {
            Add(value);
            return;
        }
        Reserve(mCount + 1);
        Construct(mData + mCount, std::move(mData[mCount - 1]));
        for (u32 i = mCount - 1; i > index; --i)
        {
            mData[i] = std::move(mData[i - 1]);
        }
        mData[index] = value;
        ++mCount;
    }

    void Clear()
    {
        for (u32 i = 0; i < mCount; ++i)
        {
            mData[i].~T();
        }
        mCount = 0;
    }

    // Value-initializes new elements (zero for POD).
    void Resize(u32 count)
    {
        if (count > mCount)
        {
            Reserve(count);
            for (u32 i = mCount; i < count; ++i)
            {
                ::new (static_cast<void*>(mData + i)) T();
            }
        }
        else if (count < mCount)
        {
            for (u32 i = count; i < mCount; ++i)
            {
                mData[i].~T();
            }
        }
        mCount = count;
    }

    void Resize(u32 count, const T& fill)
    {
        if (count > mCount)
        {
            Reserve(count);
            for (u32 i = mCount; i < count; ++i)
            {
                Construct(mData + i, fill);
            }
        }
        else if (count < mCount)
        {
            for (u32 i = count; i < mCount; ++i)
            {
                mData[i].~T();
            }
        }
        mCount = count;
    }

    u32 Count() const { return mCount; }
    u32 Capacity() const { return mCapacity; }
    bool IsEmpty() const { return mCount == 0; }

    T* Data() { return mData; }
    const T* Data() const { return mData; }

    T& operator[](u32 index)
    {
        AETHER_ASSERT(index < mCount);
        return mData[index];
    }

    const T& operator[](u32 index) const
    {
        AETHER_ASSERT(index < mCount);
        return mData[index];
    }

    T& First()
    {
        AETHER_ASSERT(mCount > 0);
        return mData[0];
    }

    const T& First() const
    {
        AETHER_ASSERT(mCount > 0);
        return mData[0];
    }

    T& Last()
    {
        AETHER_ASSERT(mCount > 0);
        return mData[mCount - 1];
    }

    const T& Last() const
    {
        AETHER_ASSERT(mCount > 0);
        return mData[mCount - 1];
    }

    i32 Find(const T& value) const
    {
        for (u32 i = 0; i < mCount; ++i)
        {
            if (mData[i] == value)
            {
                return (i32)i;
            }
        }
        return -1;
    }

    template<typename Pred>
    i32 FindIf(Pred pred) const
    {
        for (u32 i = 0; i < mCount; ++i)
        {
            if (pred(mData[i]))
            {
                return (i32)i;
            }
        }
        return -1;
    }

    bool Contains(const T& value) const { return Find(value) >= 0; }

    void Sort() { Sort([](const T& a, const T& b) { return a < b; }); }

    template<typename Pred>
    void Sort(Pred pred)
    {
        if (mCount > 1)
        {
            QuickSort(0, (i64)mCount - 1, pred);
        }
    }

    Iterator begin() { return mData; }
    Iterator end() { return mData + mCount; }
    ConstIterator begin() const { return mData; }
    ConstIterator end() const { return mData + mCount; }

private:
    static void Construct(T* dst, const T& value) { ::new (static_cast<void*>(dst)) T(value); }
    static void Construct(T* dst, T&& value) { ::new (static_cast<void*>(dst)) T(std::move(value)); }

    static void RelocateTo(T* dst, T* src)
    {
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>)
        {
            ::new (static_cast<void*>(dst)) T(std::move(*src));
        }
        else
        {
            ::new (static_cast<void*>(dst)) T(*src);
        }
        src->~T();
    }

    void DestroyAll()
    {
        for (u32 i = 0; i < mCount; ++i)
        {
            mData[i].~T();
        }
        ::operator delete(mData);
        mData = nullptr;
        mCount = 0;
        mCapacity = 0;
    }

    template<typename Pred>
    void QuickSort(i64 lo, i64 hi, Pred& pred)
    {
        while (lo < hi)
        {
            if (hi - lo < 16)
            {
                InsertionSort(lo, hi, pred);
                return;
            }

            i64 mid = lo + (hi - lo) / 2;
            if (pred(mData[(u32)mid], mData[(u32)lo]))
            {
                SwapElems((u32)lo, (u32)mid);
            }
            if (pred(mData[(u32)hi], mData[(u32)lo]))
            {
                SwapElems((u32)lo, (u32)hi);
            }
            if (pred(mData[(u32)hi], mData[(u32)mid]))
            {
                SwapElems((u32)mid, (u32)hi);
            }

            T pivot = mData[(u32)mid];
            i64 i = lo;
            i64 j = hi;
            while (i <= j)
            {
                while (pred(mData[(u32)i], pivot))
                {
                    ++i;
                }
                while (pred(pivot, mData[(u32)j]))
                {
                    --j;
                }
                if (i <= j)
                {
                    SwapElems((u32)i, (u32)j);
                    ++i;
                    --j;
                }
            }

            if (j - lo < hi - i)
            {
                QuickSort(lo, j, pred);
                lo = i;
            }
            else
            {
                QuickSort(i, hi, pred);
                hi = j;
            }
        }
    }

    template<typename Pred>
    void InsertionSort(i64 lo, i64 hi, Pred& pred)
    {
        for (i64 i = lo + 1; i <= hi; ++i)
        {
            T value = std::move(mData[(u32)i]);
            i64 j = i - 1;
            while (j >= lo && pred(value, mData[(u32)j]))
            {
                mData[(u32)(j + 1)] = std::move(mData[(u32)j]);
                --j;
            }
            mData[(u32)(j + 1)] = std::move(value);
        }
    }

    void SwapElems(u32 a, u32 b)
    {
        T tmp = std::move(mData[a]);
        mData[a] = std::move(mData[b]);
        mData[b] = std::move(tmp);
    }

    T* mData = nullptr;
    u32 mCount = 0;
    u32 mCapacity = 0;
};

}
