#pragma once

#include "Render/2D/TextBlockRender.h"
#include "Render/2D/FTFont.h"

namespace DAVA
{
class TextBlockSoftwareRender : public TextBlockRender
{
public:
    TextBlockSoftwareRender(TextBlock*);
    ~TextBlockSoftwareRender();

    void Prepare() override;
    TextBlockRender* Clone() override;

#if defined(LOCALIZATION_DEBUG)
    //in physical coordinates
    Vector2 getTextOffsetTL();
    //in physical coordinates
    Vector2 getTextOffsetBR();
#endif

private:
    Font::StringMetrics DrawTextSL(const WideString& drawText, int32 x, int32 y, int32 w) override;
    Font::StringMetrics DrawTextML(const WideString& drawText, int32 x, int32 y, int32 w,
                                   int32 xOffset, uint32 yOffset, int32 lineSize) override;

    void Restore();

#if defined(LOCALIZATION_DEBUG)
    void CalculateTextBBox();
#endif

private:
    int8* buf = nullptr;
    FTFont* ftFont = nullptr;
    Texture* currentTexture = nullptr;

#if defined(LOCALIZATION_DEBUG)
    Vector2 textOffsetTL;
    Vector2 textOffsetBR;
    int32 bufHeight = 0;
    int32 bufWidth = 0;
#endif
};

}; //end of namespace
