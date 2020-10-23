#include "UI/Text/UITextComponent.h"
#include "Base/GlobalEnum.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"
#include "Reflection/ReflectionRegistrator.h"
#include "UITextSystemLink.h"

ENUM_DECLARE(DAVA::UITextComponent::eTextMultiline)
{
    ENUM_ADD_DESCR(DAVA::UITextComponent::MULTILINE_DISABLED, "MULTILINE_DISABLED");
    ENUM_ADD_DESCR(DAVA::UITextComponent::MULTILINE_ENABLED, "MULTILINE_ENABLED");
    ENUM_ADD_DESCR(DAVA::UITextComponent::MULTILINE_ENABLED_BY_SYMBOL, "MULTILINE_ENABLED_BY_SYMBOL");
};

ENUM_DECLARE(DAVA::UITextComponent::eTextFitting)
{
    ENUM_ADD_DESCR(DAVA::UITextComponent::FITTING_NONE, "FITTING_NONE");
    ENUM_ADD_DESCR(DAVA::UITextComponent::FITTING_ENLARGE, "FITTING_ENLARGE");
    ENUM_ADD_DESCR(DAVA::UITextComponent::FITTING_REDUCE, "FITTING_REDUCE");
    ENUM_ADD_DESCR(DAVA::UITextComponent::FITTING_FILL, "FITTING_FILL");
    ENUM_ADD_DESCR(DAVA::UITextComponent::FITTING_POINTS, "FITTING_POINTS");
};

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UITextComponent)
{
    ReflectionRegistrator<UITextComponent>::Begin()[M::DisplayName("Text"), M::Group("Content")]
    .ConstructorByPointer()
    .DestructorByPointer([](UITextComponent* o) { o->Release(); })
    .Field("text", &UITextComponent::GetText, &UITextComponent::SetText)[M::DisplayName("Text"), M::Bindable()]
    .Field("fontName", &UITextComponent::GetFontName, &UITextComponent::SetFontName)[M::DisplayName("Font Preset")]
    .Field("fontPath", &UITextComponent::GetFontPath, &UITextComponent::SetFontPath)[M::DisplayName("Font Path")]
    .Field("fontSize", &UITextComponent::GetFontSize, &UITextComponent::SetFontSize)[M::DisplayName("Font Size")]
    .Field("color", &UITextComponent::GetColor, &UITextComponent::SetColor)[M::DisplayName("Color")]
    .Field("colorInheritType", &UITextComponent::GetColorInheritType, &UITextComponent::SetColorInheritType)[M::EnumT<UIControlBackground::eColorInheritType>(), M::DisplayName("Color Inherit")]
    .Field("perPixelAccuracyType", &UITextComponent::GetPerPixelAccuracyType, &UITextComponent::SetPerPixelAccuracyType)[M::EnumT<UIControlBackground::ePerPixelAccuracyType>(), M::DisplayName("PPA")]
    .Field("shadowOffset", &UITextComponent::GetShadowOffset, &UITextComponent::SetShadowOffset)[M::DisplayName("Shadow Offset")]
    .Field("shadowColor", &UITextComponent::GetShadowColor, &UITextComponent::SetShadowColor)[M::DisplayName("Shadow Color")]
    .Field("multiline", &UITextComponent::GetMultiline, &UITextComponent::SetMultiline)[M::EnumT<eTextMultiline>(), M::DisplayName("Multiline")]
    .Field("fitting", &UITextComponent::GetFitting, &UITextComponent::SetFitting)[M::EnumT<eTextFitting>(), M::DisplayName("Fitting")]
    .Field("align", &UITextComponent::GetAlign, &UITextComponent::SetAlign)[M::FlagsT<eAlign>(), M::DisplayName("Align")]
    .Field("useRtlAlign", &UITextComponent::GetUseRtlAlign, &UITextComponent::SetUseRtlAlign)[M::EnumT<TextBlock::eUseRtlAlign>(), M::DisplayName("Use RTL")]
    .Field("forceBiDiSupport", &UITextComponent::IsForceBiDiSupportEnabled, &UITextComponent::SetForceBiDiSupportEnabled)[M::DisplayName("Force BiDi")]
    .End();
}

IMPLEMENT_UI_COMPONENT(UITextComponent);

UITextComponent::UITextComponent(const UITextComponent& src)
    : UIComponent(src)
    , align(src.align)
    , text(src.text)
    , fontPresetName(src.fontPresetName)
    , multiline(src.multiline)
    , fitting(src.fitting)
    , color(src.color)
    , colorInheritType(src.colorInheritType)
    , shadowOffset(src.shadowOffset)
    , shadowColor(src.shadowColor)
    , perPixelAccuracyType(src.perPixelAccuracyType)
    , useRtlAlign(src.useRtlAlign)
    , forceBiDiSupport(src.forceBiDiSupport)
    , requestedTextRectSize(src.requestedTextRectSize)
    , font(src.font)
    , fontSize(src.fontSize)
    , modified(true)
{
}

UITextComponent::UITextComponent()
{
}

UITextComponent::~UITextComponent()
{
    font = nullptr;
}

UITextComponent* UITextComponent::Clone() const
{
    return new UITextComponent(*this);
}

void UITextComponent::SetAlign(int32 value)
{
    if (align != value)
    {
        align = value;
        modified = true;
    }
}

int32 UITextComponent::GetAlign() const
{
    return align;
}

void UITextComponent::SetText(const String& value)
{
    if (text != value)
    {
        text = value;
        modified = true;
    }
}

String UITextComponent::GetText() const
{
    return text;
}

void UITextComponent::SetFitting(eTextFitting value)
{
    if (fitting != value)
    {
        fitting = value;
        modified = true;
    }
}

UITextComponent::eTextFitting UITextComponent::GetFitting() const
{
    return fitting;
}

void UITextComponent::SetFontName(const String& value)
{
    if (fontPresetName != value)
    {
        fontPresetName = value;
        font = nullptr;
        modified = true;
    }
}

String UITextComponent::GetFontName() const
{
    return fontPresetName;
}

void UITextComponent::SetFontPath(const FilePath& fontPath_)
{
    if (fontPath != fontPath_)
    {
        fontPath = fontPath_;
        font = nullptr;
        modified = true;
    }
}

const FilePath& UITextComponent::GetFontPath() const
{
    return fontPath;
}

void UITextComponent::SetFont(const RefPtr<Font>& font_)
{
    if (font != font_)
    {
        font = font_;
        fontPresetName = "";
        fontPath = "";
        modified = true;
    }
}

Font* UITextComponent::GetFont() const
{
    return font.Get();
}

void UITextComponent::SetFontSize(float32 size)
{
    if (!FLOAT_EQUAL(fontSize, size))
    {
        fontSize = size;
        modified = true;
    }
}

float32 UITextComponent::GetFontSize() const
{
    return fontSize;
}

void UITextComponent::SetColor(const Color& value)
{
    if (color != value)
    {
        color = value;
        modified = true;
    }
}

const Color& UITextComponent::GetColor() const
{
    return color;
}

void UITextComponent::SetMultiline(eTextMultiline value)
{
    if (multiline != value)
    {
        multiline = value;
        modified = true;
    }
}

UITextComponent::eTextMultiline UITextComponent::GetMultiline() const
{
    return multiline;
}

void UITextComponent::SetColorInheritType(UIControlBackground::eColorInheritType value)
{
    if (colorInheritType != value)
    {
        colorInheritType = value;
        modified = true;
    }
}

UIControlBackground::eColorInheritType UITextComponent::GetColorInheritType() const
{
    return colorInheritType;
}

void UITextComponent::SetShadowOffset(const Vector2& value)
{
    if (shadowOffset != value)
    {
        shadowOffset = value;
    }
}

const Vector2& UITextComponent::GetShadowOffset() const
{
    return shadowOffset;
}

void UITextComponent::SetShadowColor(const Color& value)
{
    if (shadowColor != value)
    {
        shadowColor = value;
        modified = true;
    }
}

const Color& UITextComponent::GetShadowColor() const
{
    return shadowColor;
}

void UITextComponent::SetPerPixelAccuracyType(UIControlBackground::ePerPixelAccuracyType value)
{
    if (perPixelAccuracyType != value)
    {
        perPixelAccuracyType = value;
        modified = true;
    }
}

UIControlBackground::ePerPixelAccuracyType UITextComponent::GetPerPixelAccuracyType() const
{
    return perPixelAccuracyType;
}

void UITextComponent::SetUseRtlAlign(TextBlock::eUseRtlAlign value)
{
    if (useRtlAlign != value)
    {
        useRtlAlign = value;
        modified = true;
    }
}

TextBlock::eUseRtlAlign UITextComponent::GetUseRtlAlign() const
{
    return useRtlAlign;
}

void UITextComponent::SetForceBiDiSupportEnabled(bool value)
{
    if (forceBiDiSupport != value)
    {
        forceBiDiSupport = value;
        modified = true;
    }
}

void UITextComponent::SetRequestedTextRectSize(const Vector2& value)
{
    if (requestedTextRectSize != value)
    {
        requestedTextRectSize = value;
        modified = true;
    }
}

DAVA::Vector2 UITextComponent::GetRequestedTextRectSize() const
{
    return requestedTextRectSize;
}

bool UITextComponent::IsForceBiDiSupportEnabled() const
{
    return forceBiDiSupport;
}

void UITextComponent::SetTextOffset(const Vector2& value)
{
    if (textOffset != value)
    {
        textOffset = value;
    }
}

const Vector2& UITextComponent::GetTextOffset() const
{
    return textOffset;
}

void UITextComponent::SetModified(bool value)
{
    modified = value;
}

bool UITextComponent::IsModified() const
{
    return modified;
}

UITextSystemLink* UITextComponent::GetLink() const
{
    return &link;
}
};
