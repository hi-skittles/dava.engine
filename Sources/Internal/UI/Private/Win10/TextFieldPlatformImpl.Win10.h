#pragma once

#include "Base/Platform.h"

#if defined(__DAVAENGINE_WIN_UAP__)

#include "Base/BaseTypes.h"
#include "Math/Color.h"
#include "Math/Rect.h"

namespace DAVA
{
class Color;
class Sprite;
class Texture;
class UIGeometricData;
class UITextField;
class UITextFieldDelegate;
class Window;

class TextFieldPlatformImpl : public std::enable_shared_from_this<TextFieldPlatformImpl>
{
    struct TextFieldProperties
    {
        void ClearChangedFlags();

        bool createNew : 1;
        bool focus : 1;
        bool focusChanged : 1;

        Rect rect;
        Rect rectInWindowSpace;
        WideString text;
        Color textColor;
        bool visible = false;
        bool password = false;
        bool multiline = false;
        bool inputEnabled = false;
        bool spellCheckingEnabled = false;
        bool textRtlAlignment = false;
        int32 textAlignment = 0;
        int32 maxTextLength = 0;
        int32 keyboardType = 0;
        int32 caretPosition = 0;
        float32 fontSize = 0.0f;
        float32 virtualFontSize = 0.0f;

        bool anyPropertyChanged : 1;
        bool rectChanged : 1;
        bool textChanged : 1;
        bool textColorChanged : 1;
        bool visibleChanged : 1;
        bool passwordChanged : 1;
        bool multilineChanged : 1;
        bool inputEnabledChanged : 1;
        bool spellCheckingEnabledChanged : 1;
        bool textRtlAlignmentChanged : 1;
        bool textAlignmentChanged : 1;
        bool maxTextLengthChanged : 1;
        bool keyboardTypeChanged : 1;
        bool caretPositionChanged : 1;
        bool fontSizeChanged : 1;

        bool rectAssigned : 1;
        bool textAssigned : 1;
        bool textColorAssigned : 1;
        bool visibleAssigned : 1;
        bool passwordAssigned : 1;
        bool multilineAssigned : 1;
        bool inputEnabledAssigned : 1;
        bool spellCheckingEnabledAssigned : 1;
        bool textRtlAlignmentAssigned : 1;
        bool textAlignmentAssigned : 1;
        bool maxTextLengthAssigned : 1;
        bool keyboardTypeAssigned : 1;
        bool caretPositionAssigned : 1;
        bool fontSizeAssigned : 1;
    };

public:
    TextFieldPlatformImpl(Window* w, UITextField* uiTextField);
    ~TextFieldPlatformImpl();

    void Initialize();
    void OwnerIsDying();

    void SetVisible(bool isVisible);
    void SetIsPassword(bool isPassword);
    void SetMaxLength(int32 value);

    void OpenKeyboard();
    void CloseKeyboard();

    void UpdateRect(const Rect& rect);

    void SetRect(const Rect& rect);

    void SetText(const WideString& text);
    void GetText(WideString& text) const;

    void SetTextColor(const Color& color);
    void SetTextAlign(int32 align);
    int32 GetTextAlign() const;
    void SetTextUseRtlAlign(bool useRtlAlign);
    bool GetTextUseRtlAlign() const;

    void SetFontSize(float32 virtualFontSize);

    void SetDelegate(UITextFieldDelegate* textFieldDelegate);

    void SetMultiline(bool enable);

    void SetInputEnabled(bool enable);

    void SetRenderToTexture(bool value);
    bool IsRenderToTexture() const;

    void SetAutoCapitalizationType(int32 value);
    void SetAutoCorrectionType(int32 value);
    void SetSpellCheckingType(int32 value);
    void SetKeyboardAppearanceType(int32 value);
    void SetKeyboardType(int32 value);
    void SetReturnKeyType(int32 value);
    void SetEnableReturnKeyAutomatically(bool value);

    uint32 GetCursorPos() const;
    void SetCursorPos(uint32 pos);

    void SystemDraw(const UIGeometricData&);

private:
    void CreateNativeControl(bool textControl);
    void DeleteNativeControl();
    void InstallCommonEventHandlers();
    void InstallTextEventHandlers();
    void InstallPasswordEventHandlers();
    void InstallKeyboardEventHandlers();

    void ProcessProperties(const TextFieldProperties& props);
    void ApplyChangedProperties(const TextFieldProperties& props);
    void ApplyAssignedProperties(const TextFieldProperties& props);

    void SetNativePositionAndSize(const Rect& rect, bool offScreen);
    void SetNativeVisible(bool visible);
    void SetNativeMultiline(bool multiline);
    void SetNativeInputEnabled(bool enabled);
    void SetNativeText(const WideString& text);
    void SetNativeMaxTextLength(int32 maxLength);
    void SetNativeCaretPosition(int32 caretPosition);
    void SetNativeFontSize(float32 fontSize);
    void SetNativeTextColor(const Color& textColor);
    void SetNativeTextAlignment(int32 textAlignment, bool textRtlAlignment);
    void SetNativeKeyboardType(int32 type);
    void SetNativeSpellChecking(bool enabled);

    bool HasFocus() const;
    Platform::String ^ GetNativeText() const;
    int32 GetNativeCaretPosition() const;

    bool IsPassword() const;
    bool IsMultiline() const;

    Rect VirtualToWindow(const Rect& srcRect) const;
    Rect WindowToVirtual(const Rect& srcRect) const;
    void RenderToTexture(bool moveOffScreenOnCompletion);
    Sprite* CreateSpriteFromPreviewData(uint8* imageData, uint32 width, uint32 height);

private: // Event handlers
    void OnKeyDown(Windows::UI::Xaml::Input::KeyRoutedEventArgs ^ args);
    void OnGotFocus();
    void OnLostFocus();
    void OnTextChanging();
    void OnTextChanged();
    void OnLayoutUpdated();

    // TextBox specific events
    void OnSelectionChanged();

    // Onscreen keyboard events
    void OnKeyboardShowing(Windows::UI::ViewManagement::InputPaneVisibilityEventArgs ^ args);

    // Signal handlers
    void OnWindowSizeChanged(Window* w, Size2f windowSize, Size2f surfaceSize);
    void OnWindowDestroyed(Window* w);

private:
    Window* window = nullptr;
    UITextField* uiTextField = nullptr;
    UITextFieldDelegate* textFieldDelegate = nullptr;
    // Windows UAP has two different controls for text input and password input
    // So we should switch internal implementation depending on user's wishes
    Windows::UI::Xaml::Controls::TextBox ^ nativeText = nullptr;
    Windows::UI::Xaml::Controls::PasswordBox ^ nativePassword = nullptr;
    Windows::UI::Xaml::Controls::Control ^ nativeControl = nullptr; // Points either to nativeText or nativePassword
    Windows::UI::Xaml::Controls::Border ^ nativeControlHolder = nullptr;

    // Tokens to unsubscribe from touch keyboard event handlers
    Windows::Foundation::EventRegistrationToken tokenKeyboardShowing;

    bool ignoreTextChange = false;
    bool waitRenderToTextureComplete = false; // If flag is set do not move native control offscreen to get rid of some flickering

    int32 caretPosition = 0; // Current caret position
    int32 savedCaretPosition = 0; // Saved caret position to restore it when delegate declines text changing

    Rect rectInWindowSpace;

    WideString curText;
    WideString lastProgrammaticText;
    TextFieldProperties properties;
    bool programmaticTextChange = false;

    Texture* texture = nullptr;
    Sprite* sprite = nullptr;

    static Windows::UI::Xaml::Style ^ customTextBoxStyle;
    static Windows::UI::Xaml::Style ^ customPasswordBoxStyle;
    static Platform::String ^ xamlTextBoxStyles;
};

//////////////////////////////////////////////////////////////////////////
inline int32 TextFieldPlatformImpl::GetTextAlign() const
{
    return properties.textAlignment;
}

inline bool TextFieldPlatformImpl::GetTextUseRtlAlign() const
{
    return properties.textRtlAlignment;
}

inline void TextFieldPlatformImpl::SetRenderToTexture(bool /*value*/)
{
    // Do nothing as single line text field always is painted into texture
    // Multiline text field is never rendered to texture
}

inline bool TextFieldPlatformImpl::IsRenderToTexture() const
{
    return !properties.multiline;
}

inline uint32 TextFieldPlatformImpl::GetCursorPos() const
{
    return caretPosition;
}

inline void TextFieldPlatformImpl::SystemDraw(const UIGeometricData&)
{
}

} // namespace DAVA

#endif // __DAVAENGINE_WIN_UAP__
