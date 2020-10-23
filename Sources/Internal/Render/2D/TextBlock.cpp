#include "Render/2D/TextBlock.h"
#include "Engine/Engine.h"
#include "UI/UIControlSystem.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"
#include "Render/2D/TextBlockSoftwareRender.h"
#include "Render/2D/TextBlockGraphicRender.h"
#include "Render/2D/TextLayout.h"
#include "Concurrency/LockGuard.h"
#include "Utils/TextBox.h"
#include "Utils/StringUtils.h"
#include "Logger/Logger.h"

#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerMarkerNames.h"

#include <numeric>

namespace DAVA
{
#define NEW_RENDER 1

namespace TextBlockDetail
{
static const float32 INVALID_WIDTH = -2.0f;
static const Vector2 INVALID_VECTOR = Vector2(-1.0f, -1.0f);
}

bool TextBlock::isBiDiSupportEnabled = false;
Set<TextBlock*> TextBlock::registredTextBlocks;
Mutex TextBlock::textblockListMutex;

void TextBlock::RegisterTextBlock(TextBlock* textBlock)
{
    LockGuard<Mutex> lock(textblockListMutex);
    registredTextBlocks.insert(textBlock);
}

void TextBlock::UnregisterTextBlock(TextBlock* textBlock)
{
    LockGuard<Mutex> lock(textblockListMutex);
    registredTextBlocks.erase(textBlock);
}

void TextBlock::InvalidateAllTextBlocks()
{
    Logger::FrameworkDebug("Invalidate all text blocks");
    LockGuard<Mutex> lock(textblockListMutex);
    for (auto textBlock : registredTextBlocks)
    {
        textBlock->NeedPrepare();
    }
}

void TextBlock::ScreenResolutionChanged()
{
    InvalidateAllTextBlocks();
}

void TextBlock::SetBiDiSupportEnabled(bool value)
{
    if (isBiDiSupportEnabled != value)
    {
        isBiDiSupportEnabled = value;
        InvalidateAllTextBlocks();
    }
}

TextBlock* TextBlock::Create(const Vector2& size)
{
    TextBlock* textSprite = new TextBlock();
    textSprite->SetRectSize(size);
    return textSprite;
}

TextBlock::TextBlock()
    : scale(1.f, 1.f)
    , cacheFinalSize(0.f, 0.f)
    , cacheTextSize(0.f, 0.f)
    , cachedLayoutData({ TextBlockDetail::INVALID_VECTOR, TextBlockDetail::INVALID_WIDTH })
    , renderSize(1.f)
    , fontSize(14.f)
    , cacheDx(0)
    , cacheDy(0)
    , cacheW(0)
    , cacheOx(0)
    , cacheOy(0)
    , needCalculateCacheParams(false)
    , forceBiDiSupport(false)
    , needMeasureLines(false)
    , textBox(new TextBox())
    , angle(0.f)
{
    font = NULL;
    isMultilineEnabled = false;
    useRtlAlign = RTL_DONT_USE;

    align = ALIGN_HCENTER | ALIGN_VCENTER;
    RegisterTextBlock(this);

    isMultilineBySymbolEnabled = false;
    treatMultilineAsSingleLine = false;
    isRtl = false;

    textBlockRender = NULL;
    needPrepareInternal = false;
}

TextBlock::TextBlock(const TextBlock& src)
    : scale(src.scale)
    , rectSize(src.rectSize)
    , requestedSize(src.requestedSize)
    , cacheFinalSize(src.cacheFinalSize)
    , cacheSpriteOffset(src.cacheSpriteOffset)
    , cacheTextSize(src.cacheTextSize)
    , cachedLayoutData(src.cachedLayoutData)
    , renderSize(src.renderSize)
    , fontSize(src.fontSize)
    , cacheDx(src.cacheDx)
    , cacheDy(src.cacheDy)
    , cacheW(src.cacheW)
    , cacheOx(src.cacheOx)
    , cacheOy(src.cacheOy)
    , fittingType(src.fittingType)
    , fittingTypeUsed(src.fittingTypeUsed)
    , visualTextCroped(src.visualTextCroped)
    , align(src.align)
    , useRtlAlign(src.useRtlAlign)
    , font(nullptr)
    , logicalText(src.logicalText)
    , visualText(src.visualText)
    , multilineStrings(src.multilineStrings)
    , stringSizes(src.stringSizes)
    , isMultilineEnabled(src.isMultilineEnabled)
    , isMultilineBySymbolEnabled(src.isMultilineBySymbolEnabled)
    , isPredrawed(src.isPredrawed)
    , cacheUseJustify(src.cacheUseJustify)
    , treatMultilineAsSingleLine(src.treatMultilineAsSingleLine)
    , needPrepareInternal(src.needPrepareInternal)
    , isRtl(src.isRtl)
    , needCalculateCacheParams(src.needCalculateCacheParams)
    , forceBiDiSupport(src.forceBiDiSupport)
    , needMeasureLines(src.needMeasureLines)
    , textBlockRender(nullptr)
    , textBox(new TextBox(*src.textBox))
    , angle(src.angle)
    , pivot(src.pivot)
{
    //SetFont without Prepare
    if (nullptr != src.font)
    {
        SetFontInternal(src.font);
    }

    RegisterTextBlock(this);
}

TextBlock::~TextBlock()
{
    SafeDelete(textBox);
    SafeRelease(textBlockRender);
    SafeRelease(font);
    UnregisterTextBlock(this);
}

// Setters // Getters

void TextBlock::SetFontInternal(Font* _font)
{
    SafeRelease(font);
    font = SafeRetain(_font);

    SafeRelease(textBlockRender);
    switch (font->GetFontType())
    {
    case Font::TYPE_FT:
        textBlockRender = new TextBlockSoftwareRender(this);
        break;
    case Font::TYPE_GRAPHIC:
    case Font::TYPE_DISTANCE:
        textBlockRender = new TextBlockGraphicRender(this);
        break;

    default:
        DVASSERT(!"Unknown font type");
        break;
    }
}

void TextBlock::SetFont(Font* _font)
{
    if (_font && _font != font)
    {
        SetFontInternal(_font);
        NeedPrepare();
    }
}

void TextBlock::SetRectSize(const Vector2& size)
{
    if (rectSize != size)
    {
        rectSize = size;
        NeedPrepare();
    }
}

void TextBlock::SetScale(const Vector2& _scale)
{
    if (scale != _scale)
    {
        scale = _scale;
        NeedPrepare();
    }
}

void TextBlock::SetText(const WideString& _string, const Vector2& requestedTextRectSize)
{
    if (logicalText != _string || requestedSize != requestedTextRectSize)
    {
        requestedSize = requestedTextRectSize;
        logicalText = _string;
        NeedPrepare();
    }
}

void TextBlock::SetMultiline(bool _isMultilineEnabled, bool bySymbol)
{
    if (isMultilineEnabled != _isMultilineEnabled || isMultilineBySymbolEnabled != bySymbol)
    {
        isMultilineBySymbolEnabled = bySymbol;
        isMultilineEnabled = _isMultilineEnabled;
        NeedPrepare();
    }
}

void TextBlock::SetFittingOption(int32 _fittingType)
{
    if (fittingType != _fittingType)
    {
        fittingType = _fittingType;
        NeedPrepare();
    }
}

void TextBlock::SetAlign(int32 _align)
{
    if (align != _align)
    {
        align = _align;
        NeedPrepare();
    }
}

void TextBlock::SetUseRtlAlign(eUseRtlAlign _useRtlAlign)
{
    if (useRtlAlign != _useRtlAlign)
    {
        useRtlAlign = _useRtlAlign;
        NeedPrepare();
    }
}

DAVA::TextBox* TextBlock::GetTextBox()
{
    CalculateCacheParamsIfNeed();
    return textBox;
}

const Vector<WideString>& TextBlock::GetMultilineStrings()
{
    CalculateCacheParamsIfNeed();
    return multilineStrings;
}

const WideString& TextBlock::GetVisualText()
{
    CalculateCacheParamsIfNeed();
    return visualText;
}

float32 TextBlock::GetRenderSize()
{
    CalculateCacheParamsIfNeed();
    return renderSize;
}

void TextBlock::SetFontSize(float32 size)
{
    if (!FLOAT_EQUAL(fontSize, size))
    {
        fontSize = Max(size, 0.1f);
        NeedPrepare();
    }
}

float32 TextBlock::GetFontSize() const
{
    return fontSize;
}

const Vector2& TextBlock::GetTextSize()
{
    CalculateCacheParamsIfNeed();
    return cacheTextSize;
}

const Vector<float32>& TextBlock::GetStringSizes()
{
    CalculateCacheParamsIfNeed();
    return stringSizes;
}

void TextBlock::SetForceBiDiSupportEnabled(bool value)
{
    if (forceBiDiSupport != value)
    {
        forceBiDiSupport = value;
        NeedPrepare();
    }
}

void TextBlock::SetMeasureEnable(bool value)
{
    if (needMeasureLines != value)
    {
        needMeasureLines = value;
        NeedPrepare();
    }
}

const Vector2& TextBlock::GetSpriteOffset()
{
    CalculateCacheParamsIfNeed();
    return cacheSpriteOffset;
}

bool TextBlock::IsRtl()
{
    CalculateCacheParamsIfNeed();
    return isRtl;
}

int32 TextBlock::GetVisualAlign()
{
    CalculateCacheParamsIfNeed();
    return visualAlign;
}

int32 TextBlock::GetFittingOptionUsed()
{
    CalculateCacheParamsIfNeed();
    return fittingTypeUsed;
}

bool TextBlock::IsVisualTextCroped()
{
    CalculateCacheParamsIfNeed();
    return visualTextCroped;
}

Vector2 TextBlock::GetPreferredSizeForWidth(float32 width)
{
    using namespace TextBlockDetail;

    if (!font)
        return Vector2();

    if (!NeedCalculateCacheParams() &&
        cachedLayoutData.size != INVALID_VECTOR &&
        cachedLayoutData.width == width)
    {
        return cachedLayoutData.size;
    }

    if (requestedSize.dx < 0.0f && requestedSize.dy < 0.0f && fittingType == 0)
    {
        CalculateCacheParamsIfNeed();
        cachedLayoutData.size = cacheTextSize;
        cachedLayoutData.width = width;
    }
    else
    {
        RefPtr<TextBlock> clone(new TextBlock(*this));
        clone->requestedSize = Vector2(width, -1.0f);
        clone->rectSize = Vector2(width < 0.0f ? 99999.0f : width, 99999.0f);
        clone->fittingType = 0;
        clone->CalculateCacheParams();

        cachedLayoutData.size = clone->cacheTextSize;
        cachedLayoutData.width = width;
    }

    return cachedLayoutData.size;
}

Sprite* TextBlock::GetSprite()
{
    if (textBlockRender)
    {
        return textBlockRender->GetSprite();
    }
    return nullptr;
}

void TextBlock::NeedPrepare(Texture* texture /*=NULL*/)
{
    needCalculateCacheParams = true;
    needPrepareInternal = true;
    cachedLayoutData.size = TextBlockDetail::INVALID_VECTOR;
    cachedLayoutData.width = TextBlockDetail::INVALID_WIDTH;
}

void TextBlock::PrepareInternal()
{
    needPrepareInternal = false;
    if (textBlockRender)
    {
        textBlockRender->Prepare();
    }
}

void TextBlock::CalculateCacheParams()
{
    DAVA_PROFILER_CPU_SCOPE(ProfilerCPUMarkerName::UI_TEXTBLOCK_RECALC_PARAMS);

    stringSizes.clear();
    multilineStrings.clear();

    fittingTypeUsed = 0;
    visualTextCroped = false;

    if (logicalText.empty() || font == nullptr)
    {
        visualText.clear();
        isRtl = false;
        cacheFinalSize = Vector2(0.f, 0.f);
        cacheW = 0;
        cacheDx = 0;
        cacheDy = 0;
        cacheOx = 0;
        cacheOy = 0;
        cacheSpriteOffset = Vector2(0.f, 0.f);
        cacheTextSize = Vector2(0.f, 0.f);
        textBox->SetText(L"");

        if (((align & (ALIGN_LEFT | ALIGN_RIGHT)) != 0) &&
            (useRtlAlign == RTL_USE_BY_SYSTEM && GetEngineContext()->uiControlSystem->IsRtl()))
        {
            // Mirror left/right align
            visualAlign = align ^ (ALIGN_LEFT | ALIGN_RIGHT);
        }
        else
        {
            visualAlign = align;
        }
        return;
    }

    Vector2 drawSize = rectSize;
    if (requestedSize.dx > 0)
    {
        drawSize.x = requestedSize.dx;
    }
    if (requestedSize.dy > 0)
    {
        drawSize.y = requestedSize.dy;
    }
    bool useJustify = ((align & ALIGN_HJUSTIFY) != 0);

    renderSize = fontSize * scale.y;

    bool useBiDi = IsBiDiSupportEnabled() || IsForceBiDiSupportEnabled();

    textBox->SetText(logicalText);
    if (useBiDi)
    {
        isRtl = textBox->GetBaseDirection() == TextBox::Direction::RTL;
        textBox->ChangeDirectionMode(isRtl ? TextBox::DirectionMode::STRONG_RTL : TextBox::DirectionMode::AUTO);
        textBox->Shape();
    }

    if (((align & (ALIGN_LEFT | ALIGN_RIGHT)) != 0) &&
        ((useRtlAlign == RTL_USE_BY_CONTENT && isRtl) ||
         (useRtlAlign == RTL_USE_BY_SYSTEM && GetEngineContext()->uiControlSystem->IsRtl())))
    {
        // Mirror left/right align
        visualAlign = align ^ (ALIGN_LEFT | ALIGN_RIGHT);
    }
    else
    {
        visualAlign = align;
    }

    // Store processed (shaped or original) text as visual text
    visualText = textBox->GetProcessedText();

    Vector<uint8> breaks;
    StringUtils::GetLineBreaks(visualText, breaks);

    Vector<float32> charactersSizes;
    Font::StringMetrics textMetrics = font->GetStringMetrics(renderSize, visualText, &charactersSizes);

    TextBox::WrapMode multilineWrapMode = isMultilineBySymbolEnabled ? TextBox::WrapMode::SYMBOLS_WRAP : TextBox::WrapMode::WORD_WRAP;
    if (isMultilineEnabled)
    {
        // Split text into lines with line breaking algorithm and width limit
        textBox->Split(multilineWrapMode, breaks, charactersSizes, drawSize.dx);
        treatMultilineAsSingleLine = textBox->GetLinesCount() == 1;
    }

    if (!isMultilineEnabled || treatMultilineAsSingleLine)
    {
        // Store real visual text (after reordering)
        if (useBiDi)
        {
            textBox->Reorder();
        }
        textBox->CleanUpVisualLines();
        visualText = textBox->GetLine(0).visualString;

        Vector<float32> visualCharactersSizes;
        textMetrics = font->GetStringMetrics(renderSize, visualText, &visualCharactersSizes);

        WideString pointsStr;
        if ((fittingType & FITTING_POINTS) && (drawSize.x < textMetrics.width))
        {
            static float32 FT_WIDTH_EPSILON = 0.3f;

            uint32 length = static_cast<uint32>(visualCharactersSizes.size());
            Font::StringMetrics pointsMetric = font->GetStringMetrics(renderSize, L"...");
            float32 fullWidth = static_cast<float32>(textMetrics.width + pointsMetric.width) - FT_WIDTH_EPSILON;
            pointsStr.clear();
            for (uint32 i = length; i > 0U; --i)
            {
                if (fullWidth <= drawSize.x)
                {
                    fittingTypeUsed = FITTING_POINTS;
                    pointsStr.append(visualText, 0, i);
                    pointsStr += L"...";
                    break;
                }
                fullWidth -= visualCharactersSizes[i - 1];
            }
            if (pointsStr.empty())
            {
                pointsStr = L"...";
            }
        }
        else if (!((fittingType & FITTING_REDUCE) || (fittingType & FITTING_ENLARGE)) && (drawSize.x < textMetrics.width) && (requestedSize.x >= 0))
        {
            uint32 length = static_cast<uint32>(visualCharactersSizes.size());
            float32 fullWidth = static_cast<float32>(textMetrics.width);
            if (ALIGN_RIGHT & visualAlign)
            {
                for (uint32 i = 0U; i < length; ++i)
                {
                    if (fullWidth <= drawSize.x)
                    {
                        pointsStr.clear();
                        pointsStr.append(visualText, i, length - i);
                        break;
                    }
                    fullWidth -= visualCharactersSizes[i];
                }
            }
            else if (ALIGN_HCENTER & visualAlign)
            {
                uint32 left = 0U;
                uint32 right = length - 1;
                bool cutFromBegin = false;

                while (left != right)
                {
                    if (fullWidth <= drawSize.x)
                    {
                        pointsStr.clear();
                        pointsStr.append(visualText, left, right - left + 1);
                        break;
                    }

                    if (cutFromBegin)
                    {
                        fullWidth -= visualCharactersSizes[left++];
                    }
                    else
                    {
                        fullWidth -= visualCharactersSizes[right--];
                    }
                    cutFromBegin = !cutFromBegin;
                }
            }
            else if (ALIGN_LEFT & visualAlign)
            {
                for (uint32 i = 1U; i < length; ++i)
                {
                    fullWidth -= visualCharactersSizes[length - i];
                    if (fullWidth <= drawSize.x)
                    {
                        pointsStr.clear();
                        pointsStr.append(visualText, 0, length - i);
                        break;
                    }
                }
            }
        }
        else if (((fittingType & FITTING_REDUCE) || (fittingType & FITTING_ENLARGE)) && (requestedSize.dy >= 0 || requestedSize.dx >= 0))
        {
            bool isChanged = false;
            float32 prevFontSize = renderSize;
            while (true)
            {
                float32 yMul = 1.0f;
                float32 xMul = 1.0f;

                bool xBigger = false;
                bool xLower = false;
                bool yBigger = false;
                bool yLower = false;
                if (requestedSize.dy >= 0)
                {
                    if ((isChanged || fittingType & FITTING_REDUCE) && textMetrics.height > drawSize.y)
                    {
                        if (prevFontSize < renderSize)
                        {
                            renderSize = prevFontSize;
                            textMetrics = font->GetStringMetrics(renderSize, visualText);
                            break;
                        }
                        yBigger = true;
                        yMul = drawSize.y / textMetrics.height;
                    }
                    else if ((isChanged || fittingType & FITTING_ENLARGE) && textMetrics.height < drawSize.y * 0.9)
                    {
                        yLower = true;
                        yMul = (drawSize.y * 0.9f) / textMetrics.height;
                        if (yMul < 1.01f)
                        {
                            yLower = false;
                        }
                    }
                }

                if (requestedSize.dx >= 0)
                {
                    if ((isChanged || fittingType & FITTING_REDUCE) && textMetrics.width > drawSize.x)
                    {
                        if (prevFontSize < renderSize)
                        {
                            renderSize = prevFontSize;
                            textMetrics = font->GetStringMetrics(renderSize, visualText);
                            break;
                        }
                        xBigger = true;
                        xMul = drawSize.x / textMetrics.width;
                    }
                    else if ((isChanged || fittingType & FITTING_ENLARGE) && textMetrics.width < drawSize.x * 0.95)
                    {
                        xLower = true;
                        xMul = (drawSize.x * 0.95f) / textMetrics.width;
                        if (xMul < 1.01f)
                        {
                            xLower = false;
                        }
                    }
                }

                if (((!xBigger && !yBigger) && (!xLower || !yLower)) || FLOAT_EQUAL(renderSize, 0.f))
                {
                    break;
                }

                float32 finalSize = renderSize;
                prevFontSize = finalSize;
                isChanged = true;
                if (xMul < yMul)
                {
                    finalSize *= xMul;
                }
                else
                {
                    finalSize *= yMul;
                }
                {
                    float mult = DAVA::Min(xMul, yMul);
                    if (mult > 1.0f)
                        fittingTypeUsed |= FITTING_ENLARGE;
                    else if (mult < 1.0f)
                        fittingTypeUsed |= FITTING_REDUCE;
                }
                renderSize = finalSize;
                textMetrics = font->GetStringMetrics(renderSize, visualText);
            }
        }

        if (!pointsStr.empty())
        {
            visualText = pointsStr;
            textMetrics = font->GetStringMetrics(renderSize, visualText);
            visualTextCroped = true;
        }

        if (needMeasureLines)
        {
            textBox->Measure(charactersSizes, float32(textMetrics.height), 0, 1);
        }

        if (treatMultilineAsSingleLine)
        {
            // Another temporary solution to return correct multiline strings/
            // string sizes.
            multilineStrings.push_back(visualText);
            stringSizes.push_back(textMetrics.width);
        }
    }
    else //if(isMultilineEnabled)
    {
        DVASSERT(textBox->GetLinesCount() > 0, "Empty lines information");

        int32 yOffset = font->GetVerticalSpacing();
        float32 fontHeight = float32(font->GetFontHeight(renderSize) + yOffset);
        textMetrics.width = 0.f;
        textMetrics.drawRect.dx = 0;
        textMetrics.height = fontHeight * textBox->GetLinesCount() - yOffset;
        textMetrics.drawRect.dy = int32(std::ceil(textMetrics.height));

        if ((fittingType & FITTING_POINTS) && textMetrics.height > drawSize.y)
        {
            const uint32 actualLinesCount = textBox->GetLinesCount();
            const uint32 drawableLinesCount = Min(actualLinesCount, Max(1U, static_cast<uint32>((drawSize.y + yOffset + DAVA::EPSILON) / fontHeight)));
            const TextBox::Line& lastDrawableLine = textBox->GetLine(drawableLinesCount - 1);
            WideString lineText = lastDrawableLine.visualString;

            Vector<float32> lineCharactersSizes;
            Font::StringMetrics textMetrics = font->GetStringMetrics(renderSize, lineText, &lineCharactersSizes);
            WideString pointsStr;

            static float32 FT_WIDTH_EPSILON = 0.3f;

            uint32 length = static_cast<uint32>(lineCharactersSizes.size());
            Font::StringMetrics pointsMetric = font->GetStringMetrics(renderSize, L"...");
            float32 fullWidth = static_cast<float32>(textMetrics.width + pointsMetric.width) - FT_WIDTH_EPSILON;
            for (uint32 i = length; i > 0U; --i)
            {
                uint32 logicalTextEnd = lastDrawableLine.start + i;
                if (fullWidth <= drawSize.x && !StringUtils::IsWhitespace(logicalText[logicalTextEnd - 1]))
                {
                    fittingTypeUsed = FITTING_POINTS;
                    pointsStr.assign(logicalText, 0, logicalTextEnd);
                    break;
                }
                fullWidth -= lineCharactersSizes[i - 1];
            }

            pointsStr.append(L"...");
            visualText = pointsStr;

            textBox->SetText(visualText);
            charactersSizes.clear();
            StringUtils::GetLineBreaks(visualText, breaks);
            font->GetStringMetrics(renderSize, visualText, &charactersSizes);
            textBox->Split(multilineWrapMode, breaks, charactersSizes, drawSize.dx);

            fontHeight = float32(font->GetFontHeight(renderSize) + yOffset);
            textMetrics.height = fontHeight * textBox->GetLinesCount() - yOffset;
            textMetrics.drawRect.dy = int32(std::ceil(textMetrics.height));

            visualTextCroped = true;
        }
        else if (fittingType && (requestedSize.dy >= 0 /* || requestedSize.dx >= 0*/) && visualText.size() > 3)
        {
            float32 lastSize = renderSize;
            bool isChanged = false;
            while (true)
            {
                float32 yMul = 1.0f;

                bool yBigger = false;
                bool yLower = false;
                if (requestedSize.dy >= 0)
                {
                    if ((isChanged || fittingType & FITTING_REDUCE) && textMetrics.height > drawSize.y)
                    {
                        yBigger = true;
                        yMul = drawSize.y / textMetrics.height;
                        if (lastSize < renderSize)
                        {
                            renderSize = lastSize;
                            font->GetStringMetrics(renderSize, visualText, &charactersSizes);
                            textBox->Split(multilineWrapMode, breaks, charactersSizes, drawSize.dx);

                            fontHeight = float32(font->GetFontHeight(renderSize) + yOffset);
                            textMetrics.height = fontHeight * textBox->GetLinesCount() - yOffset;
                            textMetrics.drawRect.dy = int32(std::ceil(textMetrics.height));
                            break;
                        }
                    }
                    else if ((isChanged || fittingType & FITTING_ENLARGE) && textMetrics.height < drawSize.y * 0.95)
                    {
                        yLower = true;
                        if (textMetrics.height < drawSize.y * 0.75f)
                        {
                            yMul = (drawSize.y * 0.75f) / textMetrics.height;
                        }
                        else if (textMetrics.height < drawSize.y * 0.8f)
                        {
                            yMul = (drawSize.y * 0.8f) / textMetrics.height;
                        }
                        else if (textMetrics.height < drawSize.y * 0.85f)
                        {
                            yMul = (drawSize.y * 0.85f) / textMetrics.height;
                        }
                        else if (textMetrics.height < drawSize.y * 0.9f)
                        {
                            yMul = (drawSize.y * 0.9f) / textMetrics.height;
                        }
                        else
                        {
                            yMul = (drawSize.y * 0.95f) / textMetrics.height;
                        }
                        if (FLOAT_EQUAL(yMul, 1.0f))
                        {
                            yMul = 1.05f;
                        }
                    }
                }

                if ((!yBigger && !yLower) || FLOAT_EQUAL(renderSize, 0.f))
                {
                    break;
                }

                float32 finalSize = lastSize = renderSize;
                isChanged = true;
                finalSize *= yMul;

                if (yMul > 1.0f)
                {
                    fittingTypeUsed |= FITTING_ENLARGE;
                }
                if (yMul < 1.0f)
                {
                    fittingTypeUsed |= FITTING_REDUCE;
                }

                renderSize = finalSize;
                font->GetStringMetrics(renderSize, visualText, &charactersSizes);
                textBox->Split(multilineWrapMode, breaks, charactersSizes, drawSize.dx);

                fontHeight = float32(font->GetFontHeight(renderSize) + yOffset);
                textMetrics.height = fontHeight * textBox->GetLinesCount() - yOffset;
                textMetrics.drawRect.dy = int32(std::ceil(textMetrics.height));
            };
        }

        if (useBiDi)
        {
            textBox->Reorder();
        }
        textBox->CleanUpVisualLines();
        const Vector<TextBox::Line>& lines = textBox->GetLines();

        // Detect visible lines
        int32 fromLine = 0;
        int32 linesCount = int32(lines.size());
        if (textMetrics.height > drawSize.y && requestedSize.y >= 0.f)
        {
            int32 needLines = Min(linesCount, int32(std::ceil(drawSize.y / fontHeight)) + 1);
            // For align & ALIGN_TOP do nothing
            if (visualAlign & ALIGN_VCENTER)
            {
                fromLine = (linesCount - needLines + 1) / 2;
            }
            else if (visualAlign & ALIGN_BOTTOM)
            {
                fromLine = linesCount - needLines;
            }
            linesCount = needLines;
        }
        textMetrics.height = fontHeight * linesCount - yOffset;
        textMetrics.drawRect.dy = int32(std::ceil(textMetrics.height));

        // Get lines as visual strings and its metrics
        stringSizes.reserve(linesCount);
        multilineStrings.reserve(linesCount);
        for (int32 lineInd = 0; lineInd < linesCount; ++lineInd)
        {
            WideString visualLine = lines.at(fromLine + lineInd).visualString;
            const Font::StringMetrics& stringSize = font->GetStringMetrics(renderSize, visualLine);

            multilineStrings.push_back(visualLine);
            stringSizes.push_back(stringSize.width);

            textMetrics.drawRect.dx = Max(textMetrics.drawRect.dx, stringSize.drawRect.dx + stringSize.drawRect.x);
            textMetrics.drawRect.x = Min(textMetrics.drawRect.x, stringSize.drawRect.x);

            if (requestedSize.dx >= 0)
            {
                textMetrics.width = Max(textMetrics.width, Min(stringSize.width, drawSize.x));
            }
            else
            {
                textMetrics.width = Max(textMetrics.width, stringSize.width);
            }

            // Get draw rectangle Y position from first line only
            if (0 == lineInd)
            {
                textMetrics.drawRect.y = stringSize.drawRect.y;
            }

            if (textMetrics.width < stringSize.width)
            {
                visualTextCroped = true;
            }
        }

        if (needMeasureLines)
        {
            textBox->Measure(charactersSizes, float32(fontHeight), fromLine, linesCount);
        }

        // Translate right/bottom edge to width/height
        textMetrics.drawRect.dx -= textMetrics.drawRect.x;
        textMetrics.drawRect.dy -= textMetrics.drawRect.y;
    }

    if (requestedSize.dx >= 0 && useJustify)
    {
        textMetrics.drawRect.dx = Max(textMetrics.drawRect.dx, int32(drawSize.dx));
    }

    // Measure fix
    if (needMeasureLines)
    {
        float32 lyoffset = 0.f;
        for (TextBox::Line& line : textBox->GetLines())
        {
            if (line.skip)
            {
                continue;
            }

            if (visualAlign & ALIGN_LEFT /*|| align & ALIGN_HJUSTIFY*/)
            {
                line.xoffset = 0.f;
            }
            else if (visualAlign & ALIGN_RIGHT)
            {
                line.xoffset = drawSize.x - line.visiblexadvance;
            }
            else //if (visualAlign & ALIGN_HCENTER)
            {
                line.xoffset = (drawSize.x - line.visiblexadvance) * 0.5f;
            }

            if (visualAlign & ALIGN_TOP)
            {
                line.yoffset = lyoffset;
            }
            else if (visualAlign & ALIGN_BOTTOM)
            {
                line.yoffset = lyoffset + drawSize.y - static_cast<float32>(textMetrics.height);
            }
            else //if (visualAlign & ALIGN_VCENTER)
            {
                line.yoffset = lyoffset + (drawSize.y - static_cast<float32>(textMetrics.height)) * 0.5f;
            }

            lyoffset += line.yadvance;
        }
    }

    //calculate texture size
    int32 dx = int32(std::ceil(GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToPhysicalX(float32(textMetrics.drawRect.dx))));
    int32 dy = int32(std::ceil(GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToPhysicalY(float32(textMetrics.drawRect.dy))));
    int32 ox = int32(std::floor(GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToPhysicalX(float32(textMetrics.drawRect.x))));
    int32 oy = int32(std::floor(GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToPhysicalY(float32(textMetrics.drawRect.y))));

    cacheUseJustify = useJustify;
    cacheDx = dx;
    EnsurePowerOf2(cacheDx);

    cacheDy = dy;
    EnsurePowerOf2(cacheDy);

    cacheOx = ox;
    cacheOy = oy;

    cacheW = int32(std::ceil(drawSize.dx));
    cacheFinalSize.x = float32(textMetrics.drawRect.dx);
    cacheFinalSize.y = float32(textMetrics.drawRect.dy);
    cacheTextSize = Vector2(std::ceil(textMetrics.width), std::ceil(textMetrics.height));

    // Align sprite offset
    if (visualAlign & ALIGN_LEFT /*|| align & ALIGN_HJUSTIFY*/)
    {
        cacheSpriteOffset.x = float32(textMetrics.drawRect.x);
    }
    else if (visualAlign & ALIGN_RIGHT)
    {
        cacheSpriteOffset.x = float32(textMetrics.drawRect.dx - textMetrics.width + textMetrics.drawRect.x);
    }
    else //if (visualAlign & ALIGN_HCENTER)
    {
        cacheSpriteOffset.x = (textMetrics.drawRect.dx - textMetrics.width) * 0.5f + textMetrics.drawRect.x;
    }

    if (visualAlign & ALIGN_TOP)
    {
        cacheSpriteOffset.y = float32(textMetrics.drawRect.y);
    }
    else if (visualAlign & ALIGN_BOTTOM)
    {
        cacheSpriteOffset.y = float32(textMetrics.drawRect.dy - textMetrics.height + textMetrics.drawRect.y);
    }
    else //if (visualAlign & ALIGN_VCENTER)
    {
        cacheSpriteOffset.y = (textMetrics.drawRect.dy - textMetrics.height) * 0.5f + textMetrics.drawRect.y;
    }
}

void TextBlock::CalculateCacheParamsIfNeed()
{
    using namespace TextBlockDetail;
    if (needCalculateCacheParams)
    {
        needCalculateCacheParams = false;
        CalculateCacheParams();
        cachedLayoutData.size = INVALID_VECTOR;
        cachedLayoutData.width = INVALID_WIDTH;
    }
}

void TextBlock::PreDraw()
{
    CalculateCacheParamsIfNeed();

    if (needPrepareInternal)
    {
        DAVA_PROFILER_CPU_SCOPE(ProfilerCPUMarkerName::UI_TEXTBLOCK_PREPARE);
        PrepareInternal();
    }

    if (textBlockRender)
    {
        textBlockRender->PreDraw();
    }
}

void TextBlock::Draw(const Color& textColor, const Vector2* offset /* = NULL*/)
{
    if (textBlockRender)
    {
        textBlockRender->Draw(textColor, offset);
    }
}

TextBlock* TextBlock::Clone()
{
    TextBlock* block = new TextBlock();
    block->CopyDataFrom(this);
    return block;
}

void TextBlock::CopyDataFrom(TextBlock* block)
{
    DVASSERT(block != nullptr);

    SetScale(block->scale);
    SetRectSize(block->rectSize);
    SetMultiline(block->GetMultiline(), block->GetMultilineBySymbol());
    SetAlign(block->align);
    SetFittingOption(block->fittingType);
    SetUseRtlAlign(block->useRtlAlign);
    SetForceBiDiSupportEnabled(block->forceBiDiSupport);
    SetMeasureEnable(block->needMeasureLines);

    if (block->font != nullptr)
    {
        if (block->textBlockRender != nullptr)
        {
            SafeRelease(font);
            SafeRelease(textBlockRender);

            font = SafeRetain(block->font);
            textBlockRender = block->textBlockRender->Clone();
            textBlockRender->SetTextBlock(this);
        }
        else
        {
            SetFont(block->font);
        }
    }

    SetText(block->GetText(), block->requestedSize);
}
};
