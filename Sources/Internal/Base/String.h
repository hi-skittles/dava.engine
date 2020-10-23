#pragma once

#include "Base/STLAllocator.h"
#include <string>

namespace DAVA
{
template <typename CharT>
using BasicString = std::basic_string<CharT, std::char_traits<CharT>, DefaultSTLAllocator<CharT>>;

using String = BasicString<char>;
using WideString = BasicString<wchar_t>;
}
