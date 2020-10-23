#include "Base/BaseTypes.h"
#include "Base/Platform.h"
#include <libuv/uv.h>

#include "Debug/DVAssert.h"

namespace DAVA
{
namespace Net
{
const char8* ErrorToString(int32 error)
{
#if !defined(DAVA_NETWORK_DISABLE)
    return uv_strerror(error);
#else
    return "DAVA network is disabled";
#endif
}

} // namespace Net
} // namespace DAVA
