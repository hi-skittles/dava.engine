#include "Base/Platform.h"
#if !defined(USE_CPP11_CONCURRENCY) && defined(__DAVAENGINE_WINDOWS__)

#include "Concurrency/AtomicWindows.h"

namespace DAVA
{
namespace Detail
{
void DVMemBarrier()
{
    ::MemoryBarrier();
}
} // namespace Detail
} // namespace DAVA

#endif //  !USE_CPP11_CONCURRENCY && __DAVAENGINE_WINDOWS__
