#pragma once

#include "Base/STLAllocator.h"
#include <unordered_map>

namespace DAVA
{
template <typename Key,
          typename T,
          typename Hash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>>
using UnorderedMap = std::unordered_map<Key, T, Hash, KeyEqual, DefaultSTLAllocator<std::pair<const Key, T>>>;

template <typename Key,
          typename T,
          typename Hash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>>
using UnorderedMultiMap = std::unordered_multimap<Key, T, Hash, KeyEqual, DefaultSTLAllocator<std::pair<const Key, T>>>;
}
