#pragma once

#include "Base/BaseTypes.h"
#include <cctype>
#include <locale>

namespace DAVA
{
/**
 * \namespace StringUtils
 *
 * \brief Namespace with string helper functions.
 */
namespace StringUtils
{
/**
* \enum eLineBreakType
*
* \brief Values that represent line break types.
*/
enum eLineBreakType
{
    LB_MUSTBREAK = 0, /**< Break is mandatory */
    LB_ALLOWBREAK, /**< Break is allowed */
    LB_NOBREAK, /**< No break is possible */
    LB_INSIDECHAR /**< A UTF-8/16 sequence is unfinished */
};

/**
 * \brief Gets information about UTF8 line breaks using libunibreak library.
 * \param string The input string.
 * \param [out] breaks The output vector of breaks.
 * \param locale (Optional) The locale code.
 */
void GetLineBreaks(const String& string, Vector<uint8>& breaks, const char8* locale = 0);

/**
* \brief Gets information about line breaks using libunibreak library.
* \param string The input string.
* \param [out] breaks The output vector of breaks.
* \param locale (Optional) The locale code.
*/
void GetLineBreaks(const WideString& string, Vector<uint8>& breaks, const char8* locale = 0);

bool IsWhitespace(char8 t);
bool IsWhitespace(char16 t);

/**
* \brief Trims the given string.
* \param [in] string The string.
* \return output string.
*/
template <typename StringType>
StringType Trim(const StringType& string)
{
    auto it = string.begin();
    auto end = string.end();
    auto rit = string.rbegin();
    while (it != end && IsWhitespace(*it)) ++it;
    while (rit.base() != it && IsWhitespace(*rit)) ++rit;
    return StringType(it, rit.base());
}

/**
* \brief Trim left.
* \param [in] string The string.
* \return output string.
*/
template <typename StringType>
StringType TrimLeft(const StringType& string)
{
    auto it = string.begin();
    auto end = string.end();
    while (it != end && IsWhitespace(*it)) ++it;
    return StringType(it, end);
}

/**
* \brief Trim right.
* \param [in] string The string.
* \return output string.
*/
template <typename StringType>
StringType TrimRight(const StringType& string)
{
    auto rit = string.rbegin();
    auto rend = string.rend();
    while (rit != rend && IsWhitespace(*rit)) ++rit;
    return StringType(rend.base(), rit.base());
}

/**
* \brief Remove from line non-printable characters and replace
*        unicode spaces into ASCII space.
* \param [in] string The string.
* \param [in] tabRule The kind of process \t symbol: -1 - keep tab symbol, 0..n - replace tab with 0..n spaces.
* \return output string.
*/
WideString RemoveNonPrintable(const WideString& string, const int8 tabRule = -1);

/**
 * \brief Replaces all occurrences of a search string in the specified string with replacement string
 * \param string Original string
 * \param search Seeking value
 * \param replacement Replacement value
 */
void ReplaceAll(WideString& string, const WideString& search, const WideString& replacement);

/**
 * \brief Substitute all param occurrences like '%(param_name)' in source string with values from replacements map.
 * \param string Original string with params %(param_name1), %(param_name2), etc.
 * \param replacements Map with substitutions like 'param_name' = 'ParamValue'
 */
String SubstituteParams(const String& string, const DAVA::UnorderedMap<DAVA::String, DAVA::String>& replacements);

/**
* \brief Query if 't' is kind of printable character.
* \param t The char16 to process.
* \return false if not printable.
*/
inline bool IsPrintable(char16 t)
{
    switch (t)
    {
    case L'\n': // Line feed
    case L'\r': // Carriage return
    case 0x200B: // Zero-width space
    case 0x200C: // Zero-width non-joiner
    case 0x200D: // Zero-width joiner
    case 0x200E: // Left-to-right zero-width character
    case 0x200F: // Right-to-left zero-width non-Arabic character
    case 0x061C: // Right-to-left zero-width Arabic character
        return false;
    default:
        return true;
    }
}

/**
 * \brief Query if 't' is all kind of spaces or linebreak. Using this function for trim whitespace.
 * \param t The char16 to process.
 * \return true if space, false if not.
 */
inline bool IsWhitespace(char16 t)
{
    switch (t)
    {
    case 0x0009: // Tabulation
    case 0x000A: // Line feed
    case 0x000B: // Line tab
    case 0x000C: // Form feed
    case 0x000D: // Carriage return
    // Unicode characters in 'Separator, Space' category (Zs)
    case 0x0020: // Space
    case 0x00A0: // No-break space
    case 0x1680: // Ogham space mark
    case 0x2000: // En quad
    case 0x2001: // Em quad
    case 0x2002: // En space
    case 0x2003: // Em space
    case 0x2004: // Three-per-em space
    case 0x2005: // Four-per-em space
    case 0x2006: // Six-per-em space
    case 0x2007: // Figure space
    case 0x2008: // Punctuation space
    case 0x2009: // Thin space
    case 0x200A: // Hair space
    case 0x202F: // Narrow No-break space
    case 0x205F: // Medium mathematical space
    case 0x3000: // Ideographic space
    // Unicode characters in 'Separator, Line' category (Zl)
    case 0x2028: // Line separator
    // Unicode characters in 'Separator, Paragraph' category (Zp)
    case 0x2029: // Paragraph separator
    // Additional characters are treated as spaces
    case 0x200B: // Zero-width space
        return true;
    default:
        return false;
    }
}

inline bool IsWhitespace(char8 t)
{
    return (std::isspace(static_cast<unsigned char>(t)) != 0);
}

/**
check whether given `string` has at least one whitespace symbol in it
*/
inline bool HasWhitespace(const String& string)
{
    for (char8 c : string)
    {
        if (IsWhitespace(c))
            return true;
    }
    return false;
}

inline bool StartsWith(const String& str, const String& substr)
{
    if (str.length() < substr.length())
    {
        return false;
    }
    return (str.compare(0, substr.size(), substr) == 0);
}

inline bool EndsWith(const String& str, const String& end)
{
    size_t lineSize = str.size();
    size_t endSize = end.size();
    return lineSize >= endSize &&
    0 == str.compare(lineSize - endSize, endSize, end);
}

template <typename StringType>
bool ContainsIgnoreCase(const StringType& string, const StringType& toFind,
                        const std::locale& locale = std::locale())
{
    using CharType = typename StringType::value_type;
    auto findIt = std::search(std::begin(string), std::end(string),
                              std::begin(toFind), std::end(toFind),
                              [&locale](CharType char1, CharType char2)
                              {
                                  return std::toupper(char1, locale) == std::toupper(char2, locale);
                              });
    if (findIt != std::end(string))
    {
        return true;
    }
    return false;
}

/**
* \brief Case-insensitive string comparison.
* \return true if strings are equals; false in other case.
*/
template <typename StringType>
bool CompareIgnoreCase(const StringType& a, const StringType& b,
                       const std::locale& locale = std::locale())
{
    using CharType = typename StringType::value_type;
    if (a.length() == b.length())
    {
        return std::equal(b.begin(), b.end(), a.begin(),
                          [&locale](CharType char1, CharType char2) {
                              return std::toupper(char1, locale) == std::toupper(char2, locale);
                          });
    }
    return false;
}

/**
* \brief Convert string to lower case.
*/
template <typename StringType>
StringType ToLowerCase(const StringType& string,
                       const std::locale& locale = std::locale())
{
    using CharType = typename StringType::value_type;
    StringType ret;
    ret.resize(string.length());
    std::transform(string.begin(), string.end(), ret.begin(),
                   [&locale](CharType char1) {
                       return std::tolower(char1, locale);
                   });
    return ret;
}

/**
* \brief Convert string to upper case.
*/
template <typename StringType>
StringType ToUpperCase(const StringType& string,
                       const std::locale& locale = std::locale())
{
    using CharType = typename StringType::value_type;
    StringType ret;
    ret.resize(string.length());
    std::transform(string.begin(), string.end(), ret.begin(),
                   [&locale](CharType char1) {
                       return std::toupper(char1, locale);
                   });
    return ret;
}

} // end namespace StringUtils
} // end namespace DAVA
