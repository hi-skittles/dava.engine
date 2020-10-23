#if defined(__DAVAENGINE_LINUX__)

#include "Base/String.h"
#include "Debug/DVAssert.h"

namespace DAVA
{
String GenerateGUID()
{
    DVASSERT(0, "Implement GenerateGUID");
    return String();
}

void OpenURL(const String& /*url*/)
{
    DVASSERT(0, "Implement OpenURL");
}

} //  namespace DAVA

#endif // __DAVAENGINE_LINUX__
