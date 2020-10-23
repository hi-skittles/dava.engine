#pragma once

#include "Base/RefPtr.h"
#include "Base/TemplateHelpers.h"

namespace DAVA
{
template <class T, typename... Args>
RefPtr<T> MakeRef(Args&&... args)
{
    return RefPtr<T>(new T(std::forward<Args>(args)...));
}

template <class T1, class T2>
RefPtr<T1> DynamicTypeCheckRef(const RefPtr<T2>& rp)
{
    RefPtr<T1> p;
    p = DynamicTypeCheck<T1*>(rp.Get());
    return p;
}
}