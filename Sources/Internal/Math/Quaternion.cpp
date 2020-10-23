#include "Math/Quaternion.h"

namespace DAVA
{
const Quaternion Quaternion::Identity(0.f, 0.f, 0.f, 1.f);

template <>
bool AnyCompare<Quaternion>::IsEqual(const DAVA::Any& v1, const DAVA::Any& v2)
{
    return v1.Get<Quaternion>() == v2.Get<Quaternion>();
}
}
