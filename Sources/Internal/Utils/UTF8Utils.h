#pragma once

#include "Base/BaseTypes.h"

namespace DAVA
{
/**
     \ingroup utils
     \brief Class to work with UTF8 strings
 */
namespace UTF8Utils
{
/**
    List of errors for SafeEncodeToWideString function
*/
enum eSafeEncodeError
{
    NONE = 0, //!< no errors
    NON_UTF8_SYMBOLS_REPLACED, //!< original string contains non-utf8 symbols which was replaced by '?'
    STRING_NOT_ENCODED //!< original string wasn't encoded
};

/**
        \brief convert UTF8 string to WideString
        \param[in] string string in UTF8 format
        \param[in] size size of buffer allocated for this string
        \param[out] resultString result unicode string
     */
void EncodeToWideString(const uint8* string, size_type size, WideString& resultString);

/**
        \brief convert UTF8 string to WideString without throwing utf8::exception
        \param[in] string string in UTF8 format
        \param[in] size size of buffer allocated for this string
        \param[out] resultString result unicode string
        \param[out] encodeError encoding error
*/
void SafeEncodeToWideString(const uint8* string, size_t size, WideString& result, eSafeEncodeError& encodeError);

/**
        \brief convert UTF8 string to WideString
        \param[in] utf8String string in UTF8 format
        \return string in unicode
     */
inline WideString EncodeToWideString(const String& utf8String)
{
    WideString str;
    EncodeToWideString(reinterpret_cast<const uint8*>(utf8String.c_str()), utf8String.length(), str);
    return str;
}

/**
        \brief convert UTF8 string to WideString without throwing utf8::exception
        \param[in] utf8String string in UTF8 format
        \param[out] encodeError encoding error
        \return string in unicode
*/
inline WideString SafeEncodeToWideString(const String& utf8String, eSafeEncodeError& encodeError)
{
    WideString str;
    SafeEncodeToWideString(reinterpret_cast<const uint8*>(utf8String.c_str()), utf8String.length(), str, encodeError);
    return str;
}
/**
     \brief convert WideString string to UTF8
     \param[in] wstring string in WideString format
     \returns string in UTF8 format, contained in DAVA::String
     */
String EncodeToUTF8(const WideString& wstring);
String EncodeToUTF8(const char32_t* str);

template <typename CHARTYPE>
String MakeUTF8String(const CHARTYPE* value);

template <>
inline String MakeUTF8String<char8>(const char8* value)
{
    return String(value);
}

template <>
inline String MakeUTF8String<char16>(const char16* value)
{
    return EncodeToUTF8(WideString(value));
}

template <>
inline String MakeUTF8String<char32_t>(const char32_t* value)
{
    return EncodeToUTF8(value);
}

/**
Trim whitespace at begin and end of specified UTF8 string.
Behavior depends by current system locale: http://en.cppreference.com/w/cpp/string/wide/iswspace
*/
String Trim(const String& str);

/**
Trim whitespace at begin of specified UTF8 string.
Behavior depends by current system locale: http://en.cppreference.com/w/cpp/string/wide/iswspace
*/
String TrimLeft(const String& str);

/**
Trim whitespace at end of specified UTF8 string.
Behavior depends by current system locale: http://en.cppreference.com/w/cpp/string/wide/iswspace
*/
String TrimRight(const String& str);

} // namespace UTF8Utils

//////////////////////////////////////////////////////////////////////////

} // namespace DAVA
