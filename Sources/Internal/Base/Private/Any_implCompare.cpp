#include "Any_implCompare.h"

#include "Base/String.h"

namespace DAVA
{
template <>
bool AnyCompare<String>::IsEqual(const Any& v1, const Any& v2)
{
    return v1.Get<String>() == v2.Get<String>();
}

template <>
bool AnyCompare<WideString>::IsEqual(const Any& v1, const Any& v2)
{
    return v1.Get<WideString>() == v2.Get<WideString>();
}

} // namespace DAVA
