#pragma once

#include "Base/BaseTypes.h"
#include "Base/RefPtr.h"
#include "UI/UIControl.h"
#include "Render/2D/TextBlock.h"

namespace DAVA
{
class UITextComponent;

class UIStaticText : public UIControl
{
    DAVA_VIRTUAL_REFLECTION(UIStaticText, UIControl);

public:
    // Use NO_REQUIRED_SIZE to notify SetText that we don't want
    // to enable of any kind of static text fitting
    static const Vector2 NO_REQUIRED_SIZE;
    // Use REQUIRED_CONTROL_SIZE to notify SetText that we want
    // to enable of some kind of static text fitting with staticText control
    // size
    static const Vector2 REQUIRED_CONTROL_SIZE;
    // Use REQUIRED_CONTROL_WIDTH to notify SetText that we want
    // to enable limit of text width and not any limits of text
    // height
    static const Vector2 REQUIRED_CONTROL_WIDTH;
    // Use REQUIRED_CONTROL_HEIGHT to notify SetText that we want
    // to enable limit of text height and not any limits of text
    // width
    static const Vector2 REQUIRED_CONTROL_HEIGHT;

    enum eMultiline
    {
        MULTILINE_DISABLED = 0,
        MULTILINE_ENABLED,
        MULTILINE_ENABLED_BY_SYMBOL
    };

protected:
    virtual ~UIStaticText();

    void LoadFromYamlNodeCompleted() override;

public:
    UIStaticText(const Rect& rect = Rect());

    //if requested size is 0 - text creates in the rect with size of the drawRect on draw phase
    //if requested size is >0 - text creates int the rect with the requested size
    //if requested size in <0 - rect creates for the all text size
    DAVA_DEPRECATED(virtual void SetText(const WideString& string, const Vector2& requestedTextRectSize = Vector2::Zero));

    void SetUtf8Text(const String& utf8String, const Vector2& requestedTextRectSize = Vector2::Zero);
    void SetUtf8TextWithoutRect(const String& utf8String);
    String GetUtf8Text() const;

    void SetFont(Font* font);
    void SetFontSize(float32 newSize);
    void SetTextColor(const Color& color);

    void SetShadowColor(const Color& color);
    void SetShadowOffset(const Vector2& offset);

    void SetMultiline(bool isMultilineEnabled, bool bySymbol = false);
    bool GetMultiline() const;
    bool GetMultilineBySymbol() const;

    void SetFittingOption(int32 fittingType); //may be FITTING_ENLARGE, FITTING_REDUCE, FITTING_ENLARGE | FITTING_REDUCE
    int32 GetFittingOption() const;

    virtual void SetTextAlign(int32 _align);
    virtual int32 GetTextAlign() const;

    virtual int32 GetTextVisualAlign() const;
    virtual bool GetTextIsRtl() const;
    virtual void SetTextUseRtlAlign(TextBlock::eUseRtlAlign useRtlAlign);
    virtual TextBlock::eUseRtlAlign GetTextUseRtlAlign() const;

    virtual const WideString& GetVisualText() const;
    const Vector2& GetTextSize();

    DAVA_DEPRECATED(const WideString& GetText() const);
    const Vector<WideString>& GetMultilineStrings() const;

    Font* GetFont() const;
    float32 GetFontSize() const;

    virtual UIStaticText* Clone() override;
    virtual void CopyDataFrom(UIControl* srcControl) override;
    const Color& GetTextColor() const;
    const Color& GetShadowColor() const;
    const Vector2& GetShadowOffset() const;

    // Animation methods for Text Color and Shadow Color.
    virtual Animation* TextColorAnimation(const Color& finalColor, float32 time, Interpolation::FuncType interpolationFunc = Interpolation::LINEAR, int32 track = 0);
    virtual Animation* ShadowColorAnimation(const Color& finalColor, float32 time, Interpolation::FuncType interpolationFunc = Interpolation::LINEAR, int32 track = 1);

    inline bool IsForceBiDiSupportEnabled() const;
    void SetForceBiDiSupportEnabled(bool value);

    String GetFontPresetName() const;
    void SetFontByPresetName(const String& presetName);

    int32 GetTextColorInheritType() const;
    void SetTextColorInheritType(int32 type);

    int32 GetTextPerPixelAccuracyType() const;
    void SetTextPerPixelAccuracyType(int32 type);

    int32 GetMultilineType() const;
    void SetMultilineType(int32 multilineType);

    TextBlock* GetTextBlock() const;

private:
    RefPtr<UITextComponent> text;
};
};
