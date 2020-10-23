#ifndef __DAVAENGINE_TEXTBLOCK_GRAPHIC_RENDER_H__
#define __DAVAENGINE_TEXTBLOCK_GRAPHIC_RENDER_H__

#include "Render/2D/TextBlockRender.h"
#include "Render/2D/GraphicFont.h"
#include "Base/FastName.h"
#include "Render/UniqueStateSet.h"

namespace DAVA
{
class TextBlockGraphicRender : public TextBlockRender
{
public:
    enum
    {
        TextVerticesDefaultStride = 5
    };

public:
    TextBlockGraphicRender(TextBlock*);
    ~TextBlockGraphicRender();

    TextBlockRender* Clone() override;

    void Prepare() override;
    void Draw(const Color& textColor, const Vector2* offset) override;

    static const uint16* GetSharedIndexBuffer();
    static const uint32 GetSharedIndexBufferCapacity();

protected:
    Font::StringMetrics DrawTextSL(const WideString& drawText, int32 x, int32 y, int32 w) override;
    Font::StringMetrics DrawTextML(const WideString& drawText,
                                   int32 x, int32 y, int32 w,
                                   int32 xOffset, uint32 yOffset,
                                   int32 lineSize) override;

private:
    Font::StringMetrics InternalDrawText(const WideString& drawText, int32 x, int32 y, int32 w, int32 lineSize);

private:
    static uint16* indexBuffer;
    Vector<GraphicFont::GraphicFontVertex> vertexBuffer;
    GraphicFont* graphicFont = nullptr;
    Rect renderRect;
    NMaterial* dfMaterial = nullptr;
    uint32 charDrawed = 0;
    float32 cachedSpread = 0.0f;
};

}; //end of namespace

#endif //__DAVAENGINE_TEXTBLOCK_DISTANCE_RENDER_H__