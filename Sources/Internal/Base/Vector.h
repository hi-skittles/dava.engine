#pragma once

#include "Base/STLAllocator.h"
#include <vector>

namespace DAVA
{
template <typename T>
using Vector = std::vector<T, DefaultSTLAllocator<T>>;
}
