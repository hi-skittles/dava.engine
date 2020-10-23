#include "DAVAEngine.h"
#include "StringUtils.h"
#include "Utils/UTF8Utils.h"

#include "unibreak/linebreak.h"

namespace DAVA
{
namespace StringUtils
{
void GetLineBreaks(const String& string, Vector<uint8>& breaks, const char8* locale)
{
    breaks.resize(string.length(), LB_NOBREAK); // By default all characters not breakable
    set_linebreaks_utf8(reinterpret_cast<const utf8_t*>(string.c_str()), string.length(), locale, reinterpret_cast<char*>(&breaks.front()));
}

void GetLineBreaks(const WideString& string, Vector<uint8>& breaks, const char8* locale)
{
    breaks.resize(string.length(), LB_NOBREAK); // By default all characters not breakable
#if defined(__DAVAENGINE_WINDOWS__) // sizeof(wchar_t) == 2
    set_linebreaks_utf16(reinterpret_cast<const utf16_t*>(string.c_str()), string.length(), locale, reinterpret_cast<char*>(&breaks.front()));
#else
    set_linebreaks_utf32(reinterpret_cast<const utf32_t*>(string.c_str()), string.length(), locale, reinterpret_cast<char*>(&breaks.front()));
#endif
}

WideString RemoveNonPrintable(const WideString& string, const int8 tabRule /*= -1*/)
{
    WideString out;
    WideString::const_iterator it = string.begin();
    WideString::const_iterator end = string.end();
    for (; it != end; ++it)
    {
        if (!IsPrintable(*it))
        {
            continue;
        }

        switch (*it)
        {
        case L'\t':
            if (tabRule < 0)
            {
                out.push_back(*it); // Leave tab symbol
            }
            else
            {
                out.append(tabRule, L' '); // Replace tab with (tabRule x spaces)
            }
            break;
        case 0x00A0: // Non-break space
            out.push_back(L' ');
            break;
        default:
            out.push_back(*it);
            break;
        }
    }
    return out;
}

void ReplaceAll(WideString& string, const WideString& search, const WideString& replacement)
{
    size_t pos = 0;
    size_t oldSubStringLength = search.length();
    while ((pos = string.find(search, pos)) != WideString::npos)
    {
        string.replace(pos, oldSubStringLength, replacement);
        pos += 1;
    }
}

String SubstituteParams(const String& str, const DAVA::UnorderedMap<DAVA::String, DAVA::String>& replacements)
{
    String result;
    result.reserve(static_cast<int32>(str.length() * 1.5f));

    String::size_type pos = 0;
    String::size_type next = 0;
    while ((next = str.find("%(", pos)) != String::npos)
    {
        if (next > pos)
        {
            result += str.substr(pos, next - pos);
        }

        pos = next;
        next = str.find(")", pos + 1);
        if (next == String::npos)
        {
            DVASSERT(false, str.c_str());
            return str;
        }

        String key = str.substr(pos + 2, next - pos - 2);
        auto it = replacements.find(key);
        if (it != replacements.end())
        {
            result += it->second;
        }
        else
        {
            result += "%(";
            result += key;
            result += ")";
        }
        pos = next + 1;
    }

    if (pos < str.length())
    {
        result += str.substr(pos, str.length() - pos);
    }

    return result;
}
}
}
