#pragma once

#include "Base/BaseMath.h"
#include "Base/BaseObject.h"
#include "Base/BaseTypes.h"
#include "Render/2D/Font.h"
#include "Render/2D/Sprite.h"
#include "Render/RenderBase.h"
#include "Render/Texture.h"

namespace DAVA
{
class TextBlockRender;
class TextBlockSoftwareRender;
class TextBlockGraphicRender;
class TextBox;

/**
    \ingroup render_2d
    \brief Class to render text on the screen. 
    This class support to draw single line / multiline text to sprites using font objects that available in SDK.
    Normally you do not need it directly and you can use UIStaticText or TextGameObject. 
    */
class TextBlock : public BaseObject
{
public:
    enum eFitType
    {
        FITTING_ENLARGE = 0x1,
        FITTING_REDUCE = 0x2,
        FITTING_POINTS = 0x4,
    };

    enum eUseRtlAlign
    {
        RTL_DONT_USE,
        RTL_USE_BY_CONTENT,
        RTL_USE_BY_SYSTEM
    };

    static void ScreenResolutionChanged();
    /**
    * \brief Sets BiDi transformation support enabled.
    * \param value true to enable BiDi support.
    */
    static void SetBiDiSupportEnabled(bool value);

    /**
    * \brief Is BiDi transformations support enabled.
    * \return true if BiDi transformations supported.
    */
    static bool IsBiDiSupportEnabled();

    static TextBlock* Create(const Vector2& size);

    virtual void SetFont(Font* font);
    virtual void SetScale(const Vector2& scale);
    virtual void SetRectSize(const Vector2& size);
    virtual void SetPosition(const Vector2& position);
    virtual void SetAlign(int32 align);
    virtual int32 GetAlign();
    virtual int32 GetVisualAlign(); // Return align for displaying BiDi-text (w/ mutex lock)
    virtual void SetUseRtlAlign(eUseRtlAlign useRtlAlign);
    virtual eUseRtlAlign GetUseRtlAlign();
    virtual bool IsRtl();

    //[DO NOT ACTUAL ANYMORE] if requested size is 0 - text creates in the rect with size of the drawRect on draw phase
    //if requested size is >0 - text creates int the rect with the requested size
    //if requested size in <0 - rect creates for the all text size
    virtual void SetText(const WideString& string, const Vector2& requestedTextRectSize = Vector2(0, 0));
    virtual void SetMultiline(bool isMultilineEnabled, bool bySymbol = false);
    virtual void SetFittingOption(int32 fittingType); //may be FITTING_ENLARGE, FITTING_REDUCE, FITTING_ENLARGE | FITTING_REDUCE, FITTING_POINTS

    Vector2 GetPreferredSizeForWidth(float32 width);

    virtual Font* GetFont();
    virtual const WideString& GetText();
    virtual const WideString& GetVisualText();
    virtual const Vector<WideString>& GetMultilineStrings();
    virtual TextBox* GetTextBox();
    virtual bool GetMultiline();
    virtual bool GetMultilineBySymbol();
    virtual int32 GetFittingOption();

    /**
    \brief Get the render size.
    \returns size in pixels
    */
    virtual float32 GetRenderSize();
    void SetFontSize(float32 size);
    float32 GetFontSize() const;

    Sprite* GetSprite();
    bool IsSpriteReady();
    const Vector2& GetSpriteOffset();

    const Vector2& GetTextSize();

    void PreDraw();
    void Draw(const Color& textColor, const Vector2* offset = NULL);

    TextBlock* Clone();
    void CopyDataFrom(TextBlock* block);

    const Vector<float32>& GetStringSizes();

    /** Calculated fitting */
    int32 GetFittingOptionUsed();
    bool IsVisualTextCroped();

    TextBlockRender* GetRenderer()
    {
        return textBlockRender;
    }

    bool IsForceBiDiSupportEnabled() const
    {
        return forceBiDiSupport;
    }
    void SetForceBiDiSupportEnabled(bool value);

    bool IsMeasureEnabled() const
    {
        return needMeasureLines;
    }
    void SetMeasureEnable(bool measure);

    void SetAngle(const float32 _angle);
    void SetPivot(const Vector2& _pivot);

    bool NeedCalculateCacheParams() const
    {
        return needCalculateCacheParams;
    }

private:
    static void RegisterTextBlock(TextBlock* textBlock);
    static void UnregisterTextBlock(TextBlock* textBlock);
    static void InvalidateAllTextBlocks();

    TextBlock();
    TextBlock(const TextBlock& src);
    virtual ~TextBlock();

    void NeedPrepare(Texture* texture = NULL);
    void PrepareInternal();

    void CalculateCacheParams();
    void CalculateCacheParamsIfNeed();

    void SetFontInternal(Font* _font);

    Vector2 scale;
    Vector2 rectSize;
    Vector2 position;
    Vector2 requestedSize;

    Vector2 cacheFinalSize;
    Vector2 cacheSpriteOffset;
    Vector2 cacheTextSize;
    struct
    {
        Vector2 size;
        float32 width;
    } cachedLayoutData;

    float32 renderSize;
    float32 fontSize;

    int32 cacheDx;
    int32 cacheDy;
    int32 cacheW;
    int32 cacheOx;
    int32 cacheOy;

    int32 fittingType = 0;
    int32 fittingTypeUsed = 0;
    bool visualTextCroped = false;
    int32 align;
    eUseRtlAlign useRtlAlign;
    int32 visualAlign;

    Font* font;
    WideString logicalText;
    WideString visualText;
    Vector<WideString> multilineStrings;
    Vector<float32> stringSizes;

    bool isMultilineEnabled : 1;
    bool isMultilineBySymbolEnabled : 1;
    bool isPredrawed : 1;
    bool cacheUseJustify : 1;
    bool treatMultilineAsSingleLine : 1;
    bool needPrepareInternal : 1;
    bool isRtl : 1;
    bool needCalculateCacheParams : 1;
    bool forceBiDiSupport : 1;
    bool needMeasureLines : 1;

    static bool isBiDiSupportEnabled; //!< true if BiDi transformation support enabled
    static Set<TextBlock*> registredTextBlocks;
    static Mutex textblockListMutex;

    friend class TextBlockRender;
    friend class TextBlockSoftwareRender;
    friend class TextBlockGraphicRender;

    TextBlockRender* textBlockRender = nullptr;
    TextBox* textBox = nullptr;

    float angle;
    Vector2 pivot;

public:
};

inline void TextBlock::SetPosition(const Vector2& _position)
{
    position = _position;
}

inline void TextBlock::SetAngle(const float32 _angle)
{
    angle = _angle;
}

inline void TextBlock::SetPivot(const Vector2& _pivot)
{
    pivot = _pivot;
}

inline Font* TextBlock::GetFont()
{
    return font;
}

inline const WideString& TextBlock::GetText()
{
    return logicalText;
}

inline bool TextBlock::GetMultiline()
{
    return isMultilineEnabled;
}

inline bool TextBlock::GetMultilineBySymbol()
{
    return isMultilineBySymbolEnabled;
}

inline int32 TextBlock::GetFittingOption()
{
    return fittingType;
}

inline TextBlock::eUseRtlAlign TextBlock::GetUseRtlAlign()
{
    return useRtlAlign;
}

inline int32 TextBlock::GetAlign()
{
    return align;
}

inline bool TextBlock::IsSpriteReady()
{
    return (GetSprite() != nullptr);
}

inline bool TextBlock::IsBiDiSupportEnabled()
{
    return isBiDiSupportEnabled;
}

}; //end of namespace
