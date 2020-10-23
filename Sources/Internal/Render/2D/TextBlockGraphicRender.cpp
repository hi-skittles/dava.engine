#include "Render/2D/TextBlockGraphicRender.h"
#include "Engine/Engine.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"
#include "Render/2D/Systems/RenderSystem2D.h"
#include "Render/Material/NMaterial.h"
#include "Render/Renderer.h"
#include "Render/ShaderCache.h"
#include "UI/UIControlSystem.h"

namespace DAVA
{
static uint16* InitIndexBuffer()
{
    static uint16 buffer[GRAPHIC_FONT_INDEX_BUFFER_SIZE];

    uint16 a = 0;
    for (int32 i = 0; i < GRAPHIC_FONT_INDEX_BUFFER_SIZE;)
    {
        buffer[i] = buffer[i + 3] = a;
        buffer[i + 1] = a + 1;
        buffer[i + 2] = buffer[i + 4] = a + 2;
        buffer[i + 5] = a + 3;
        i += 6;
        a += 4;
    }
    return buffer;
}

uint16* TextBlockGraphicRender::indexBuffer = InitIndexBuffer();

TextBlockGraphicRender::TextBlockGraphicRender(TextBlock* textBlock)
    : TextBlockRender(textBlock)
    , dfMaterial(SafeRetain(RenderSystem2D::DEFAULT_2D_TEXTURE_MATERIAL))
    , charDrawed(0)
    , cachedSpread(0)
{
    graphicFont = static_cast<GraphicFont*>(textBlock->GetFont());

    if (graphicFont->GetFontType() == Font::TYPE_DISTANCE)
    {
        cachedSpread = graphicFont->GetSpread(textBlock->renderSize);
        dfMaterial = new NMaterial();
        dfMaterial->SetFXName(FastName("~res:/Materials/2d.DistanceFont.material"));
        dfMaterial->SetMaterialName(FastName("DistanceFontMaterial"));
        dfMaterial->AddProperty(FastName("smoothing"), &cachedSpread, rhi::ShaderProp::TYPE_FLOAT1);
        dfMaterial->PreBuildMaterial(RenderSystem2D::RENDER_PASS_NAME);
    }
}

TextBlockGraphicRender::~TextBlockGraphicRender()
{
    SafeRelease(dfMaterial);
}

TextBlockRender* TextBlockGraphicRender::Clone()
{
    TextBlockGraphicRender* result = new TextBlockGraphicRender(textBlock);
    result->cachedSpread = cachedSpread;
    result->charDrawed = charDrawed;
    result->vertexBuffer = vertexBuffer;
    result->renderRect = renderRect;
    return result;
}

void TextBlockGraphicRender::Prepare()
{
    size_t charCount = 0;
    if (!textBlock->isMultilineEnabled || textBlock->treatMultilineAsSingleLine)
    {
        charCount = textBlock->visualText.length();
    }
    else
    {
        size_t stringsCnt = textBlock->multilineStrings.size();
        for (size_t line = 0; line < stringsCnt; ++line)
        {
            charCount += textBlock->multilineStrings[line].length();
        }
    }
    size_t vertexCount = charCount * 4;

    if (vertexBuffer.size() != vertexCount)
    {
        vertexBuffer.resize(vertexCount);
    }

    charDrawed = 0;
    renderRect = Rect(0, 0, 0, 0);
    DrawText();
}

void TextBlockGraphicRender::Draw(const Color& textColor, const Vector2* offset)
{
    if (charDrawed == 0)
        return;

    int32 xOffset = 0; // (int32)(textBlock->position.x);
    int32 yOffset = 0; // (int32)(textBlock->position.y);

    if (offset)
    {
        xOffset += int32(offset->x);
        yOffset += int32(offset->y);
    }

    int32 align = textBlock->GetVisualAlign();
    if (align & ALIGN_RIGHT)
    {
        xOffset += int32(textBlock->rectSize.dx - renderRect.dx);
    }
    else if ((align & ALIGN_HCENTER) || (align & ALIGN_HJUSTIFY))
    {
        xOffset += int32((textBlock->rectSize.dx - renderRect.dx) * 0.5f);
    }

    if (align & ALIGN_BOTTOM)
    {
        yOffset += int32(textBlock->rectSize.dy - renderRect.dy);
    }
    else if ((align & ALIGN_VCENTER) || (align & ALIGN_HJUSTIFY))
    {
        yOffset += int32((textBlock->rectSize.dy - renderRect.dy) * 0.5f);
    }

    //NOTE: correct affine transformations
    Matrix4 offsetMatrix;
    offsetMatrix.BuildTranslation(Vector3(float32(xOffset) - textBlock->pivot.x, float32(yOffset) - textBlock->pivot.y, 0.f));

    Matrix4 rotateMatrix;
    rotateMatrix.BuildRotation(Vector3(0.f, 0.f, 1.f), -textBlock->angle);

    Matrix4 scaleMatrix;
    //recalculate x scale - for non-uniform scale
    const float difX = 1.0f - (textBlock->scale.dy - textBlock->scale.dx);
    scaleMatrix.BuildScale(Vector3(difX, 1.f, 1.0f));

    Matrix4 worldMatrix;
    worldMatrix.BuildTranslation(Vector3(textBlock->position.x, textBlock->position.y, 0.f));

    offsetMatrix = (scaleMatrix * offsetMatrix * rotateMatrix) * worldMatrix;

    if (graphicFont->GetFontType() == Font::TYPE_DISTANCE)
    {
        float32 spread = graphicFont->GetSpread(textBlock->renderSize);
        if (!FLOAT_EQUAL(cachedSpread, spread))
        {
            cachedSpread = spread;
            dfMaterial->SetPropertyValue(FastName("smoothing"), &cachedSpread);
        }
    }

    BatchDescriptor2D batch;
    batch.material = dfMaterial; // RenderSystem2D::DEFAULT_2D_TEXTURE_MATERIAL;
    batch.singleColor = textColor;
    batch.vertexStride = TextVerticesDefaultStride;
    batch.texCoordStride = TextVerticesDefaultStride;
    batch.vertexPointer = vertexBuffer[0].position.data;
    batch.texCoordPointer[0] = vertexBuffer[0].texCoord.data;
    batch.textureSetHandle = graphicFont->GetTexture()->singleTextureSet;
    batch.samplerStateHandle = graphicFont->GetTexture()->samplerStateHandle;
    batch.vertexCount = static_cast<uint32>(vertexBuffer.size());
    batch.indexPointer = indexBuffer;
    batch.indexCount = batch.vertexCount * 6 / 4;
    batch.worldMatrix = &offsetMatrix;
    RenderSystem2D::Instance()->PushBatch(batch);
}

Font::StringMetrics TextBlockGraphicRender::DrawTextSL(const WideString& drawText, int32 x, int32 y, int32 w)
{
    return InternalDrawText(drawText, 0, 0, 0, 0);
}

Font::StringMetrics TextBlockGraphicRender::DrawTextML(const WideString& drawText,
                                                       int32 x, int32 y, int32 w,
                                                       int32 xOffset, uint32 yOffset,
                                                       int32 lineSize)
{
    return InternalDrawText(drawText, xOffset, yOffset, int32(std::ceil(GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToPhysicalX(float32(w)))), lineSize);
}

Font::StringMetrics TextBlockGraphicRender::InternalDrawText(const WideString& drawText, int32 x, int32 y, int32 w, int32 lineSize)
{
    if (drawText.empty())
        return Font::StringMetrics();

    int32 lastDrawed = 0;

    Font::StringMetrics metrics = graphicFont->DrawStringToBuffer(textBlock->renderSize, drawText, x, y, &vertexBuffer[0] + (charDrawed * 4), lastDrawed, NULL, w, lineSize);
    if (metrics.drawRect.dx <= 0 && metrics.drawRect.dy <= 0)
        return metrics;

    renderRect = renderRect.Combine(Rect(float32(metrics.drawRect.x), float32(metrics.drawRect.y), float32(metrics.drawRect.dx), float32(metrics.drawRect.dy)));
    this->charDrawed += lastDrawed;
    return metrics;
}

const uint16* TextBlockGraphicRender::GetSharedIndexBuffer()
{
    return indexBuffer;
}

const uint32 TextBlockGraphicRender::GetSharedIndexBufferCapacity()
{
    return GRAPHIC_FONT_INDEX_BUFFER_SIZE;
}
};
