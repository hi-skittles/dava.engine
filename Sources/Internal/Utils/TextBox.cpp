#include "TextBox.h"
#include "Debug/DVAssert.h"
#include "Logger/Logger.h"
#include "Utils/StringUtils.h"
#include "Utils/StringFormat.h"

#define U_COMMON_IMPLEMENTATION
#define U_STATIC_IMPLEMENTATION
#if __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wheader-hygiene"
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif
#include <unicode/ubidi.h>
#include <unicode/ushape.h>
#if __clang__
#pragma clang diagnostic pop
#endif

// Disable UTF16<->UTF8<->UTF32 for fast conversion
#define USE_UTF8_CONVERSION 0

#if defined(__DAVAENGINE_WIN32__) || defined(__DAVAENGINE_WIN_UAP__)
static_assert(sizeof(DAVA::WideString::value_type) == sizeof(UChar), "Check size of wchar_t and UChar");
#else
static_assert(sizeof(DAVA::WideString::value_type) == 4, "Check size of wchar_t equal 4");
static_assert(sizeof(UChar) == 2, "Check size of UChar equal 2");
#if USE_UTF8_CONVERSION
#include <utf8.h>
#endif
#endif

namespace DAVA
{
using UCharString = BasicString<UChar>;

// TODO: linux
#if defined(__DAVAENGINE_LINUX__)

class TextBoxImpl final
{
public:
    TextBoxImpl(TextBox* /*tb*/)
    {
    }
    TextBoxImpl(TextBox* /*tb*/, const TextBoxImpl& /*src*/)
    {
    }
    TextBoxImpl(const TextBoxImpl& /*src*/) = delete;
    ~TextBoxImpl() = default;

    void SetString(const WideString& /*str*/, const TextBox::DirectionMode /*mode*/)
    {
    }
    void ChangeDirectionMode(const TextBox::DirectionMode /*mode*/)
    {
    }
    void Shape()
    {
    }
    void ReorderLines()
    {
    }
    WideString AsString() const
    {
        return WideString();
    }
    TextBox::Direction GetDirection() const
    {
        return TextBox::Direction::LTR;
    }
    TextBox::Direction GetBaseDirection() const
    {
        return TextBox::Direction::LTR;
    }
};

#else

class TextBoxImpl final
{
public:
    TextBoxImpl(TextBox* tb);
    TextBoxImpl(TextBox* tb, const TextBoxImpl& src);
    TextBoxImpl(const TextBoxImpl& src) = delete;
    ~TextBoxImpl();

    void SetString(const WideString& str, const TextBox::DirectionMode mode);
    void ChangeDirectionMode(const TextBox::DirectionMode mode);
    void Shape();
    void ReorderLines();
    WideString AsString() const;
    TextBox::Direction GetDirection() const;
    TextBox::Direction GetBaseDirection() const;

private:
    static UBiDiLevel DirectionModeToBiDiLevel(const TextBox::DirectionMode mode);
    static TextBox::Direction BiDiDirectionToDirection(const UBiDiDirection dir);
    static WideString ConvertU2W(const UCharString& src);
    static UCharString ConvertW2U(const WideString& src);

    void UpdateParagraph();

    UCharString uString;
    UBiDi* paragraph = nullptr;
    TextBox* tb = nullptr;
    UBiDiLevel baseLevel = UBIDI_DEFAULT_LTR;
};

TextBoxImpl::TextBoxImpl(TextBox* tb_)
    : tb(tb_)
{
    paragraph = ubidi_open();
    DVASSERT(paragraph != nullptr, "Can't alocate new paragraph");
}

TextBoxImpl::TextBoxImpl(TextBox* tb_, const TextBoxImpl& src)
    : tb(tb_)
{
    paragraph = ubidi_open();
    DVASSERT(paragraph != nullptr, "Can't alocate new paragraph");
    uString = src.uString;
    baseLevel = src.baseLevel;
    UpdateParagraph();
}

TextBoxImpl::~TextBoxImpl()
{
    if (paragraph != nullptr)
    {
        ubidi_close(paragraph);
    }
}

void TextBoxImpl::SetString(const WideString& str, const TextBox::DirectionMode mode)
{
    uString = ConvertW2U(str);
    baseLevel = DirectionModeToBiDiLevel(mode);
    UpdateParagraph();
}

void TextBoxImpl::ChangeDirectionMode(const TextBox::DirectionMode mode)
{
    baseLevel = DirectionModeToBiDiLevel(mode);
    UpdateParagraph();
}

void TextBoxImpl::UpdateParagraph()
{
    if (paragraph == nullptr)
    {
        Logger::Error("[TextBox::UpdatePara] pararagraph == nullptr");
        DVASSERT(false, "[TextBox::UpdatePara] pararagraph == nullptr");
        return;
    }

    UErrorCode errorCode = U_ZERO_ERROR;
    ubidi_setPara(paragraph, uString.data(), int32(uString.length()), baseLevel, nullptr, &errorCode);
    if (errorCode != U_ZERO_ERROR)
    {
        Logger::Error("[TextBox::UpdatePara] errorCode = %d", errorCode);
        DVASSERT(false, Format("[TextBox::UpdatePara] errorCode", errorCode).c_str());
    }
}

void TextBoxImpl::Shape()
{
    if (paragraph == nullptr)
    {
        Logger::Error("[TextBox::UpdatePara] pararagraph == nullptr");
        DVASSERT(false, "[TextBox::UpdatePara] pararagraph == nullptr");
        return;
    }

    // Commented options are extended options from ICU 3.6 and 4.2
    // TODO: uncomment for more complex shaping
    static const int32 commonOptions = U_SHAPE_LETTERS_SHAPE /*| U_SHAPE_AGGREGATE_TASHKEEL | U_SHAPE_PRESERVE_PRESENTATION*/;
    static const int32 detectLengthOptions = commonOptions | U_SHAPE_LAMALEF_RESIZE;
    static const int32 shapeOptions = commonOptions | U_SHAPE_LAMALEF_NEAR /*| U_SHAPE_SEEN_TWOCELL_NEAR | U_SHAPE_YEHHAMZA_TWOCELL_NEAR | U_SHAPE_TASHKEEL_REPLACE_BY_TATWEEL*/;

    const UChar* text = ubidi_getText(paragraph);
    const int32 length = ubidi_getLength(paragraph);

    UErrorCode errorCode = U_ZERO_ERROR;
    UCharString output(length, 0);
    u_shapeArabic(text, length, const_cast<UChar*>(output.data()), length, shapeOptions, &errorCode);
    if (errorCode != U_ZERO_ERROR && errorCode != U_STRING_NOT_TERMINATED_WARNING)
    {
        Logger::Error("[TextBox::Shape] shapeArabic2 errorCode = %d", errorCode);
        DVASSERT(false, Format("[TextBox::Shape] shapeArabic errorCode", errorCode).c_str());
    }

    // Check new shaped length
    errorCode = U_ZERO_ERROR;
    int32 outputLength = u_shapeArabic(text, length, nullptr, 0, detectLengthOptions, &errorCode);
    if (errorCode != U_ZERO_ERROR && errorCode != U_BUFFER_OVERFLOW_ERROR)
    {
        Logger::Error("[TextBox::Shape] shapeArabic1 errorCode = %d", errorCode);
        DVASSERT(false, Format("[TextBox::Shape] detect length errorCode %d", errorCode).c_str());
    }
    if (length > outputLength)
    {
        // Fix indices mapping if length of result string reduced
        for (int32 i = 0; i < length; ++i)
        {
            TextBox::Character& c = tb->GetCharacter(i);
            UChar outputChar = output[i];
            UChar inputChar = text[i];
            if (inputChar != ' ' && outputChar == ' ') // According shaping options all merged symbols replace with spaces
            {
                // Merging!
                c.skip = true;
            }
        }
    }
    else if (length < outputLength)
    {
        DVASSERT(false, Format("[TextBox::Shape] Unexpected behaviour (%d, %d)", length, outputLength).c_str());
    }

    // Store shaped text
    uString = output;
    UpdateParagraph();
}

DAVA::WideString TextBoxImpl::AsString() const
{
    return ConvertU2W(uString);
}

void TextBoxImpl::ReorderLines()
{
    // Commented options are extended options for auto clean reordered string
    // Don't use it for now
    static const uint32 reorderOptions = UBIDI_DO_MIRRORING /*| UBIDI_REMOVE_BIDI_CONTROLS*/;

    Vector<int32> l2v;
    UErrorCode errorCode = U_ZERO_ERROR;
    UBiDi* lpara = ubidi_open();
    if (lpara == nullptr)
    {
        Logger::Error("[TextBox::UpdatePara] pararagraph == nullptr");
        DVASSERT(false, "[TextBox::UpdatePara] pararagraph == nullptr");
        return;
    }

    for (TextBox::Line& line : tb->GetLines())
    {
        if (line.length == 0)
        {
            continue;
        }

        errorCode = U_ZERO_ERROR;
        ubidi_setPara(lpara, uString.data() + line.start, line.length, baseLevel, nullptr, &errorCode);
        if (errorCode != U_ZERO_ERROR)
        {
            Logger::Error("[TextBox::ReorderLines] setPara errorCode = %d", errorCode);
            DVASSERT(false, Format("[TextBox::Shape] setPara errorCode", errorCode).c_str());
        }

        // Write reordered string
        errorCode = U_ZERO_ERROR;
        UCharString visString(line.length, 0);
        ubidi_writeReordered(lpara, const_cast<UChar*>(visString.data()), line.length, reorderOptions, &errorCode);
        if (errorCode != U_ZERO_ERROR && errorCode != U_STRING_NOT_TERMINATED_WARNING)
        {
            Logger::Error("[TextBox::ReorderLines] writeReordered errorCode = %d", errorCode);
            DVASSERT(false, Format("[TextBox::ReorderLines] writeReordered errorCode", errorCode).c_str());
        }

        line.visualString = ConvertU2W(visString);

        // Get local reordered characters map
        int32 length = ubidi_getLength(lpara);
        l2v.resize(length);
        errorCode = U_ZERO_ERROR;
        ubidi_getLogicalMap(lpara, l2v.data(), &errorCode);
        if (errorCode != U_ZERO_ERROR)
        {
            Logger::Error("[TextBox::ReorderLines] getLogicalMap errorCode = %d", errorCode);
            DVASSERT(false, Format("[TextBox::ReorderLines] getLogicalMap errorCode", errorCode).c_str());
        }

        // Correct global reordered characters map according local map
        int32 li = line.start;
        for (int32 i = 0; i < length; ++i)
        {
            TextBox::Character& c = tb->GetCharacter(li++);
            if (l2v[i] >= 0)
            {
                c.visualIndex = uint32(l2v[i]) + line.start; // Make local index global (text)
            }
            else
            {
                c.visualIndex = 0;
                c.skip = true;
            }
            c.rtl = (ubidi_getLevelAt(lpara, i) & UBIDI_RTL) == UBIDI_RTL;
        }
    }
    ubidi_close(lpara);
}

TextBox::Direction TextBoxImpl::GetDirection() const
{
    return BiDiDirectionToDirection(ubidi_getDirection(paragraph));
}

TextBox::Direction TextBoxImpl::GetBaseDirection() const
{
    return BiDiDirectionToDirection(ubidi_getBaseDirection(ubidi_getText(paragraph), ubidi_getLength(paragraph)));
}

UBiDiLevel TextBoxImpl::DirectionModeToBiDiLevel(const TextBox::DirectionMode mode)
{
    switch (mode)
    {
    case TextBox::DirectionMode::AUTO:
    case TextBox::DirectionMode::WEAK_LTR:
        return UBIDI_DEFAULT_LTR;
    case TextBox::DirectionMode::WEAK_RTL:
        return UBIDI_DEFAULT_RTL;
    case TextBox::DirectionMode::STRONG_LTR:
        return UBIDI_LTR;
    case TextBox::DirectionMode::STRONG_RTL:
        return UBIDI_RTL;
    }
    DVASSERT(false, "Undefined direction mode");
    return UBIDI_DEFAULT_LTR;
}

DAVA::TextBox::Direction TextBoxImpl::BiDiDirectionToDirection(const UBiDiDirection dir)
{
    switch (dir)
    {
    case UBIDI_LTR:
    case UBIDI_NEUTRAL:
        return TextBox::Direction::LTR;
    case UBIDI_RTL:
        return TextBox::Direction::RTL;
    case UBIDI_MIXED:
        return TextBox::Direction::MIXED;
    }
    DVASSERT(false, "Undefined direction");
    return TextBox::Direction::LTR;
}

WideString TextBoxImpl::ConvertU2W(const UCharString& src)
{
    if (src.empty())
    {
        return WideString();
    }
#if defined(__DAVAENGINE_WIN32__) || defined(__DAVAENGINE_WIN_UAP__)
    return WideString(src);
#elif USE_UTF8_CONVERSION
    String tmp;
    tmp.reserve(src.length());
    utf8::utf16to8(src.begin(), src.end(), std::back_inserter(tmp));
    WideString result;
    result.reserve(tmp.length());
    utf8::utf8to32(tmp.begin(), tmp.end(), std::back_inserter(result));
    return result;
#else
    return WideString(src.begin(), src.end());
#endif
}

UCharString TextBoxImpl::ConvertW2U(const WideString& src)
{
    if (src.empty())
    {
        return UCharString();
    }
#if defined(__DAVAENGINE_WIN32__) || defined(__DAVAENGINE_WIN_UAP__)
    return UCharString(src);
#elif USE_UTF8_CONVERSION
    String tmp;
    tmp.reserve(src.length());
    utf8::utf32to8(src.begin(), src.end(), std::back_inserter(tmp));
    UCharString result;
    result.reserve(tmp.length());
    utf8::utf8to16(tmp.begin(), tmp.end(), std::back_inserter(result));
    return result;
#else
    return UCharString(src.begin(), src.end());
#endif
}
#endif

/******************************************************************************/
/******************************************************************************/

TextBox::Line TextBox::Line::EMPTY = TextBox::Line();
TextBox::Character TextBox::Character::EMPTY = TextBox::Character();

TextBox::TextBox()
    : pImpl(new TextBoxImpl(this))
{
}

TextBox::TextBox(const TextBox& box)
    : logicalText(box.logicalText)
    , processedText(box.processedText)
    , lines(box.lines)
    , characters(box.characters)
    , pImpl(new TextBoxImpl(this, *box.pImpl))
{
}

TextBox::~TextBox()
{
}

void TextBox::ClearLines()
{
    lines.clear();
}

void TextBox::AddLineRange(uint32 start, uint32 length)
{
    TextBox::Line l;
    l.index = uint32(lines.size());
    l.start = start;
    l.length = length;
    l.visualString = processedText.substr(start, length);
    lines.push_back(l);

    // Store line index in character
    uint32 limit = start + length;
    for (uint32 i = start; i < limit; ++i)
    {
        characters[i].lineIndex = l.index;
    }
}

void TextBox::SetText(const WideString& str, const DirectionMode mode)
{
    logicalText = str;

    // Replace some characters for better processing
    // Replace tabs at space character until the renderer will not be able to draw a text columns
    StringUtils::ReplaceAll(logicalText, L"\t", L" ");

    processedText = str;
    uint32 length = uint32(str.length());
    characters.resize(length);
    for (uint32 i = 0; i < length; ++i)
    {
        Character& c = characters.at(i);
        c = Character::EMPTY;
        c.logicIndex = i;
        c.visualIndex = i;
    }

    ClearLines();
    AddLineRange(0, length);

    pImpl->SetString(logicalText, mode);
}

void TextBox::ChangeDirectionMode(const DirectionMode mode)
{
    pImpl->ChangeDirectionMode(mode);
}

void TextBox::Shape()
{
    pImpl->Shape();
    processedText = pImpl->AsString();
}

void TextBox::Split(const WrapMode mode, const Vector<uint8>& breaks, const Vector<float32>& widths, const float32 maxWidth)
{
    ClearLines();
    if (mode == WrapMode::NO_WRAP)
    {
        AddLineRange(0, uint32(processedText.length()));
    }
    else
    {
        DVASSERT(breaks.size() == processedText.length(),
                 Format("Incorrect breaks information (%d != %d)", breaks.size(), processedText.length()).c_str());
        DVASSERT(widths.size() == processedText.length(),
                 Format("Incorrect character sizes information (%d != %d)", widths.size(), processedText.length()).c_str());
        DVASSERT(characters.size() == processedText.length(),
                 Format("Incorrect character information and process text lengths (%d != %d)", widths.size(), processedText.length()).c_str());

        float32 currentWidth = 0;
        uint32 lastPossibleBreak = 0;
        uint32 lineStart = 0;
        uint32 lineLength = 0;
        uint32 textLength = uint32(processedText.length());

        auto addLine = [&](const uint32 pos) {
            currentWidth = 0.f;
            lastPossibleBreak = 0;
            lineLength = pos - lineStart + 1;
            AddLineRange(lineStart, lineLength);
            lineStart += lineLength;
        };

        for (uint32 pos = lineStart; pos < textLength; ++pos)
        {
            Character& ch = characters.at(pos);
            if (ch.skip || ch.hiden)
            {
                continue;
            }

            uint8 canBreak = breaks.at(pos);
            currentWidth += widths.at(pos);

            // Check that targetWidth defined and currentWidth less than targetWidth.
            // If symbol is whitespace skip it and go to next (add all whitespace to current line)
            if (currentWidth <= maxWidth || StringUtils::IsWhitespace(char16(processedText.at(pos))))
            {
                if (canBreak == StringUtils::LB_MUSTBREAK) // If symbol is line breaker then split string
                {
                    addLine(pos);
                    continue;
                }
                else if (canBreak == StringUtils::LB_ALLOWBREAK || mode == WrapMode::SYMBOLS_WRAP) // Store breakable symbol position
                {
                    lastPossibleBreak = pos;
                }
                continue;
            }

            if (lastPossibleBreak > 0) // If we have any breakable symbol in current substring then split by it
            {
                pos = lastPossibleBreak;
            }
            else if (pos > lineStart) // Too big single word in line, split word by symbol
            {
                pos--;
            }
            addLine(pos);
        }
    }
}

void TextBox::Reorder()
{
    if (lines.empty())
    {
        return;
    }
    pImpl->ReorderLines();
}

void TextBox::CleanUpVisualLines()
{
    Set<int32> removeLiterals;
    uint32 linesCount = uint32(lines.size());
    for (TextBox::Line& line : lines)
    {
        if (line.length == 0)
        {
            continue;
        }

        uint32 limit = line.start + line.length;
        // For all but the last line trim trailing spaces
        if (line.index < linesCount - 1)
        {
            // Detect trailing whitespace characters and hide them
            for (uint32 li = limit - 1;; --li)
            {
                if (!StringUtils::IsWhitespace(processedText.at(li)))
                {
                    break;
                }

                Character& ch = GetCharacter(li);
                ch.hiden = true;

                if (li == line.start)
                {
                    break;
                }
            }
        }

        removeLiterals.clear();
        for (uint32 li = line.start; li < limit; ++li)
        {
            Character& ch = GetCharacter(li);
            char16& c = processedText.at(li);

            // Skip non-printable characters
            if (!StringUtils::IsPrintable(c))
            {
                ch.hiden = true;
            }

            if (ch.skip || ch.hiden)
            {
                // Store visual index for erasing
                uint32 vi = characters[li].visualIndex - line.start; // Make global index local (line)
                removeLiterals.insert(vi);
                continue;
            }

            // Replace some visual representation of characters
            if (c == 0x00A0) // Non-break space
            {
                uint32 vi = characters[li].visualIndex - line.start; // Make global index local (line)
                line.visualString.at(vi) = L' ';
            }
        }

        // Erase set of literal indeces from visual string
        auto endReverseIt = removeLiterals.crend();
        for (auto reverseIt = removeLiterals.crbegin(); reverseIt != endReverseIt; ++reverseIt)
        {
            line.visualString.erase(*reverseIt, 1);
        }
    }
}

void TextBox::Measure(const Vector<float32>& characterSizes, float32 lineHeight, uint32 fromLine, uint32 linesCount)
{
    DVASSERT(characterSizes.size() == processedText.size(), Format("Incorrect character sizes information (%d != %d)", characterSizes.size(), processedText.size()).c_str());

    uint32 lineLimit = fromLine + linesCount;
    uint32 linesSize = uint32(lines.size());
    for (uint32 lineIndex = 0; lineIndex < linesSize; ++lineIndex)
    {
        Line& line = lines.at(lineIndex);

        // Do not measure skipped lines
        if (lineIndex < fromLine || lineIndex >= lineLimit)
        {
            line.skip = true;
            continue;
        }

        line.skip = false;
        line.yoffset = (lineIndex - fromLine) * lineHeight;
        line.yadvance = lineHeight;
        line.xoffset = 0.f;
        line.xadvance = 0.f;
        line.visiblexadvance = 0.f;

        // Generate visual order
        Vector<uint32> vo(line.length, -1);
        uint32 limit = line.start + line.length;
        for (uint32 li = line.start; li < limit; ++li)
        {
            const Character& c = characters.at(li);
            int32 vi = c.visualIndex - line.start; // Make global index local (line)
            vo[vi] = c.logicIndex;
        }

        // Layout
        for (uint32 logInd : vo)
        {
            Character& c = characters.at(logInd);
            if (c.skip)
            {
                continue;
            }

            c.xoffset = line.xadvance;
            c.xadvance = characterSizes[logInd];

            if (line.xoffset > c.xoffset)
            {
                line.xoffset = c.xoffset;
            }
            line.xadvance += c.xadvance;
            if (!c.hiden)
            {
                line.visiblexadvance += c.xadvance;
            }
        }
    }
}

DAVA::TextBox::Direction TextBox::GetDirection() const
{
    return pImpl->GetDirection();
}

TextBox::Direction TextBox::GetBaseDirection() const
{
    return pImpl->GetBaseDirection();
}

} // namespace DAVA
