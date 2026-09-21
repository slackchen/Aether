#pragma once

#include "Container/Array.h"
#include "Container/String.h"
#include "Core.h"

#include <cstring>
#include <initializer_list>
#include <type_traits>
#include <utility>

namespace Aether {

//
// Hash helpers
//
inline u64 Mix64(u64 v)
{
    v ^= v >> 30;
    v *= 0xbf58476d1ce4e5b9ull;
    v ^= v >> 27;
    v *= 0x94d049bb133111ebull;
    v ^= v >> 31;
    return v;
}

inline u64 HashCombine(u64 a, u64 b)
{
    return Mix64(a ^ (b + 0x9e3779b97f4a7c15ull + (a << 6) + (a >> 2)));
}

template<typename T> requires std::is_integral_v<T>
u64 HashValue(T value)
{
    return Mix64((u64)value);
}

template<typename T> requires std::is_enum_v<T>
u64 HashValue(T value)
{
    return Mix64((u64)(std::underlying_type_t<T>)value);
}

template<typename T>
u64 HashValue(T* pointer)
{
    return Mix64((u64)pointer);
}

inline u64 HashValue(f32 value)
{
    u32 bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return Mix64(bits);
}

inline u64 HashValue(f64 value)
{
    u64 bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return Mix64(bits);
}

//
// Open-addressing hash map with linear probing. Iteration order is
// deterministic for the same sequence of operations, but not sorted.
//
template<typename K, typename V>
class HashMap
{
public:
    struct KVPair
    {
        K Key;
        V Value;
    };

private:
    static constexpr u8 SLOT_EMPTY = 0;
    static constexpr u8 SLOT_USED = 1;
    static constexpr u8 SLOT_TOMBSTONE = 2;

    struct Slot
    {
        u8 State = SLOT_EMPTY;
        KVPair Pair;
    };

public:
    class Iterator
    {
    public:
        explicit Iterator(Slot* slot, Slot* end)
            : mSlot(slot)
            , mEnd(end)
        {
            SkipUnused();
        }

        KVPair& operator*() const { return mSlot->Pair; }
        KVPair* operator->() const { return &mSlot->Pair; }

        Iterator& operator++()
        {
            ++mSlot;
            SkipUnused();
            return *this;
        }

        bool operator!=(const Iterator& other) const { return mSlot != other.mSlot; }
        bool operator==(const Iterator& other) const { return mSlot == other.mSlot; }

    private:
        void SkipUnused()
        {
            while (mSlot < mEnd && mSlot->State != SLOT_USED)
            {
                ++mSlot;
            }
        }

        Slot* mSlot;
        Slot* mEnd;
    };

    class ConstIterator
    {
    public:
        explicit ConstIterator(const Slot* slot, const Slot* end)
            : mSlot(slot)
            , mEnd(end)
        {
            SkipUnused();
        }

        const KVPair& operator*() const { return mSlot->Pair; }
        const KVPair* operator->() const { return &mSlot->Pair; }

        ConstIterator& operator++()
        {
            ++mSlot;
            SkipUnused();
            return *this;
        }

        bool operator!=(const ConstIterator& other) const { return mSlot != other.mSlot; }
        bool operator==(const ConstIterator& other) const { return mSlot == other.mSlot; }

    private:
        void SkipUnused()
        {
            while (mSlot < mEnd && mSlot->State != SLOT_USED)
            {
                ++mSlot;
            }
        }

        const Slot* mSlot;
        const Slot* mEnd;
    };

    HashMap() = default;

    HashMap(std::initializer_list<KVPair> init)
    {
        Reserve((u32)init.size());
        for (const KVPair& pair : init)
        {
            FindOrAdd(pair.Key) = pair.Value;
        }
    }

    u32 Count() const { return mCount; }
    bool IsEmpty() const { return mCount == 0; }

    void Reserve(u32 count)
    {
        u32 needed = (u32)((f64)count / LOAD_FACTOR) + 1;
        u32 capacity = 16;
        while (capacity < needed)
        {
            capacity *= 2;
        }
        if (capacity > mSlots.Count())
        {
            Rehash(capacity);
        }
    }

    void Clear()
    {
        for (Slot& slot : mSlots)
        {
            if (slot.State == SLOT_USED)
            {
                slot.Pair.~KVPair();
            }
            slot.State = SLOT_EMPTY;
        }
        mCount = 0;
    }

    V* Find(const K& key)
    {
        if (mSlots.IsEmpty())
        {
            return nullptr;
        }
        Slot* slots = mSlots.Data();
        u32 mask = mSlots.Count() - 1;
        u32 index = (u32)(HashValue(key) & mask);
        while (true)
        {
            Slot& slot = slots[index];
            if (slot.State == SLOT_EMPTY)
            {
                return nullptr;
            }
            if (slot.State == SLOT_USED && slot.Pair.Key == key)
            {
                return &slot.Pair.Value;
            }
            index = (index + 1) & mask;
        }
    }

    const V* Find(const K& key) const
    {
        return const_cast<HashMap*>(this)->Find(key);
    }

    bool Contains(const K& key) const { return Find(key) != nullptr; }

    // Returns the value for key, default-constructing it if missing.
    V& FindOrAdd(const K& key)
    {
        if (mSlots.IsEmpty() || (mCount + mTombstones + 1) * 10 >= mSlots.Count() * (u32)(LOAD_FACTOR * 10))
        {
            Grow();
        }
        Slot* slots = mSlots.Data();
        u32 mask = mSlots.Count() - 1;
        u32 index = (u32)(HashValue(key) & mask);
        Slot* tombstone = nullptr;
        while (true)
        {
            Slot& slot = slots[index];
            if (slot.State == SLOT_EMPTY)
            {
                Slot* target = tombstone ? tombstone : &slot;
                if (tombstone)
                {
                    --mTombstones;
                }
                ::new (static_cast<void*>(&target->Pair)) KVPair(key, V());
                target->State = SLOT_USED;
                ++mCount;
                return target->Pair.Value;
            }
            if (slot.State == SLOT_TOMBSTONE)
            {
                if (!tombstone)
                {
                    tombstone = &slot;
                }
            }
            else if (slot.Pair.Key == key)
            {
                return slot.Pair.Value;
            }
            index = (index + 1) & mask;
        }
    }

    V& operator[](const K& key) { return FindOrAdd(key); }

    // Inserts the pair, replacing any existing value under the same key.
    void Add(const K& key, const V& value) { FindOrAdd(key) = value; }
    void Add(const K& key, V&& value) { FindOrAdd(key) = std::move(value); }

    bool Remove(const K& key)
    {
        if (mSlots.IsEmpty())
        {
            return false;
        }
        Slot* slots = mSlots.Data();
        u32 mask = mSlots.Count() - 1;
        u32 index = (u32)(HashValue(key) & mask);
        while (true)
        {
            Slot& slot = slots[index];
            if (slot.State == SLOT_EMPTY)
            {
                return false;
            }
            if (slot.State == SLOT_USED && slot.Pair.Key == key)
            {
                slot.Pair.~KVPair();
                slot.State = SLOT_TOMBSTONE;
                --mCount;
                ++mTombstones;
                return true;
            }
            index = (index + 1) & mask;
        }
    }

    Iterator begin()
    {
        return Iterator(mSlots.IsEmpty() ? nullptr : mSlots.Data(), mSlots.IsEmpty() ? nullptr : mSlots.Data() + mSlots.Count());
    }

    Iterator end()
    {
        return Iterator(mSlots.IsEmpty() ? nullptr : mSlots.Data() + mSlots.Count(), mSlots.IsEmpty() ? nullptr : mSlots.Data() + mSlots.Count());
    }

    ConstIterator begin() const
    {
        return ConstIterator(mSlots.IsEmpty() ? nullptr : mSlots.Data(), mSlots.IsEmpty() ? nullptr : mSlots.Data() + mSlots.Count());
    }

    ConstIterator end() const
    {
        return ConstIterator(mSlots.IsEmpty() ? nullptr : mSlots.Data() + mSlots.Count(), mSlots.IsEmpty() ? nullptr : mSlots.Data() + mSlots.Count());
    }

private:
    static constexpr f64 LOAD_FACTOR = 0.7;

    void Grow()
    {
        u32 newCapacity = mSlots.IsEmpty() ? 16 : mSlots.Count() * 2;
        Rehash(newCapacity);
    }

    void Rehash(u32 capacity)
    {
        Array<Slot> oldSlots;
        oldSlots = std::move(mSlots);
        mSlots.Resize(capacity);
        mTombstones = 0;
        for (Slot& slot : oldSlots)
        {
            if (slot.State != SLOT_USED)
            {
                continue;
            }
            u32 mask = capacity - 1;
            u32 index = (u32)(HashValue(slot.Pair.Key) & mask);
            while (mSlots[index].State == SLOT_USED)
            {
                index = (index + 1) & mask;
            }
            Slot& target = mSlots[index];
            ::new (static_cast<void*>(&target.Pair)) KVPair(std::move(slot.Pair));
            target.State = SLOT_USED;
            slot.Pair.~KVPair();
        }
    }

    Array<Slot> mSlots;
    u32 mCount = 0;
    u32 mTombstones = 0;
};

}
