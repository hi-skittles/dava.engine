#pragma once

#include "Base/STLAllocator.h"
#include <sstream>

namespace DAVA
{
template <typename CharT>
using BasicStringStream = std::basic_stringstream<CharT, std::char_traits<CharT>, DefaultSTLAllocator<CharT>>;

using StringStream = BasicStringStream<char>;
}
