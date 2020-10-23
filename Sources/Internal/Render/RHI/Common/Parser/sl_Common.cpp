
#include "sl_Common.h"

#include <stdio.h> // vsnprintf
#include <string.h> // strcmp, strcasecmp
#include <stdlib.h> // strtod, strtol
#include <cstdarg> // va_start, va_end

namespace sl
{
DAVA::int32 String_PrintfArgList(char* buffer, int size, const char* format, va_list args)
{
    va_list tmp;
    va_copy(tmp, args);

#if _MSC_VER >= 1400
    int n = vsnprintf_s(buffer, size, _TRUNCATE, format, tmp);
#else
    int n = vsnprintf(buffer, size, format, tmp);
#endif

    va_end(tmp);

    if (n < 0 || n > size)
        return -1;
    return n;
}

DAVA::int32 String_FormatFloat(char* buffer, int size, float value)
{
    return Snprintf(buffer, size, "%f", value);
}

bool String_Equal(const char* a, const char* b)
{
    if (a == b)
        return true;
    if (a == NULL || b == NULL)
        return false;
    return strcmp(a, b) == 0;
}

bool String_EqualNoCase(const char* a, const char* b)
{
    if (a == b)
        return true;
    if (a == NULL || b == NULL)
        return false;
#if _MSC_VER
    return _stricmp(a, b) == 0;
#else
    return strcasecmp(a, b) == 0;
#endif
}

double String_ToDouble(const char* str, char** endptr)
{
    return strtod(str, endptr);
}

int String_ToInteger(const char* str, char** endptr)
{
    return strtol(str, endptr, 10);
}

int String_ToIntegerHex(const char* str, char** endptr)
{
    return strtol(str, endptr, 16);
}

StringPool::StringPool(Allocator* allocator)
    : stringArray(allocator)
{
}

StringPool::~StringPool()
{
    for (int i = 0; i < stringArray.GetSize(); i++)
    {
        ::free(stringArray[i]);
    }
}

const char* StringPool::AddString(const char* string)
{
    for (int i = 0; i < stringArray.GetSize(); i++)
    {
        if (String_Equal(stringArray[i], string))
            return stringArray[i];
    }
#if _MSC_VER
    char* dup = _strdup(string);
#else
    char* dup = strdup(string);
#endif
    stringArray.PushBack(dup);
    return dup;
}

bool StringPool::GetContainsString(const char* string) const
{
    for (int i = 0; i < stringArray.GetSize(); i++)
    {
        if (String_Equal(stringArray[i], string))
            return true;
    }
    return false;
}

} // namespace sl
