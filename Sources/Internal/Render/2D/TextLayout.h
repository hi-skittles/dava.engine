#ifndef __DAVAENGINE_TEXT_LAYOUT_H__
#define __DAVAENGINE_TEXT_LAYOUT_H__

#include "Base/BaseTypes.h"
#include "Render/2D/Font.h"
#include "Utils/BiDiHelper.h"

namespace DAVA
{
/**
 * \brief TextLayout class
 * \details Using for splitting text to lines with specified width
 */
class TextLayout
{
public:
    struct Line
    {
        uint32 offset = 0;
        uint32 length = 0;
    };

    /**
     * \brief Create TextLayout with word wrap and disabled BiDi transformations
     */
    TextLayout();

    /**
     * \brief Create TextLayout with specified wrap mode and BiDi transformations
     * \param[in] useBiDi true for enabling BiDi transformations
     */
    TextLayout(const bool useBiDi);

    /**
     * \brief Destructor
     */
    virtual ~TextLayout();

    /**
    * \brief Set process text for text splitting
    * \param[in] input text to process
    */
    void Reset(const WideString& input);

    /**
    * \brief Set process text and font for text splitting
    * \param[in] input text to process
    * \param[in] font font for detecting characters sizes
    */
    DAVA_DEPRECATED(void Reset(const WideString& input, const Font& font, float32 size));

    /**
     * \brief Set sizes for characters of text
     * \param[in] charSizes characters sizes
     */
    void SetCharSizes(const Vector<float32>& charSizes);

    /**
     * \brief Return sizes for characters of text
     * \return characters sizes as vector of float
     */
    const Vector<float32>& GetCharSizes() const;

    /**
    * \brief Calculate sizes for characters of text from font
    * \param[in] font specified font
    */
    void CalculateCharSizes(const Font& font, float32 size);

    /**
     * \brief Puts cursor to given position
     * \param[in] position cursor position in input text
     */
    void Seek(const uint32 position);

    /**
     * \brief Returns current cursor position
     * \return current cursor position
     */
    const uint32 Tell() const;

    /**
     * \brief Checks that text didn't finished yet
     * \return false if not all text processed
     */
    bool IsEndOfText();

    /**
     * \brief Split text by words from current cursor position with specified width
     * \param[in] lineWidth maximum line width in pixels
     * \return true if text can be split by words
     */
    bool NextByWords(const float32 lineWidth);

    /**
     * \brief Split text by symbols from current cursor position with specified width
     * \param[in] lineWidth maximum line width in pixels
     * \return always return true and set in current line minimum one symbol
     */
    bool NextBySymbols(const float32 lineWidth);

    /**
     * \brief Checks that input text is Right-To-Left
     * \return true if input text is Right-To-Left text or has Right-To-Left blocks
     */
    const bool IsRtlText() const;

    /**
    * \brief Return information about current line (offset of first character
    *        and length
    * \return line information
    */
    const Line& GetLine() const;

    /**
     * \brief Returns original text
     * \return original text that sets with TextLayout::Reset
     */
    const WideString& GetOriginalText() const;

    /**
     * \brief Returns internal representation of original text
     * \return text after BiDi transformations without reordering
     */
    const WideString& GetPreparedText() const;

    /**
     * \brief Returns visual representation of original text
     * \param[in] trimEnd true for trims whitespace characters on line end
     * \return text after BiDi transformations with reordering and removing non-printable characters
     */
    const WideString GetVisualText(const bool trimEnd) const;

    /**
     * \brief Returns internal representation of last split line
     * \return last split line after BiDi transformations without reordering
     */
    const WideString GetPreparedLine() const;

    /**
    * \brief Returns internal representation of last split line
    * \param[in] line line information
    * \return last split line after BiDi transformations without reordering
    */
    const WideString GetPreparedLine(const Line& line) const;

    /**
     * \brief Returns visual representation of last split line
     * \param[in] trimEnd true for trims whitespace characters on line end
     * \return last after BiDi transformations with reordering and removing non-printable characters
     */
    DAVA_DEPRECATED(const WideString GetVisualLine(const bool trimEnd) const);

    /**
     * \brief Fill vector of split lines with specified width
     * \param[out] outputList vector of slit lines
     * \param[in] lineWidth maximum line width in pixels
     * \param[in] splitBySymbols true for split only by symbols
     * \param[in] trimEnd true for trims whitespace characters on each line end
     */
    DAVA_DEPRECATED(void FillList(Vector<WideString>& outputList, float32 lineWidth, bool splitBySymbols, bool trimEnd));

    /**
    * \brief Returns visual representation of last split line
    * \param[in] line line information
    * \param[in] trimEnd true for trims whitespace characters on line end
    * \return last after BiDi transformations with reordering and removing non-printable characters
    */
    const WideString GetVisualLine(const Line& line, const bool trimEnd) const;

    /**
    * \brief Fill vector of split lines information with specified width
    * \param[out] outputList vector of slit lines information
    * \param[in] lineWidth maximum line width in pixels
    * \param[in] splitBySymbols true for split only by symbols
    * \param[in] trimEnd true for trims whitespace characters on each line end
    */
    void FillList(Vector<Line>& outputList, float32 lineWidth, bool splitBySymbols);

private:
    /**
     * \brief Returns string after BiDi reordering and removing non-printable characters
     * \param[in] input string for processing
     * \param[in] trimEnd true for trims whitespace characters on line end
     * \return string after BiDi transformations with reordering and removing non-printable characters
     */
    const WideString BuildVisualString(const WideString& input, const bool trimEnd) const;

    /**
     * \brief Correct sizes for special BiDi characters
     */
    void PrepareCharSizes();

    WideString inputText;
    WideString preparedText;

    bool useBiDi;
    bool isRtl;

    Vector<float32> characterSizes;
    Vector<uint8> breaks;
    BiDiHelper bidiHelper;
    Line lineData;
};

inline const Vector<float32>& TextLayout::GetCharSizes() const
{
    return characterSizes;
}

inline const WideString TextLayout::GetPreparedLine() const
{
    return GetPreparedLine(GetLine());
}

inline const WideString TextLayout::GetPreparedLine(const Line& line) const
{
    return preparedText.substr(line.offset, line.length);
}

inline const WideString& TextLayout::GetOriginalText() const
{
    return inputText;
}

inline const WideString& TextLayout::GetPreparedText() const
{
    return preparedText;
}

inline const bool TextLayout::IsRtlText() const
{
    return useBiDi && isRtl;
}

inline const TextLayout::Line& TextLayout::GetLine() const
{
    return lineData;
}

inline const uint32 TextLayout::Tell() const
{
    return lineData.offset;
}

inline const WideString TextLayout::GetVisualLine(const bool trimEnd) const
{
    return GetVisualLine(GetLine(), trimEnd);
}
}

#endif // __DAVAENGINE_TEXT_LAYOUT_H__
