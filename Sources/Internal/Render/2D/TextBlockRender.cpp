#include "Render/2D/TextBlockRender.h"

namespace DAVA
{
TextBlockRender::TextBlockRender(TextBlock* textBlock_)
    : textBlock(textBlock_)
{
}

TextBlockRender::~TextBlockRender()
{
    SafeRelease(sprite);
}

void TextBlockRender::Prepare()
{
    SafeRelease(sprite);
}

void TextBlockRender::DrawText()
{
    if (!textBlock->isMultilineEnabled || textBlock->treatMultilineAsSingleLine)
    {
        DrawTextSL(textBlock->visualText, textBlock->cacheDx, textBlock->cacheDy, textBlock->cacheW);
    }
    else
    {
        uint32 yOffset = 0;
        int32 stringSize = 0;
        int32 blockWidth = 0;
        int32 fontHeight = textBlock->font->GetFontHeight(textBlock->renderSize) + textBlock->font->GetVerticalSpacing();
        int32 stringsCnt = int32(textBlock->multilineStrings.size());
        for (int32 line = 0; line < stringsCnt; ++line)
        {
            bool justify = textBlock->cacheUseJustify;
            if (line == int32(textBlock->multilineStrings.size()) - 1)
            {
                justify = false;
            }
            int32 xOffset = 0;
            int32 align = textBlock->GetVisualAlign();
            if (align & ALIGN_RIGHT)
            {
                xOffset = int32(textBlock->cacheTextSize.dx - textBlock->stringSizes[line] + textBlock->cacheSpriteOffset.x);
                if (xOffset < 0)
                {
                    xOffset = 0;
                }
            }
            else if (align & ALIGN_HCENTER)
            {
                xOffset = int32(textBlock->cacheTextSize.dx - textBlock->stringSizes[line] + textBlock->cacheSpriteOffset.x) / 2;
                if (xOffset < 0)
                {
                    xOffset = 0;
                }
            }
            if (align & ALIGN_HJUSTIFY && justify)
            {
                stringSize = int32(std::ceil(textBlock->stringSizes[line]));
                blockWidth = textBlock->cacheW;
            }
            else
            {
                stringSize = 0;
                blockWidth = 0;
            }
            DrawTextML(textBlock->multilineStrings[line],
                       textBlock->cacheDx,
                       textBlock->cacheDy,
                       blockWidth,
                       xOffset,
                       yOffset,
                       stringSize);

            yOffset += fontHeight;
        }
    }
}
};