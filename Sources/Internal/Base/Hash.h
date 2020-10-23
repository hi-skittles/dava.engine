#ifndef __DAVAENGINE_HASH__
#define __DAVAENGINE_HASH__

#include <stddef.h>
#include "Base/BaseTypes.h"
#include "Debug/DVAssert.h"

namespace DAVA
{
// TODO: Think how to make it work for generic pointers and char * at the same time
class Entity;

// default hash function for strings
inline size_t DavaHashString(const char* str)
{
    DVASSERT(str && "Can't be NULL. Check logics.");

    size_t hash = 0;
    for (; *str; ++str)
    {
        hash = 5 * hash + *str;
    }
    return hash;
}

// Base hash type
// Any child (template specialized) structure
// should implement for specific type T:
// - hash function: operator()(T value)
// - compare function: compare(T value1, T value2)
template <typename T>
struct Hash
{
};

// specialization for char *
template <>
struct Hash<char*>
{
    size_t operator()(const char* str) const
    {
        return DavaHashString(str);
    }

    bool Compare(const char* str1, const char* str2) const
    {
        return (str1 == str2) || (0 == strcmp(str1, str2));
    }
};

// specialization for const char *
template <>
struct Hash<const char*>
{
    size_t operator()(const char* str) const
    {
        return DavaHashString(str);
    }

    bool Compare(const char* str1, const char* str2) const
    {
        return (str1 == str2) || (0 == strcmp(str1, str2));
    }
};

// specialization for const DAVA::String &
template <>
struct Hash<DAVA::String>
{
    size_t operator()(const DAVA::String& str) const
    {
        return DavaHashString(str.c_str());
    }

    bool Compare(const DAVA::String& str1, const DAVA::String& str2) const
    {
        return (str1 == str2);
    }
};

// specialization for DAVA::String *
template <>
struct Hash<DAVA::String*>
{
    size_t operator()(const DAVA::String* str) const
    {
        return DavaHashString(str->c_str());
    }

    bool Compare(const DAVA::String* str1, const DAVA::String* str2) const
    {
        return (str1 == str2) || 0 == str1->compare(str2->c_str());
    }
};

// specialization for const DAVA::String *
template <>
struct Hash<const DAVA::String*>
{
    size_t operator()(const DAVA::String* str) const
    {
        return DavaHashString(str->c_str());
    }

    bool Compare(const DAVA::String* str1, const DAVA::String* str2) const
    {
        return (str1 == str2) || 0 == str1->compare(str2->c_str());
    }
};

// specialization for all pointers
template <typename T>
struct Hash<T*>
{
    size_t operator()(T* pointer) const
    {
        return reinterpret_cast<size_t>(pointer);
    }

    bool Compare(T* ptr1, T* ptr2) const
    {
        return (ptr1 == ptr2);
    }
};

template <>
struct Hash<DAVA::int32>
{
    size_t operator()(const DAVA::int32 i) const
    {
        return i;
    }

    bool Compare(const DAVA::int32 i1, const DAVA::int32 i2) const
    {
        return (i1 == i2);
    }
};

template <>
struct Hash<DAVA::uint32>
{
    size_t operator()(const DAVA::uint32 i) const
    {
        return i;
    }

    bool Compare(const DAVA::uint32 i1, const DAVA::uint32 i2) const
    {
        return (i1 == i2);
    }
};

template <class T>
inline void HashCombine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

inline DAVA::uint32 HashValue_N(const char* key, uint32 length) DAVA_NOEXCEPT
{
    using DAVA::uint32;

    auto hash_mix = [](uint32& a, uint32& b, uint32& c)
    {
        a -= b;
        a -= c;
        a ^= (c >> 13);
        b -= c;
        b -= a;
        b ^= (a << 8);
        c -= a;
        c -= b;
        c ^= (b >> 13);
        a -= b;
        a -= c;
        a ^= (c >> 12);
        b -= c;
        b -= a;
        b ^= (a << 16);
        c -= a;
        c -= b;
        c ^= (b >> 5);
        a -= b;
        a -= c;
        a ^= (c >> 3);
        b -= c;
        b -= a;
        b ^= (a << 10);
        c -= a;
        c -= b;
        c ^= (b >> 15);
    };

    // set up the internal state

    uint32 len = length;
    uint32 a = 0x9E3779B9; // the golden ratio, an arbitrary value
    uint32 b = 0x9E3779B9; // the golden ratio, an arbitrary value
    uint32 c = 0; // the previous hash value

    // handle most of the key

    while (len >= 12)
    {
        a += (key[0] + (static_cast<uint32>(key[1]) << 8) + (static_cast<uint32>(key[2]) << 16) + (static_cast<uint32>(key[3]) << 24));
        b += (key[4] + (static_cast<uint32>(key[5]) << 8) + (static_cast<uint32>(key[6]) << 16) + (static_cast<uint32>(key[7]) << 24));
        c += (key[8] + (static_cast<uint32>(key[9]) << 8) + (static_cast<uint32>(key[10]) << 16) + (static_cast<uint32>(key[11]) << 24));

        hash_mix(a, b, c);

        key += 12;
        len -= 12;
    }

    // handle the last 11 bytes

    c += length;

    //lint -e{616} // control flows into case/default
    switch (len)
    {
    // all the case statements fall through

    case 11:
        c += (static_cast<uint32>(key[10]) << 24);
        DAVA_SWITCH_CASE_FALLTHROUGH;
    case 10:
        c += (static_cast<uint32>(key[9]) << 16);
        DAVA_SWITCH_CASE_FALLTHROUGH;
    case 9:
        c += (static_cast<uint32>(key[8]) << 8);
        DAVA_SWITCH_CASE_FALLTHROUGH;
    // the first byte of c is reserved for the length
    case 8:
        b += (static_cast<uint32>(key[7]) << 24);
        DAVA_SWITCH_CASE_FALLTHROUGH;
    case 7:
        b += (static_cast<uint32>(key[6]) << 16);
        DAVA_SWITCH_CASE_FALLTHROUGH;
    case 6:
        b += (static_cast<uint32>(key[5]) << 8);
        DAVA_SWITCH_CASE_FALLTHROUGH;
    case 5:
        b += key[4];
        DAVA_SWITCH_CASE_FALLTHROUGH;
    case 4:
        a += (static_cast<uint32>(key[3]) << 24);
        DAVA_SWITCH_CASE_FALLTHROUGH;
    case 3:
        a += (static_cast<uint32>(key[2]) << 16);
        DAVA_SWITCH_CASE_FALLTHROUGH;
    case 2:
        a += (static_cast<uint32>(key[1]) << 8);
        DAVA_SWITCH_CASE_FALLTHROUGH;
    case 1:
        a += key[0];
        DAVA_SWITCH_CASE_FALLTHROUGH;
    case 0:
        //  case 0: nothing left to add
        break;
    }

    hash_mix(a, b, c);

    return c;
}

template <size_t N>
DAVA_CONSTEXPR uint32 StringHash(const char(&str)[N])
{
    return HashValue_N(str, N - 1);
}

inline size_t BufferHash(const DAVA::uint8* buffer, DAVA::uint32 bufferLength) DAVA_NOEXCEPT
{
//this function was found in gcc49 functional_hash.h

#if defined(__x86_64__) || defined(_WIN64) || defined(__LP64__)
    static_assert(sizeof(size_t) == 8, "wrong size_t size");

    const size_t basis = 14695981039346656037ULL;
    const size_t prime = 1099511628211ULL;
#else
    static_assert(sizeof(size_t) == 4, "wrong size_t size");

    const size_t basis = 2166136261UL;
    const size_t prime = 16777619UL;
#endif

    size_t result = basis;
    for (; bufferLength; --bufferLength)
    {
        result ^= static_cast<size_t>(*buffer++);
        result *= prime;
    }
    return result;
}
};

#define DV_HASH(str) DAVA::StringHash(str)

#endif // __DAVAENGINE_HASH__
