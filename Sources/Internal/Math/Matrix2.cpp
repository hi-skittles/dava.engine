#include "Math/Matrix2.h"

namespace DAVA
{
template <>
bool AnyCompare<Matrix2>::IsEqual(const DAVA::Any& v1, const DAVA::Any& v2)
{
    return v1.Get<Matrix2>() == v2.Get<Matrix2>();
}
}
