#pragma once

#include "Base/STLAllocator.h"
#include <map>

namespace DAVA
{
template <class _Kty, class _Ty, class _Pr = std::less<_Kty>>
using Map = std::map<_Kty, _Ty, _Pr, DefaultSTLAllocator<std::pair<const _Kty, _Ty>>>;

template <class _Kty, class _Ty, class _Pr = std::less<_Kty>>
using MultiMap = std::multimap<_Kty, _Ty, _Pr, DefaultSTLAllocator<std::pair<const _Kty, _Ty>>>;
}
