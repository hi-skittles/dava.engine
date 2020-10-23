#include "UI/UITextField.h"
#include "Engine/Engine.h"
#include "FileSystem/FilePath.h"
#include "Input/InputSystem.h"
#include "Logger/Logger.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Render/2D/Font.h"
#include "Render/2D/FontManager.h"
#include "Render/2D/FontPreset.h"
#include "UI/UIControlSystem.h"
#include "UI/UITextFieldDelegate.h"
#include "UI/Update/UIUpdateComponent.h"
#include "Utils/UTF8Utils.h"

#ifdef __DAVAENGINE_AUTOTESTING__
#include "Autotesting/AutotestingSystem.h"
#endif

#if defined(__DAVAENGINE_ANDROID__)
#include "UI/Private/Android/TextFieldPlatformImpl.Android.h"
#elif defined(__DAVAENGINE_IPHONE__)
#include "UI/Private/Ios/TextFieldPlatformImpl.Ios.h"
#elif defined(__DAVAENGINE_WIN_UAP__) && !defined(DISABLE_NATIVE_TEXTFIELD)
#include "UI/Private/Win10/TextFieldPlatformImpl.Win10.h"
#else
#define DAVA_TEXTFIELD_USE_STB
#include "UI/UITextFieldStb.h"

namespace DAVA
{
class TextFieldPlatformImpl : public TextFieldStbImpl
{
public:
    TextFieldPlatformImpl(Window* w, UITextField* uiTextField)
        : TextFieldStbImpl(w, uiTextField)
    {
    }
};
}
#endif

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UITextField)
{
    ReflectionRegistrator<UITextField>::Begin()[M::DisplayName("Text Field")]
    .ConstructorByPointer()
    .DestructorByPointer([](UITextField* o) { o->Release(); })
    .Field("text", &UITextField::GetUtf8Text, &UITextField::SetUtf8Text)[M::DisplayName("Text"), M::Bindable()]
    .Field("font", &UITextField::GetFontPresetName, &UITextField::SetFontByPresetName)[M::DisplayName("Font Preset")]
    .Field("fontPath", &UITextField::GetFontPath, &UITextField::SetFontPath)[M::DisplayName("Font Path")]
    .Field("fontSize", &UITextField::GetFontSize, &UITextField::SetFontSize)[M::DisplayName("Font Size")]
    .Field("textcolor", &UITextField::GetTextColor, &UITextField::SetTextColor)[M::DisplayName("Text Color")] // TODO: camel style
    .Field("selectioncolor", &UITextField::GetSelectionColor, &UITextField::SetSelectionColor)[M::DisplayName("Selection Color")] // TODO: camel style
    .Field("shadowoffset", &UITextField::GetShadowOffset, &UITextField::SetShadowOffset)[M::DisplayName("Shadow Offset")] // TODO: camel style
    .Field("shadowcolor", &UITextField::GetShadowColor, &UITextField::SetShadowColor)[M::DisplayName("Shadow Color")] // TODO: camel style
    .Field("textalign", &UITextField::GetTextAlign, &UITextField::SetTextAlign)[M::FlagsT<eAlign>(), M::DisplayName("Text Align")] // TODO: camel style
    .Field("textUseRtlAlign", &UITextField::GetTextUseRtlAlign, &UITextField::SetTextUseRtlAlign)[M::EnumT<TextBlock::eUseRtlAlign>(), M::DisplayName("Use RTL")]
    .Field("maxLength", &UITextField::GetMaxLength, &UITextField::SetMaxLength)[M::DisplayName("Max Length")]
    .Field("isPassword", &UITextField::IsPassword, &UITextField::SetIsPassword)[M::DisplayName("Is Password")]
    .Field("isMultiline", &UITextField::IsMultiline, &UITextField::SetMultiline)[M::DisplayName("Multiline")]
    .Field("autoCapitalizationType", &UITextField::GetAutoCapitalizationType, &UITextField::SetAutoCapitalizationType)[M::EnumT<eAutoCapitalizationType>(), M::DisplayName("Auto Capitalization")]
    .Field("autoCorrectionType", &UITextField::GetAutoCorrectionType, &UITextField::SetAutoCorrectionType)[M::EnumT<eAutoCorrectionType>(), M::DisplayName("Auto Correction")]
    .Field("spellCheckingType", &UITextField::GetSpellCheckingType, &UITextField::SetSpellCheckingType)[M::EnumT<eSpellCheckingType>(), M::DisplayName("Spell Checking")]
    .Field("keyboardAppearanceType", &UITextField::GetKeyboardAppearanceType, &UITextField::SetKeyboardAppearanceType)[M::EnumT<eKeyboardAppearanceType>(), M::DisplayName("Keyboard Appearance")]
    .Field("keyboardType", &UITextField::GetKeyboardType, &UITextField::SetKeyboardType)[M::EnumT<eKeyboardType>(), M::DisplayName("Keyboard")]
    .Field("returnKeyType", &UITextField::GetReturnKeyType, &UITextField::SetReturnKeyType)[M::EnumT<eReturnKeyType>(), M::DisplayName("Return Key")]
    .Field("enableReturnKeyAutomatically", &UITextField::IsEnableReturnKeyAutomatically, &UITextField::SetEnableReturnKeyAutomatically)[M::DisplayName("Enable Auto Return Key")]
    .Field("startEditPolicy", &UITextField::GetStartEditPolicy, &UITextField::SetStartEditPolicy)[M::EnumT<eStartEditPolicy>(), M::DisplayName("Start Editing")]
    .Field("stopEditPolicy", &UITextField::GetStopEditPolicy, &UITextField::SetStopEditPolicy)[M::EnumT<eStopEditPolicy>(), M::DisplayName("Stop Editing")]
    .End();
}

UITextField::UITextField(const Rect& rect)
    : UIControl(rect)
    , textFieldImpl(std::make_shared<TextFieldPlatformImpl>(Engine::Instance()->PrimaryWindow(), this))
{
    // Additional step to do impl initialization which cannot be done in impl constructor, e.g.
    // call shared_from_this() to create std::weak_ptr from std::shared_ptr
    textFieldImpl->Initialize();

    textFieldImpl->SetVisible(false);

    SetupDefaults();
    GetOrCreateComponent<UIUpdateComponent>();
}

void UITextField::SetupDefaults()
{
    SetInputEnabled(true, false);

    SetAutoCapitalizationType(AUTO_CAPITALIZATION_TYPE_SENTENCES);
    SetAutoCorrectionType(AUTO_CORRECTION_TYPE_DEFAULT);
    SetSpellCheckingType(SPELL_CHECKING_TYPE_DEFAULT);
    SetKeyboardAppearanceType(KEYBOARD_APPEARANCE_DEFAULT);
    SetKeyboardType(KEYBOARD_TYPE_DEFAULT);
    SetReturnKeyType(RETURN_KEY_DEFAULT);
    SetEnableReturnKeyAutomatically(false);
    SetTextUseRtlAlign(TextBlock::RTL_DONT_USE);

    SetMaxLength(-1);

    SetIsPassword(false);
    SetTextColor(GetTextColor());
    SetTextAlign(ALIGN_LEFT | ALIGN_VCENTER);

    SetText(L"");
    SetRenderToTexture(true);
}

UITextField::~UITextField()
{
    // Tell impl that owner is dying to prevent calling delegate's methods
    textFieldImpl->OwnerIsDying();

    UIControl::RemoveAllControls();
}

bool UITextField::IsEditing() const
{
    return isEditing;
}

void UITextField::StartEdit()
{
    if (!isEditing)
    {
        isEditing = true;
        OnStartEditing();
        SetRenderToTexture(false);
        textFieldImpl->OpenKeyboard();
    }
}

void UITextField::StopEdit()
{
    if (isEditing)
    {
        isEditing = false;
        SetRenderToTexture(true);
        textFieldImpl->CloseKeyboard();
        OnStopEditing();
#ifdef __DAVAENGINE_AUTOTESTING__
        AutotestingSystem::Instance()->OnRecordSetText(this, GetUtf8Text());
#endif
    }
}

void UITextField::Update(float32 timeElapsed)
{
    textFieldImpl->UpdateRect(GetGeometricData().GetUnrotatedRect());
}

void UITextField::OnActive()
{
#if defined(__DAVAENGINE_IPHONE__)
    textFieldImpl->ShowField();
    textFieldImpl->SetVisible(IsVisible());
#endif
}

void UITextField::OnInactive()
{
#if defined(__DAVAENGINE_IPHONE__)
    textFieldImpl->HideField();
#endif
}

void UITextField::OnFocused()
{
    if (startEditPolicy == START_EDIT_WHEN_FOCUSED)
    {
        StartEdit();
    }
}

void UITextField::SetFocused()
{
    GetEngineContext()->uiControlSystem->SetFocusedControl(this);
}

void UITextField::OnFocusLost()
{
    StopEdit();

    if (delegate != nullptr)
    {
        delegate->TextFieldLostFocus(this);
    }
}

void UITextField::OnTouchOutsideFocus()
{
    if (stopEditPolicy == STOP_EDIT_BY_USER_REQUEST)
    {
        StopEdit();
    }
}

void UITextField::SetSelectionColor(const Color& selectionColor)
{
#if defined(DAVA_TEXTFIELD_USE_STB)
    textFieldImpl->SetSelectionColor(selectionColor);
#endif
}

const Color& UITextField::GetSelectionColor() const
{
#if defined(DAVA_TEXTFIELD_USE_STB)
    return textFieldImpl->GetSelectionColor();
#else
    return Color::Transparent;
#endif
}

void UITextField::ReleaseFocus()
{
    StopEdit();
}

void UITextField::SetFont(Font* font)
{
#if defined(DAVA_TEXTFIELD_USE_STB)
    textFieldImpl->SetFont(font);
#endif // !defined(DAVA_TEXTFIELD_USE_STB)
}

void UITextField::SetFontPath(const FilePath& fontPath)
{
#if defined(DAVA_TEXTFIELD_USE_STB)
    textFieldImpl->SetFontPath(fontPath);
#endif // !defined(DAVA_TEXTFIELD_USE_STB)
}

const FilePath& UITextField::GetFontPath() const
{
#if defined(DAVA_TEXTFIELD_USE_STB)
    return textFieldImpl->GetFontPath();
#else
    static const FilePath EMPTY_PATH;
    return EMPTY_PATH;
#endif // !defined(DAVA_TEXTFIELD_USE_STB)
}

void UITextField::SetTextColor(const Color& fontColor)
{
    textFieldImpl->SetTextColor(fontColor);
}

void UITextField::SetShadowOffset(const DAVA::Vector2& offset)
{
#if defined(DAVA_TEXTFIELD_USE_STB)
    textFieldImpl->SetShadowOffset(offset);
#endif
}

void UITextField::SetShadowColor(const Color& color)
{
#if defined(DAVA_TEXTFIELD_USE_STB)
    textFieldImpl->SetShadowColor(color);
#endif
}

void UITextField::SetTextAlign(int32 align)
{
    textFieldImpl->SetTextAlign(align);
}

TextBlock::eUseRtlAlign UITextField::GetTextUseRtlAlign() const
{
#ifdef DAVA_TEXTFIELD_USE_STB
    return textFieldImpl->GetTextUseRtlAlign();
#else
    return textFieldImpl->GetTextUseRtlAlign() ? TextBlock::RTL_USE_BY_CONTENT : TextBlock::RTL_DONT_USE;
#endif
}

void UITextField::SetTextUseRtlAlign(TextBlock::eUseRtlAlign useRtlAlign)
{
#ifdef DAVA_TEXTFIELD_USE_STB
    textFieldImpl->SetTextUseRtlAlign(useRtlAlign);
#else
    textFieldImpl->SetTextUseRtlAlign(useRtlAlign == TextBlock::RTL_USE_BY_CONTENT);
#endif
}

void UITextField::SetFontSize(float32 size)
{
    textFieldImpl->SetFontSize(size);
    fontSize = size;
}

void UITextField::SetDelegate(UITextFieldDelegate* _delegate)
{
    delegate = _delegate;
#if !defined(DISABLE_NATIVE_TEXTFIELD)
    textFieldImpl->SetDelegate(_delegate);
#endif
}

UITextFieldDelegate* UITextField::GetDelegate()
{
    return delegate;
}

void UITextField::SetSize(const DAVA::Vector2& newSize)
{
    UIControl::SetSize(newSize);
#if defined(DAVA_TEXTFIELD_USE_STB)
    textFieldImpl->SetSize(newSize);
#endif
}

void UITextField::SetPosition(const DAVA::Vector2& position)
{
    UIControl::SetPosition(position);
}

void UITextField::SetMultiline(bool value)
{
    if (value != isMultiline)
    {
        isMultiline = value;
        textFieldImpl->SetMultiline(isMultiline);
    }
}

bool UITextField::IsMultiline() const
{
    return isMultiline;
}

void UITextField::SetText(const WideString& text_)
{
    textFieldImpl->SetText(text_);
    text = text_;
}

String UITextField::GetUtf8Text()
{
    return UTF8Utils::EncodeToUTF8(GetText());
}

void UITextField::SetUtf8Text(const String& utf8String)
{
    SetText(UTF8Utils::EncodeToWideString(utf8String));
}

const WideString& UITextField::GetText()
{
    textFieldImpl->GetText(text);
    return text;
}

Font* UITextField::GetFont() const
{
#if defined(DAVA_TEXTFIELD_USE_STB)
    return textFieldImpl->GetFont();
#else
    return nullptr;
#endif
}

float32 UITextField::GetFontSize() const
{
#if defined(DAVA_TEXTFIELD_USE_STB)
    return textFieldImpl->GetFontSize();
#else
    return fontSize;
#endif
}

Color UITextField::GetTextColor() const
{
#if defined(DAVA_TEXTFIELD_USE_STB)
    return textFieldImpl->GetTextColor();
#else
    return Color::White;
#endif
}

Vector2 UITextField::GetShadowOffset() const
{
#if defined(DAVA_TEXTFIELD_USE_STB)
    return textFieldImpl->GetShadowOffset();
#else
    return Vector2::Zero;
#endif
}

Color UITextField::GetShadowColor() const
{
#if defined(DAVA_TEXTFIELD_USE_STB)
    return textFieldImpl->GetShadowColor();
#else
    return Color::White;
#endif
}

int32 UITextField::GetTextAlign() const
{
    return textFieldImpl->GetTextAlign();
}

void UITextField::Input(UIEvent* currentInput)
{
#if defined(DAVA_TEXTFIELD_USE_STB)
    textFieldImpl->Input(currentInput);

#else
    if (this != GetEngineContext()->uiControlSystem->GetFocusedControl())
        return;

    if (currentInput->phase == UIEvent::Phase::ENDED)
    {
        if (startEditPolicy == START_EDIT_BY_USER_REQUEST)
        {
            StartEdit();
        }
    }
#endif
}

WideString UITextField::GetAppliedChanges(int32 replacementLocation, int32 replacementLength, const WideString& replacementString)
{ //TODO: fix this for copy/paste
    WideString txt = GetText();

    if (replacementLocation >= 0)
    {
        if (replacementLocation <= static_cast<int32>(txt.length()))
        {
            txt.replace(replacementLocation, replacementLength, replacementString);
            if (GetMaxLength() > 0)
            {
                int32 outOfBounds = static_cast<int32>(txt.size()) - GetMaxLength();
                if (outOfBounds > 0)
                {
                    txt.erase(GetMaxLength(), outOfBounds);
                }
            }
        }
        else
        {
            Logger::Error("[UITextField::GetAppliedChanges] - index out of bounds.");
        }
    }

    return txt;
}

UITextField* UITextField::Clone()
{
    UITextField* t = new UITextField();
    t->CopyDataFrom(this);
    return t;
}

void UITextField::CopyDataFrom(UIControl* srcControl)
{
    UIControl::CopyDataFrom(srcControl);
    UITextField* t = static_cast<UITextField*>(srcControl);

#if defined(DAVA_TEXTFIELD_USE_STB)
    textFieldImpl->CopyDataFrom(t->textFieldImpl.get());
#endif

    cursorBlinkingTime = t->cursorBlinkingTime;
    SetText(t->GetText());
    SetRect(t->GetRect());
    SetFontByPresetName(t->GetFontPresetName());
    SetFontPath(t->GetFontPath());
    SetFontSize(t->GetFontSize());
    SetAutoCapitalizationType(t->GetAutoCapitalizationType());
    SetAutoCorrectionType(t->GetAutoCorrectionType());
    SetSpellCheckingType(t->GetSpellCheckingType());
    SetKeyboardAppearanceType(t->GetKeyboardAppearanceType());
    SetKeyboardType(t->GetKeyboardType());
    SetReturnKeyType(t->GetReturnKeyType());
    SetEnableReturnKeyAutomatically(t->IsEnableReturnKeyAutomatically());
    SetTextUseRtlAlign(t->GetTextUseRtlAlign());
    SetMaxLength(t->GetMaxLength());
    SetIsPassword(t->IsPassword());
    SetTextColor(t->GetTextColor());
    SetTextAlign(t->GetTextAlign());
    SetRenderToTexture(t->IsRenderToTexture());
}

void UITextField::SetIsPassword(bool isPassword_)
{
    isPassword = isPassword_;
    textFieldImpl->SetIsPassword(isPassword_);
}

bool UITextField::IsPassword() const
{
    return isPassword;
}

WideString UITextField::GetVisibleText()
{
    if (!isPassword)
    {
        return GetText();
    }
    return WideString(GetText().length(), L'*');
}

int32 UITextField::GetAutoCapitalizationType() const
{
    return autoCapitalizationType;
}

void UITextField::SetAutoCapitalizationType(int32 value)
{
    autoCapitalizationType = static_cast<eAutoCapitalizationType>(value);
    textFieldImpl->SetAutoCapitalizationType(value);
}

int32 UITextField::GetAutoCorrectionType() const
{
    return autoCorrectionType;
}

void UITextField::SetAutoCorrectionType(int32 value)
{
    autoCorrectionType = static_cast<eAutoCorrectionType>(value);
    textFieldImpl->SetAutoCorrectionType(value);
}

int32 UITextField::GetSpellCheckingType() const
{
    return spellCheckingType;
}

void UITextField::SetSpellCheckingType(int32 value)
{
    spellCheckingType = static_cast<eSpellCheckingType>(value);
    textFieldImpl->SetSpellCheckingType(value);
}

int32 UITextField::GetKeyboardAppearanceType() const
{
    return keyboardAppearanceType;
}

void UITextField::SetKeyboardAppearanceType(int32 value)
{
    keyboardAppearanceType = static_cast<eKeyboardAppearanceType>(value);
    textFieldImpl->SetKeyboardAppearanceType(value);
}

int32 UITextField::GetKeyboardType() const
{
    return keyboardType;
}

void UITextField::SetKeyboardType(int32 value)
{
    keyboardType = static_cast<eKeyboardType>(value);
    textFieldImpl->SetKeyboardType(value);
}

int32 UITextField::GetReturnKeyType() const
{
    return returnKeyType;
}

void UITextField::SetReturnKeyType(int32 value)
{
    returnKeyType = static_cast<eReturnKeyType>(value);
    textFieldImpl->SetReturnKeyType(value);
}

UITextField::eStartEditPolicy UITextField::GetStartEditPolicy() const
{
    return startEditPolicy;
}

void UITextField::SetStartEditPolicy(eStartEditPolicy policy)
{
    startEditPolicy = policy;
}

UITextField::eStopEditPolicy UITextField::GetStopEditPolicy() const
{
    return stopEditPolicy;
}

void UITextField::SetStopEditPolicy(eStopEditPolicy policy)
{
    stopEditPolicy = policy;
}

bool UITextField::IsEnableReturnKeyAutomatically() const
{
    return enableReturnKeyAutomatically;
}

void UITextField::SetEnableReturnKeyAutomatically(bool value)
{
    enableReturnKeyAutomatically = value;
    textFieldImpl->SetEnableReturnKeyAutomatically(value);
}

void UITextField::SetInputEnabled(bool isEnabled, bool hierarchic)
{
    UIControl::SetInputEnabled(isEnabled, hierarchic);
    textFieldImpl->SetInputEnabled(isEnabled);
}

void UITextField::SetRenderToTexture(bool value)
{
    // Workaround! Users need scrolling of large texts in
    // multiline mode so we have to disable render into texture
    if (isMultiline)
    {
        value = false;
    }

    textFieldImpl->SetRenderToTexture(value);
}

bool UITextField::IsRenderToTexture() const
{
    return textFieldImpl->IsRenderToTexture();
}

uint32 UITextField::GetCursorPos()
{
    return textFieldImpl->GetCursorPos();
}

void UITextField::SetCursorPos(uint32 pos)
{
    textFieldImpl->SetCursorPos(pos);
}

void UITextField::SetMaxLength(int32 newMaxLength)
{
    maxLength = Max(-1, newMaxLength); //-1 valid value
    textFieldImpl->SetMaxLength(maxLength);
}

int32 UITextField::GetMaxLength() const
{
    return maxLength;
}

void UITextField::OnStartEditing()
{
    if (delegate != nullptr)
    {
        delegate->OnStartEditing(this);
    }
}

void UITextField::OnStopEditing()
{
    if (delegate != nullptr)
    {
        delegate->OnStopEditing(this);
    }
}

void UITextField::OnKeyboardShown(const Rect& keyboardRect)
{
    if (delegate != nullptr)
    {
        delegate->OnKeyboardShown(keyboardRect);
    }
}

void UITextField::OnKeyboardHidden()
{
    if (delegate != nullptr)
    {
        delegate->OnKeyboardHidden();
    }
}

void UITextField::OnVisible()
{
    UIControl::OnVisible();
    textFieldImpl->SetVisible(visible);
}

void UITextField::OnInvisible()
{
    UIControl::OnInvisible();
    textFieldImpl->SetVisible(false);
}

const String& UITextField::GetFontPresetName() const
{
    return fontPresetName;
}

void UITextField::SetFontByPresetName(const String& presetName)
{
    if (fontPresetName != presetName)
    {
#if defined(DAVA_TEXTFIELD_USE_STB)
        textFieldImpl->SetFontName(presetName);

#else // for native text fields
        FontPreset preset;
        if (!presetName.empty())
        {
            preset = GetEngineContext()->fontManager->GetFontPreset(presetName);
        }

        if (fontSize > 0.f) // Font size has high priority that preset size
        {
            textFieldImpl->SetFontSize(fontSize);
        }
        else
        {
            textFieldImpl->SetFontSize(preset.GetSize());
        }
#endif

        fontPresetName = presetName;
    }
}

void UITextField::Draw(const UIGeometricData& geometricData)
{
    UIControl::Draw(geometricData);

    textFieldImpl->SystemDraw(geometricData);
}

int32 UITextField::GetStartEditPolicyAsInt() const
{
    return GetStartEditPolicy();
}

void UITextField::SetStartEditPolicyFromInt(int32 policy)
{
    SetStartEditPolicy(static_cast<eStartEditPolicy>(policy));
}

int32 UITextField::GetStopEditPolicyAsInt() const
{
    return GetStopEditPolicy();
}

void UITextField::SetStopEditPolicyFromInt(int32 policy)
{
    SetStopEditPolicy(static_cast<eStopEditPolicy>(policy));
}

} // namespace DAVA
