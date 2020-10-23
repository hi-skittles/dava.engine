#include "Base/Exception.h"
#include "Debug/DVAssert.h"
#include "Logger/Logger.h"
#include "Utils/UTF8Utils.h"

#include <utf8.h>

#include <cwctype>

namespace DAVA
{

#ifdef __DAVAENGINE_WINDOWS__
static_assert(sizeof(char16) == 2, "check size of wchar_t on current platform");
#else
static_assert(sizeof(char16) == 4, "check size of wchar_t on current platform");
#endif

void UTF8Utils::EncodeToWideString(const uint8* string, size_t size, WideString& result)
{
    DVASSERT(nullptr != string);
    result.clear();
    result.reserve(size); // minimum they will be same

    try
    {
#ifdef __DAVAENGINE_WINDOWS__
        utf8::utf8to16(string, string + size, std::back_inserter(result));
#else
        utf8::utf8to32(string, string + size, std::back_inserter(result));
#endif
    }
    catch (const utf8::exception& exception)
    {
        String msg = "UTF8->WideString Conversion error: " + String(exception.what());
        Logger::Warning(msg.c_str());
        DAVA_THROW(Exception, msg);
    }
};

void UTF8Utils::SafeEncodeToWideString(const uint8* string, size_t size, WideString& result, eSafeEncodeError& encodeError)
{
    DVASSERT(nullptr != string);
    result.clear();
    result.reserve(size); // minimum they will be same
    encodeError = eSafeEncodeError::NONE;
    try
    {
#ifdef __DAVAENGINE_WINDOWS__
        utf8::utf8to16(string, string + size, std::back_inserter(result));
#else
        utf8::utf8to32(string, string + size, std::back_inserter(result));
#endif
    }
    catch (const utf8::exception&)
    {
        String replacedString;
        replacedString.reserve(size);
        encodeError = eSafeEncodeError::NON_UTF8_SYMBOLS_REPLACED;
        try
        {
            utf8::replace_invalid(string, string + size, std::back_inserter(replacedString), '?');
            result.clear();
#ifdef __DAVAENGINE_WINDOWS__
            utf8::utf8to16(replacedString.begin(), replacedString.end(), std::back_inserter(result));
#else
            utf8::utf8to32(replacedString.begin(), replacedString.end(), std::back_inserter(result));
#endif
        }
        catch (const utf8::exception&)
        {
            encodeError = eSafeEncodeError::STRING_NOT_ENCODED;
        }
    }
};

String UTF8Utils::EncodeToUTF8(const WideString& wstring)
{
    String result;
    result.reserve(wstring.size()); // minimum they will be same

    try
    {
#ifdef __DAVAENGINE_WINDOWS__
        utf8::utf16to8(wstring.begin(), wstring.end(), std::back_inserter(result));
#else
        utf8::utf32to8(wstring.begin(), wstring.end(), std::back_inserter(result));
#endif
    }
    catch (const utf8::exception& exception)
    {
        String msg = "WideString->UTF8 Conversion error: " + String(exception.what());
        Logger::Warning(msg.c_str());
        DAVA_THROW(Exception, msg);
    }

    return result;
};

String UTF8Utils::EncodeToUTF8(const char32_t* str)
{
    String utf8string;
    const char32_t* strEnd = str + std::char_traits<char32_t>::length(str);
    utf8::utf32to8(str, strEnd, back_inserter(utf8string));
    return utf8string;
}

String UTF8Utils::Trim(const String& str)
{
    return UTF8Utils::TrimLeft(UTF8Utils::TrimRight(str));
}

String UTF8Utils::TrimLeft(const String& str)
{
    String::const_iterator begin = str.begin();
    String::const_iterator end = str.end();
    String::const_iterator it = begin;
    try
    {
        while (it != end)
        {
            uint32_t code = utf8::next(it, end);
            if (std::iswspace(static_cast<wint_t>(code)))
            {
                begin = it;
            }
            else
            {
                break;
            }
        }
        return String(begin, end);
    }
    catch (const utf8::exception& e)
    {
        String msg = "UTF8 Trim begin error: " + String(e.what());
        Logger::Warning(msg.c_str());
        DAVA_THROW(Exception, msg);
    }
}

String UTF8Utils::TrimRight(const String& str)
{
    String::const_iterator begin = str.begin();
    String::const_iterator end = str.end();
    String::const_iterator it = end;
    try
    {
        while (it != begin)
        {
            uint32_t code = utf8::prior(it, begin);
            if (std::iswspace(static_cast<wint_t>(code)))
            {
                end = it;
            }
            else
            {
                break;
            }
        }
        return String(begin, end);
    }
    catch (const utf8::exception& e)
    {
        String msg = "UTF8 Trim end error: " + String(e.what());
        Logger::Warning(msg.c_str());
        DAVA_THROW(Exception, msg);
    }
}

} // namespace DAVA
