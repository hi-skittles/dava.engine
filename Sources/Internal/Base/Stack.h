#pragma once

#include "Base/Deque.h"
#include <stack>

namespace DAVA
{
template <class T, class Container = Deque<T>>
using Stack = std::stack<T, Container>;
}
