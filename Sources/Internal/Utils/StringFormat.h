#ifndef __DAVAENGINE_STRINGFORMAT_H__
#define __DAVAENGINE_STRINGFORMAT_H__

#include "Base/BaseTypes.h"

#include <cstdarg>

namespace DAVA
{
//! String formating functions
//! Functions for use together with Global::Log

//! Formatting function (printf-like syntax)
//! Function support recursive calls as :
//! Format("%s", Format("%d: %d: %d", 10, 20, 33).c_str());

String Format(const char8* format, ...);
String FormatVL(const char8* format, va_list& args);

WideString Format(const char16* format, ...);
WideString FormatVL(const char16* format, va_list& args);

//! Function to get indent strings for usage in printf and similar functions
inline String GetIndentString(char8 indentChar, int32 level)
{
    return String(level, indentChar);
}

} // namespace DAVA

#endif // __DAVAENGINE_STRINGFORMAT_H__
