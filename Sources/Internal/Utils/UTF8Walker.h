#pragma once

#include "Base/BaseTypes.h"
#include "Utils/StringUtils.h"

namespace DAVA
{
/** Class that iterate UTF8 string by unicode characters. */
class UTF8Walker final
{
public:
    /** Default constructor. */
    UTF8Walker();

    /** Constructor with specified utf8 source string. */
    UTF8Walker(const String& utf);

    /** Copy constructor. */
    UTF8Walker(const UTF8Walker& src);

    /** Reset cursor position inside walker. */
    void Reset();

    /** Reset cursor position inside walker and set new source utf8 string. */
    void Reset(const String& utf);

    /** Return size of string in bytes. */
    uint32 GetSize() const;

    /** Return unicode characters count. */
    uint32 GetLength() const;

    /** Try to move cursor to next character. */
    bool Next();

    /** Check that cursor at the end of the string. */
    bool HasNext() const;

    /** Get codepoint of current character. */
    uint32 GetUnicodeCodepoint() const;

    /** Get utf8 sequence of current character. */
    const String& GetUtf8Character() const;

    /** Get line break posibility on current character. */
    StringUtils::eLineBreakType GetLineBreak() const;

    /** Return true if current character is whitespace. */
    bool IsWhitespace() const;

    /** Return true if current character is line break. */
    bool IsLineBreak() const;

    /** Return true if current character is printable. */
    bool IsPrintable() const;

private:
    String utfSource;
    uint32 cursor = 0;
    uint32 size = 0;
    uint32 length = 0;
    uint32 currentCodepoint = 0;
    String currentUtf8Character;
    StringUtils::eLineBreakType currentLineBreakType = StringUtils::LB_MUSTBREAK;
    bool currentIsWhitespace = false;
    bool currentIsLineBreak = false;
    bool currentIsPrintable = false;
    Vector<uint8> breaks;
};

inline uint32 UTF8Walker::GetSize() const
{
    return size;
}

inline uint32 UTF8Walker::GetLength() const
{
    return length;
}

inline bool UTF8Walker::HasNext() const
{
    return cursor < size;
}

inline uint32 UTF8Walker::GetUnicodeCodepoint() const
{
    return currentCodepoint;
}

inline const String& UTF8Walker::GetUtf8Character() const
{
    return currentUtf8Character;
}

inline StringUtils::eLineBreakType UTF8Walker::GetLineBreak() const
{
    return currentLineBreakType;
}

inline bool UTF8Walker::IsWhitespace() const
{
    return currentIsWhitespace;
}

inline bool UTF8Walker::IsLineBreak() const
{
    return currentIsLineBreak;
}

inline bool UTF8Walker::IsPrintable() const
{
    return currentIsPrintable;
}
}
