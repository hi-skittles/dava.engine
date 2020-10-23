#include "Utils/UTF8Walker.h"
#include "Logger/Logger.h"

#include <utf8.h>

namespace DAVA
{
UTF8Walker::UTF8Walker() = default;

UTF8Walker::UTF8Walker(const String& utf)
{
    Reset(utf);
}

UTF8Walker::UTF8Walker(const UTF8Walker& src)
    : utfSource(src.utfSource)
    , cursor(src.cursor)
    , size(src.size)
    , length(src.length)
    , currentCodepoint(src.currentCodepoint)
    , currentUtf8Character(src.currentUtf8Character)
    , currentLineBreakType(src.currentLineBreakType)
    , currentIsWhitespace(src.currentIsWhitespace)
    , currentIsLineBreak(src.currentIsLineBreak)
    , currentIsPrintable(src.currentIsPrintable)
    , breaks(src.breaks)
{
}

void UTF8Walker::Reset()
{
    cursor = 0;
    size = static_cast<uint32>(utfSource.size());
    length = 0;
    currentCodepoint = 0;
    currentUtf8Character.clear();
    currentLineBreakType = StringUtils::LB_MUSTBREAK;
    currentIsWhitespace = false;
    currentIsLineBreak = false;
    currentIsPrintable = false;

    // Append null character at end, because unibreak library always
    // mark last character as "must break".
    StringUtils::GetLineBreaks(utfSource + '\0', breaks);
    breaks.resize(size);

    auto begin = utfSource.begin();
    auto end = utfSource.end();
    try
    {
        while (begin != end)
        {
            utf8::next(begin, end);
            length++;
        }
    }
    catch (const utf8::exception& e)
    {
        Logger::Error(e.what());
    }
}

void UTF8Walker::Reset(const String& utf)
{
    utfSource = utf;
    Reset();
}

bool UTF8Walker::Next()
{
    currentUtf8Character.clear();

    StringUtils::eLineBreakType br = StringUtils::LB_INSIDECHAR;
    while (cursor < size && br == StringUtils::LB_INSIDECHAR)
    {
        currentUtf8Character += utfSource[cursor];
        br = static_cast<StringUtils::eLineBreakType>(breaks[cursor]);
        cursor++;
    }

    if (!currentUtf8Character.empty())
    {
        try
        {
            auto begin = currentUtf8Character.begin();
            auto end = currentUtf8Character.end();
            currentCodepoint = utf8::next(begin, end);
        }
        catch (const utf8::exception& e)
        {
            Logger::Error(e.what());
        }

        currentLineBreakType = br;
        currentIsWhitespace = StringUtils::IsWhitespace(static_cast<char16>(currentCodepoint));
        currentIsPrintable = StringUtils::IsPrintable(static_cast<char16>(currentCodepoint));
        currentIsLineBreak = currentLineBreakType == StringUtils::LB_MUSTBREAK;

        return true;
    }
    else
    {
        currentCodepoint = 0;
        currentLineBreakType = StringUtils::LB_MUSTBREAK;
        currentIsWhitespace = false;
        currentIsPrintable = false;
        currentIsLineBreak = false;
        return false;
    }
}
}
