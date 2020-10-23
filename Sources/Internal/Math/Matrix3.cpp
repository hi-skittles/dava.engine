#include "Math/Matrix3.h"

namespace DAVA
{
template <>
bool AnyCompare<Matrix3>::IsEqual(const DAVA::Any& v1, const DAVA::Any& v2)
{
    return v1.Get<Matrix3>() == v2.Get<Matrix3>();
}
}
