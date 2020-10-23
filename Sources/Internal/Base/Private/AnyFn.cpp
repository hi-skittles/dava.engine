#include "Base/AnyFn.h"

namespace DAVA
{
bool AnyFn::Params::operator==(const AnyFn::Params& p) const
{
    if (retType != p.retType)
    {
        return false;
    }

    size_t sz = argsType.size();

    if (sz != p.argsType.size())
    {
        return false;
    }

    return (0 == std::memcmp(argsType.data(), p.argsType.data(), sizeof(void*) * sz));
}

} // namespace DAVA
