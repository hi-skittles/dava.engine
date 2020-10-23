#pragma once

#include "Base/BaseTypes.h"
#include "Utils/BiDiHelper.h"

namespace DAVA
{
class TextBoxImpl;

class TextBox final
{
public:
    enum class DirectionMode : uint8
    {
        AUTO = 0,
        WEAK_LTR,
        WEAK_RTL,
        STRONG_LTR,
        STRONG_RTL
    };

    enum class Direction : uint8
    {
        LTR = 0,
        RTL,
        MIXED
    };

    enum class WrapMode : uint8
    {
        NO_WRAP = 0,
        WORD_WRAP,
        SYMBOLS_WRAP
    };

    struct Line
    {
        uint32 index = 0;
        uint32 start = 0;
        uint32 length = 0;
        float32 xadvance = 0.f;
        float32 visiblexadvance = 0.f;
        float32 yadvance = 0.f;
        float32 xoffset = 0.f;
        float32 yoffset = 0.f;
        WideString visualString;
        bool skip = false;

        static Line EMPTY;
    };

    struct Character
    {
        uint32 logicIndex = 0;
        uint32 visualIndex = 0;
        uint32 lineIndex = 0;
        float32 xadvance = 0.f;
        float32 xoffset = 0.f;
        bool rtl = false;
        bool skip = false;
        bool hiden = false;

        static Character EMPTY;
    };

    TextBox();
    TextBox(const TextBox& box);
    ~TextBox();

    void SetText(const WideString& str, const DirectionMode mode = DirectionMode::AUTO);
    void ChangeDirectionMode(const DirectionMode mode);
    void Shape();
    void Split(const WrapMode mode = WrapMode::NO_WRAP, const Vector<uint8>& breaks = Vector<uint8>(), const Vector<float32>& widths = Vector<float32>(), const float32 maxWidth = 0.f);
    void Reorder();
    void Measure(const Vector<float32>& characterSizes, float32 lineHeight, uint32 fromLine, uint32 toLine);
    void CleanUpVisualLines();

    const WideString& GetText() const;
    const WideString& GetProcessedText() const;
    Direction GetBaseDirection() const;
    Direction GetDirection() const;
    const Vector<Line>& GetLines() const;
    Vector<Line>& GetLines();
    const Line& GetLine(const uint32 index) const;
    Line& GetLine(const uint32 index);
    uint32 GetLinesCount() const;
    const Vector<Character>& GetCharacters() const;
    Vector<Character>& GetCharacters();
    const Character& GetCharacter(const uint32 index) const;
    Character& GetCharacter(const uint32 index);
    uint32 GetCharactersCount() const;

private:
    void ClearLines();
    void AddLineRange(uint32 start, uint32 length);

    WideString logicalText;
    WideString processedText;
    Vector<Line> lines;
    Vector<Character> characters;
    std::unique_ptr<TextBoxImpl> pImpl;
};

inline const DAVA::WideString& TextBox::GetText() const
{
    return logicalText;
}

inline const WideString& TextBox::GetProcessedText() const
{
    return processedText;
}

inline const Vector<TextBox::Line>& TextBox::GetLines() const
{
    return lines;
}

inline Vector<TextBox::Line>& TextBox::GetLines()
{
    return lines;
}

inline const TextBox::Line& TextBox::GetLine(const uint32 index) const
{
    return lines.at(index);
}

inline TextBox::Line& TextBox::GetLine(const uint32 index)
{
    return lines.at(index);
}

inline uint32 TextBox::GetLinesCount() const
{
    return uint32(lines.size());
}

inline const Vector<TextBox::Character>& TextBox::GetCharacters() const
{
    return characters;
}

inline Vector<TextBox::Character>& TextBox::GetCharacters()
{
    return characters;
}

inline const TextBox::Character& TextBox::GetCharacter(const uint32 index) const
{
    return characters.at(index);
}

inline TextBox::Character& TextBox::GetCharacter(const uint32 index)
{
    return characters.at(index);
}

inline uint32 TextBox::GetCharactersCount() const
{
    return uint32(characters.size());
}
}