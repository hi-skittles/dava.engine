#pragma once

#include "FileSystem/FilePath.h"
#include "Reflection/Reflection.h"
#include "Render/2D/TextBlock.h"
#include "UI/Components/UIComponent.h"
#include "UI/UIControlBackground.h"
#include "UI/Text/Private/UITextSystemLink.h"

struct UITextSystemTest;

namespace DAVA
{
class UIControl;
class TextFieldStbImpl;

/**
    Text widget component. 
    Display plain text with specified font style and basic layout settings.
*/
class UITextComponent : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UITextComponent, UIComponent);
    DECLARE_UI_COMPONENT(UITextComponent);

public:
    /** Text multiline mode */
    enum eTextMultiline
    {
        /** Single line mode*/
        MULTILINE_DISABLED = 0,
        /** Multiline is enabled. Word wrap. */
        MULTILINE_ENABLED,
        /** Multiline is enabled. By symbol wrap. */
        MULTILINE_ENABLED_BY_SYMBOL
    };

    /** Text fitting style */
    enum eTextFitting
    {
        /** Default style. Display text as-is. */
        FITTING_NONE = 0,
        /** Enlarge content scale when text is small or display as-is in another case. */
        FITTING_ENLARGE,
        /** Reduce content scale when text is too small or display as-is in another case. */
        FITTING_REDUCE,
        /**
            Maximal space filling. Works like combination  ENLARGE | REDUCE.
            Enlarge content scale when text is small and reduce scale when it is too large.         
        */
        FITTING_FILL,
        /** Display only visible text whith points ("...") on end of string if text is too large. */
        FITTING_POINTS
    };

protected:
    /** Prevents unmanagement object destruction. See SafeRelease function and BaseObject class. */
    ~UITextComponent() override;

public:
    /** Default constructor. */
    UITextComponent();
    /** Copy constructor. */
    UITextComponent(const UITextComponent& src);

    UITextComponent* Clone() const override;

    /** Removed operator overloading. */
    UITextComponent& operator=(const UITextComponent&) = delete;

    /** Set text align style bit mask. \sa eAlign */
    void SetAlign(int32 align);
    /** Return text align style bit mask. \sa eAlign */
    int32 GetAlign() const;

    /** Set widget text. */
    void SetText(const String& text);
    /** Return widget text. */
    String GetText() const;

    /** Set content fitting style. */
    void SetFitting(eTextFitting fitting);
    /** Return content fitting style.*/
    eTextFitting GetFitting() const;

    /** Set font preset by preset name from FontManager. */
    void SetFontName(const String& fontName);
    /** Return font preset name. */
    String GetFontName() const;

    /** Set font path. */
    void SetFontPath(const FilePath& fontPath);
    /** Return font path. */
    const FilePath& GetFontPath() const;

    /** Set font. */
    void SetFont(const RefPtr<Font>& font);
    /** Get font. */
    Font* GetFont() const;

    /** Set font size. */
    void SetFontSize(float32 size);
    /** Return font size. */
    float32 GetFontSize() const;

    /** Set text color. */
    void SetColor(const Color& color);
    /** Return text color. */
    const Color& GetColor() const;

    /** Set multiline style. */
    void SetMultiline(eTextMultiline multiline);
    /** Return multiline style. */
    eTextMultiline GetMultiline() const;

    /** Set color inheritance mode. */
    void SetColorInheritType(UIControlBackground::eColorInheritType type);
    /** Return color inheritance mode. */
    UIControlBackground::eColorInheritType GetColorInheritType() const;

    /** Set text shadow offset. */
    void SetShadowOffset(const Vector2& offset);
    /** Return text shadow offset. */
    const Vector2& GetShadowOffset() const;

    /** Set text shadow color. */
    void SetShadowColor(const Color& color);
    /** Return text shadow color. */
    const Color& GetShadowColor() const;

    /** Set PerPixelAccuracy rendering mode.  */
    void SetPerPixelAccuracyType(UIControlBackground::ePerPixelAccuracyType type);
    /** Return PerPixelAccuracy rendering mode. */
    UIControlBackground::ePerPixelAccuracyType GetPerPixelAccuracyType() const;

    /** Set RTL align mode. */
    void SetUseRtlAlign(TextBlock::eUseRtlAlign useRtlAlign);
    /** Return RTL align mode. */
    TextBlock::eUseRtlAlign GetUseRtlAlign() const;

    /** Set force BiDi support mode. */
    void SetForceBiDiSupportEnabled(bool value);
    /** Return force BiDi support mode. */
    bool IsForceBiDiSupportEnabled() const;

    /** Set requestedTextRectSize parameter for internal TextBlock::SetText function usage. 
        Can be used for backward compatibility whith UIStaticText.
        \param value
            - if value.[x|y] size is 0 - text creates in the rect with size of the drawRect on draw phase
            - if value.[x|y] size is >0 - text creates in the rect with the requested size
            - if value.[x|y] size in <0 - rect creates for the all text size

        \sa {TextBlock, UIStaticText}
    */
    void SetRequestedTextRectSize(const Vector2& value);
    /** Set requestedTextRectSize parameter. */
    Vector2 GetRequestedTextRectSize() const;

    /** Set modification marker. */
    void SetModified(bool value);
    /** Check modification marker.*/
    bool IsModified() const;

    /** Return internal system helper object. */
    UITextSystemLink* GetLink() const;

protected:
    //TODO Delete in future.
    /** Set text offset. */
    void SetTextOffset(const Vector2& offset);
    /** Return text offset. */
    const Vector2& GetTextOffset() const;

private:
    int32 align = eAlign::ALIGN_HCENTER | eAlign::ALIGN_VCENTER;
    String text;
    eTextMultiline multiline = eTextMultiline::MULTILINE_DISABLED;
    eTextFitting fitting = eTextFitting::FITTING_NONE;
    Color color = Color::White;
    UIControlBackground::eColorInheritType colorInheritType = UIControlBackground::COLOR_MULTIPLY_ON_PARENT;
    Vector2 shadowOffset;
    Color shadowColor = Color::Black;
    UIControlBackground::ePerPixelAccuracyType perPixelAccuracyType = UIControlBackground::PER_PIXEL_ACCURACY_ENABLED;
    TextBlock::eUseRtlAlign useRtlAlign = TextBlock::eUseRtlAlign::RTL_DONT_USE;
    bool forceBiDiSupport = false;

    Vector2 textOffset;
    Vector2 requestedTextRectSize = Vector2::Zero;

    String fontPresetName;
    RefPtr<Font> font;
    float32 fontSize = 0.f;
    FilePath fontPath;

    bool modified = true;

    mutable UITextSystemLink link;

    friend class UIRenderSystem;
    friend class TextFieldStbImpl;
};
}
