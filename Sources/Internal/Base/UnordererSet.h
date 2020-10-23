#pragma once

#include "Base/STLAllocator.h"
#include <unordered_set>

namespace DAVA
{
template <typename Key, typename Hash = std::hash<Key>, typename KeyEqual = std::equal_to<Key>>
using UnorderedSet = std::unordered_set<Key, Hash, KeyEqual, DefaultSTLAllocator<Key>>;
}
