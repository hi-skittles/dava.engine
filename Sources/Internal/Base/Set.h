#pragma once

#include "Base/STLAllocator.h"
#include <set>

namespace DAVA
{
template <class _Key, class _Compare = std::less<_Key>>
using Set = std::set<_Key, _Compare, DefaultSTLAllocator<_Key>>;
}
