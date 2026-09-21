#pragma once

#include "Core.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <utility>

namespace Aether {

//
// Reference-counted-free, heap-backed dynamic string. Always null-terminated.
//
class String
{
public:
    String() = default;

    String(const char* text)
        : String(text, (u32)std::strlen(text))
    {
    }

    String(const char* text, u32 length)
    {
        Reserve(length);
        std::memcpy(mData, text, length);
        mLength = length;
        mData[mLength] = '\0';
    }

    String(const String& other)
        : String(other.mData ? other.mData : "", other.mLength)
    {
    }

    String(String&& other) noexcept
        : mData(other.mData)
        , mLength(other.mLength)
        , mCapacity(other.mCapacity)
    {
        other.mData = nullptr;
        other.mLength = 0;
        other.mCapacity = 0;
    }

    String& operator=(const char* text)
    {
        u32 length = (u32)std::strlen(text);
        Reserve(length);
        std::memcpy(mData, text, length + 1);
        mLength = length;
        return *this;
    }

    String& operator=(const String& other)
    {
        if (this != &other)
        {
            Assign(other.CStr(), other.mLength);
        }
        return *this;
    }

    String& operator=(String&& other) noexcept
    {
        if (this != &other)
        {
            delete[] mData;
            mData = other.mData;
            mLength = other.mLength;
            mCapacity = other.mCapacity;
            other.mData = nullptr;
            other.mLength = 0;
            other.mCapacity = 0;
        }
        return *this;
    }

    ~String()
    {
        delete[] mData;
    }

    static String Format(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        String result = FormatV(format, args);
        va_end(args);
        return result;
    }

    static String FormatV(const char* format, va_list args)
    {
        va_list argsCopy;
        va_copy(argsCopy, args);
        i32 needed = vsnprintf(nullptr, 0, format, argsCopy);
        va_end(argsCopy);
        if (needed <= 0)
        {
            return String();
        }
        String result;
        result.Reserve((u32)needed);
        vsnprintf(result.mData, (size_t)needed + 1, format, args);
        result.mLength = (u32)needed;
        return result;
    }

    static f32 ToF32(const char* text, f32 fallback = 0.0f)
    {
        if (!text)
        {
            return fallback;
        }
        char* end = nullptr;
        f32 value = strtof(text, &end);
        return end == text ? fallback : value;
    }

    static i32 ToI32(const char* text, i32 fallback = 0)
    {
        if (!text)
        {
            return fallback;
        }
        char* end = nullptr;
        long value = strtol(text, &end, 10);
        return end == text ? fallback : (i32)value;
    }

    u32 Length() const { return mLength; }
    bool IsEmpty() const { return mLength == 0; }
    u32 Capacity() const { return mCapacity; }

    const char* CStr() const { return mData ? mData : ""; }

    char operator[](u32 index) const
    {
        AETHER_ASSERT(index < mLength);
        return mData[index];
    }

    void Reserve(u32 capacity)
    {
        // Capacity always covers one extra byte for the terminator.
        ++capacity;
        if (capacity <= mCapacity)
        {
            return;
        }
        u32 newCapacity = mCapacity ? mCapacity : 16;
        while (newCapacity < capacity)
        {
            newCapacity *= 2;
        }
        char* newData = new char[newCapacity];
        if (mLength > 0)
        {
            std::memcpy(newData, mData, mLength + 1);
        }
        else
        {
            newData[0] = '\0';
        }
        delete[] mData;
        mData = newData;
        mCapacity = newCapacity;
    }

    void Clear()
    {
        mLength = 0;
        if (mData)
        {
            mData[0] = '\0';
        }
    }

    void Append(const char* text, u32 length)
    {
        Reserve(mLength + length);
        std::memcpy(mData + mLength, text, length);
        mLength += length;
        mData[mLength] = '\0';
    }

    String& operator+=(const String& other)
    {
        Append(other.CStr(), other.mLength);
        return *this;
    }

    String& operator+=(const char* text)
    {
        Append(text, (u32)std::strlen(text));
        return *this;
    }

    String& operator+=(char c)
    {
        Append(&c, 1);
        return *this;
    }

    friend String operator+(const String& a, const String& b)
    {
        String result;
        result.Reserve(a.mLength + b.mLength);
        result += a;
        result += b;
        return result;
    }

    friend String operator+(const String& a, const char* b)
    {
        String result;
        result.Reserve(a.mLength + (u32)std::strlen(b));
        result += a;
        result += b;
        return result;
    }

    friend String operator+(const char* a, const String& b)
    {
        String result;
        result.Reserve((u32)std::strlen(a) + b.mLength);
        result += a;
        result += b;
        return result;
    }

    bool operator==(const String& other) const
    {
        return mLength == other.mLength && std::memcmp(CStr(), other.CStr(), mLength) == 0;
    }

    bool operator==(const char* text) const
    {
        return std::strcmp(CStr(), text) == 0;
    }

    friend bool operator==(const char* a, const String& b) { return b == a; }

    bool operator!=(const String& other) const { return !(*this == other); }
    bool operator!=(const char* text) const { return !(*this == text); }
    friend bool operator!=(const char* a, const String& b) { return !(b == a); }

    bool operator<(const String& other) const
    {
        return std::strcmp(CStr(), other.CStr()) < 0;
    }

    bool operator>(const String& other) const
    {
        return std::strcmp(CStr(), other.CStr()) > 0;
    }

    i32 Find(char c, u32 from = 0) const
    {
        for (u32 i = from; i < mLength; ++i)
        {
            if (mData[i] == c)
            {
                return (i32)i;
            }
        }
        return -1;
    }

    i32 Find(const String& sub, u32 from = 0) const
    {
        if (sub.mLength == 0 || sub.mLength > mLength)
        {
            return -1;
        }
        for (u32 i = from; i + sub.mLength <= mLength; ++i)
        {
            if (std::memcmp(mData + i, sub.mData, sub.mLength) == 0)
            {
                return (i32)i;
            }
        }
        return -1;
    }

    bool Contains(const String& sub) const { return Find(sub) >= 0; }

    String Sub(u32 start, u32 count) const
    {
        if (start >= mLength)
        {
            return String();
        }
        if (start + count > mLength)
        {
            count = mLength - start;
        }
        return String(mData + start, count);
    }

    bool StartsWith(const String& prefix) const
    {
        if (prefix.mLength > mLength)
        {
            return false;
        }
        return std::memcmp(mData, prefix.mData, prefix.mLength) == 0;
    }

    bool EndsWith(const String& suffix) const
    {
        if (suffix.mLength > mLength)
        {
            return false;
        }
        return std::memcmp(mData + mLength - suffix.mLength, suffix.mData, suffix.mLength) == 0;
    }

    const char* begin() const { return CStr(); }
    const char* end() const { return CStr() + mLength; }

private:
    void Assign(const char* text, u32 length)
    {
        Reserve(length);
        std::memcpy(mData, text, length);
        mLength = length;
        mData[mLength] = '\0';
    }

    char* mData = nullptr;
    u32 mLength = 0;
    u32 mCapacity = 0;
};

inline u64 HashValue(const String& s)
{
    u64 hash = 1469598103934665603ull;
    for (const char* p = s.CStr(); *p; ++p)
    {
        hash ^= (u64)(u8)*p;
        hash *= 1099511628211ull;
    }
    return hash;
}

}
