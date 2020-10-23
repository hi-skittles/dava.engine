#pragma once

#include "Base/Vector.h"
#include <queue>

namespace DAVA
{
template <class T, class Container = Vector<T>, class Compare = std::less<typename Container::value_type>>
using PriorityQueue = std::priority_queue<T, Container, Compare>;
}
