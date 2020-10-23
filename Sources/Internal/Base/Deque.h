#pragma once

#include "Base/STLAllocator.h"
#include <queue>

namespace DAVA
{
template <typename T>
using Deque = std::deque<T, DefaultSTLAllocator<T>>;
}
