#pragma once

#include "Base/STLAllocator.h"
#include <list>

namespace DAVA
{
template <typename T>
using List = std::list<T, DefaultSTLAllocator<T>>;
}
