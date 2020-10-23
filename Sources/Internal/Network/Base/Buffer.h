#ifndef __DAVAENGINE_BUFFER_H__
#define __DAVAENGINE_BUFFER_H__

#include "Base/BaseTypes.h"
#include "Base/Platform.h"
#include <libuv/uv.h>

namespace DAVA
{
namespace Net
{
using Buffer = uv_buf_t;

template <typename T>
Buffer CreateBuffer(T* buffer, std::size_t count = 1)
{
#if !defined(DAVA_NETWORK_DISABLE)
    return uv_buf_init(static_cast<char8*>(static_cast<void*>(buffer)), static_cast<uint32>(sizeof(T) * count));
#else
    return Buffer();
#endif
}

inline Buffer CreateBuffer(void* rawBuffer, std::size_t size)
{
#if !defined(DAVA_NETWORK_DISABLE)
    return uv_buf_init(static_cast<char8*>(rawBuffer), static_cast<uint32>(size));
#else
    return Buffer();
#endif
}

/*
 Overloads that take pointer to const buffer
*/
template <typename T>
Buffer CreateBuffer(const T* buffer, std::size_t count = 1)
{
#if !defined(DAVA_NETWORK_DISABLE)
    return uv_buf_init(static_cast<char8*>(static_cast<void*>(const_cast<T*>(buffer))), static_cast<uint32>(sizeof(T) * count));
#else
    return Buffer();
#endif
}

inline Buffer CreateBuffer(const void* rawBuffer, std::size_t size)
{
#if !defined(DAVA_NETWORK_DISABLE)
    return uv_buf_init(static_cast<char8*>(const_cast<void*>(rawBuffer)), static_cast<uint32>(size));
#else
    return Buffer();
#endif
}

} // namespace Net
} // namespace DAVA

#endif // __DAVAENGINE_BUFFER_H__
