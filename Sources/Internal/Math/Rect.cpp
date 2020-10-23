#include "Math/Rect.h"

namespace DAVA
{
template <>
bool AnyCompare<Rect>::IsEqual(const DAVA::Any& v1, const DAVA::Any& v2)
{
    return v1.Get<Rect>() == v2.Get<Rect>();
}
} // namespace DAVA