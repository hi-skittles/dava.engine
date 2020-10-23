#pragma once

#include "DAVAConfig.h"

#if defined(__DAVAENGINE_WINDOWS__)
#define DAVA_NOINLINE __declspec(noinline)
#define DAVA_FORCEINLINE __forceinline
#define DAVA_ALIGNOF(x) __alignof(x)
#if _MSC_VER >= 1900 //msvc 2015 RC or later
//Constexpr is not supported even in VS2013 (partially supported in 2015 CTP)
#define DAVA_CONSTEXPR constexpr
#define DAVA_NOEXCEPT noexcept
#else
#define DAVA_CONSTEXPR
#define DAVA_NOEXCEPT throw()
#endif
#else
#define DAVA_NOINLINE __attribute__((noinline))
#define DAVA_FORCEINLINE inline __attribute__((always_inline))
#define DAVA_ALIGNOF(x) alignof(x)
#define DAVA_CONSTEXPR constexpr
#define DAVA_ALIGNED(Var, Len) Var __attribute__((aligned(Len)))
#define DAVA_NOEXCEPT noexcept
#ifndef DAVA_DEPRECATED
#define DAVA_DEPRECATED(func) func __attribute__((deprecated))
#endif
#endif

#if defined(__clang__)
#define DAVA_SWITCH_CASE_FALLTHROUGH [[clang::fallthrough]]
#else
#define DAVA_SWITCH_CASE_FALLTHROUGH
#endif

#if defined(DAVA_MEMORY_PROFILING_ENABLE)
#include "MemoryManager/AllocPools.h"
#include "MemoryManager/TrackingAllocator.h"
#endif

#include "Base/String.h"
#include "Base/StringStream.h"
#include "Base/Array.h"
#include "Base/List.h"
#include "Base/Vector.h"
#include "Base/Deque.h"
#include "Base/Set.h"
#include "Base/Map.h"
#include "Base/Stack.h"
#include "Base/PriorityQueue.h"
#include "Base/UnordererSet.h"
#include "Base/UnordererMap.h"
#include "Base/Bitset.h"

#include <cstdint>
#include <cstring>

namespace DAVA
{
using ComponentMask = Bitset<128>;

//Platform-independent signed and unsigned integer type
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;

using pointer_size = uintptr_t;
using size_type = size_t;

using char8 = char;
using char16 = wchar_t;

using float32 = float;
using float64 = double;

//Compile-time checks for size of types
static_assert(sizeof(int8) == 1, "Invalid type size!");
static_assert(sizeof(uint8) == 1, "Invalid type size!");
static_assert(sizeof(int16) == 2, "Invalid type size!");
static_assert(sizeof(uint16) == 2, "Invalid type size!");
static_assert(sizeof(int32) == 4, "Invalid type size!");
static_assert(sizeof(uint32) == 4, "Invalid type size!");
static_assert(sizeof(int64) == 8, "Invalid type size!");
static_assert(sizeof(uint64) == 8, "Invalid type size!");
static_assert(sizeof(pointer_size) == sizeof(void*), "Invalid type size!");
static_assert(sizeof(char8) == 1, "Invalid type size!");
static_assert(sizeof(float32) == 4, "Invalid type size!");
static_assert(sizeof(float64) == 8, "Invalid type size!");

const uint32 InvalidIndex = static_cast<uint32>(-1);

template <class T>
inline T Min(T a, T b)
{
    return (a < b) ? (a) : (b);
}

template <class T>
inline T Max(T a, T b)
{
    return (a > b) ? (a) : (b);
}

template <class T>
inline T Abs(T a)
{
    return (a >= 0) ? (a) : (-a);
}

template <class T>
inline T Clamp(T val, T a, T b)
{
    return Min(b, Max(val, a));
}

template <class T>
inline T Saturate(T val)
{
    return Clamp(val, static_cast<T>(0), static_cast<T>(1));
}

#define Memcmp std::memcmp
#define Memcpy std::memcpy
#define Memset std::memset
#define Memmove std::memmove

#if defined(__DAVAENGINE_WINDOWS__)
#define Snprintf _snprintf
#else
#define Snprintf snprintf
#endif


#if defined(__DAVAENGINE_WINDOWS__)
#define strnicmp _strnicmp 
#elif defined(__DAVAENGINE_POSIX__)
#define strnicmp strncasecmp 
#else
#endif

template <class TYPE>
void SafeDelete(TYPE*& d)
{
    if (d != nullptr)
    {
        delete d;
        d = nullptr;
    }
}

template <class TYPE>
void SafeDeleteArray(TYPE*& d)
{
    if (d != nullptr)
    {
        delete[] d;
        d = nullptr;
    }
}

#ifndef SAFE_DELETE // for compatibility with FCollada
#define SAFE_DELETE(x) DAVA::SafeDelete(x)
#endif 

#ifndef SAFE_DELETE_ARRAY // for compatibility with FCollada
#define SAFE_DELETE_ARRAY(x) DAVA::SafeDeleteArray(x)
#endif

#ifndef OBJC_SAFE_RELEASE
#define OBJC_SAFE_RELEASE(x) [x release];x = nil;
#endif

/**
 \enum Graphical object alignment.
*/
enum eAlign
{
    ALIGN_LEFT = 0x01, //!<Align graphical object by the left side.
    ALIGN_HCENTER = 0x02, //!<Align graphical object by the horizontal center.
    ALIGN_RIGHT = 0x04, //!<Align graphical object by the right side.
    ALIGN_TOP = 0x08, //!<Align graphical object by the top side.
    ALIGN_VCENTER = 0x10, //!<Align graphical object by the vertical center.
    ALIGN_BOTTOM = 0x20, //!<Align graphical object by the bottom side.
    ALIGN_HJUSTIFY = 0x40 //!<Used only for the fonts. Stretch font string over all horizontal size of the area.
};

enum class eErrorCode
{
    SUCCESS,
    ERROR_FILE_FORMAT_INCORRECT,
    ERROR_FILE_NOTFOUND,
    ERROR_READ_FAIL,
    ERROR_WRITE_FAIL,
    ERROR_DECODE_FAIL
};

} // namespace DAVA

// clang-format off
/**
    \ingroup engine
    Define bitwise operators for strongly typed enums which can be used as bit flags.

    \code
    enum class E : int
    {
        FLAG1 = 0x01,
        FLAG2 = 0x02
    };
    DAVA_DEFINE_ENUM_BITWISE_OPERATORS(E)
    // Now you can use enum E without casting to int
    E e1 = E::FLAG1 | E::FLAG2;
    E e2 = e1 & ~E::FLAG1;
    e1 ^= e2;
    \endcode
 */
#define DAVA_DEFINE_ENUM_BITWISE_OPERATORS(enumType) \
    inline /*constexpr*/ enumType operator|(enumType l, enumType r) { return static_cast<enumType>(static_cast<uint32>(l) | static_cast<uint32>(r)); } \
    inline /*constexpr*/ enumType operator&(enumType l, enumType r) { return static_cast<enumType>(static_cast<uint32>(l) & static_cast<uint32>(r)); } \
    inline /*constexpr*/ enumType operator^(enumType l, enumType r) { return static_cast<enumType>(static_cast<uint32>(l) ^ static_cast<uint32>(r)); } \
    inline /*constexpr*/ enumType& operator|=(enumType& l, enumType r) { l = l | r; return l; } \
    inline /*constexpr*/ enumType& operator&=(enumType& l, enumType r) { l = l & r; return l; } \
    inline /*constexpr*/ enumType& operator^=(enumType& l, enumType r) { l = l ^ r; return l; } \
    inline /*constexpr*/ enumType operator~(enumType e) { return static_cast<enumType>(~static_cast<uint32>(e)); }
// clang-format on

/**
    \ingroup engine
    Forward declare Objective-C class `classname` in a manner that it can be used as either Objective-C or C++.

    This is primarily intended for use in header files that may be included by both Objective-C and C++ source files.
    Inspired by Qt Q_FORWARD_DECLARE_OBJC_CLASS.
 */
#if defined(__OBJC__)
#define DAVA_FORWARD_DECLARE_OBJC_CLASS(classname) @class classname
#else
#define DAVA_FORWARD_DECLARE_OBJC_CLASS(classname) typedef struct objc_object classname
#endif

#define DAVA_MAKEFOURCC(ch0, ch1, ch2, ch3) (static_cast<uint32>(static_cast<uint8>(ch0)) | (static_cast<uint32>(static_cast<uint8>(ch1)) << 8) | (static_cast<uint32>(static_cast<uint8>(ch2)) << 16) | (static_cast<uint32>(static_cast<uint8>(ch3)) << 24))