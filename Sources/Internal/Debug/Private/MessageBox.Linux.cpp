#include "Base/Platform.h"
#include "Base/BaseTypes.h"

namespace DAVA
{
namespace Debug
{
int MessageBox(const String& /*title*/, const String& /*message*/, const Vector<String>& /*buttons*/, int /*defaultButton*/)
{
    // TODO: linux
    return -1;
}

} // namespace Debug
} // namespace DAVA
