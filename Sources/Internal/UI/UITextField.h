#pragma once

#include "UI/UIControl.h"
#include "Render/2D/TextBlock.h"

namespace DAVA
{
class FilePath;
class TextFieldPlatformImpl;
class UITextFieldDelegate;

/**
    \brief  A UITextField object is a control that displays editable text and sends an action message to a target object when the user presses the return button. 
            You typically use this class to gather small amounts of text from the user and perform some immediate action, such as a search operation, based on that text.
            A text field object supports the use of a delegate object to handle editing-related notifications. 
 */
class UITextField : public UIControl
{
    DAVA_VIRTUAL_REFLECTION(UITextField, UIControl);

public:
    // Auto-capitalization type.
    enum eAutoCapitalizationType
    {
        AUTO_CAPITALIZATION_TYPE_NONE = 0,
        AUTO_CAPITALIZATION_TYPE_WORDS,
        AUTO_CAPITALIZATION_TYPE_SENTENCES,
        AUTO_CAPITALIZATION_TYPE_ALL_CHARS,
    };

    // Auto-correction type.
    enum eAutoCorrectionType
    {
        AUTO_CORRECTION_TYPE_DEFAULT = 0,
        AUTO_CORRECTION_TYPE_NO,
        AUTO_CORRECTION_TYPE_YES
    };

    // Spell checking type.
    enum eSpellCheckingType
    {
        SPELL_CHECKING_TYPE_DEFAULT = 0,
        SPELL_CHECKING_TYPE_NO,
        SPELL_CHECKING_TYPE_YES
    };

    // Keyboard appearance.
    enum eKeyboardAppearanceType
    {
        KEYBOARD_APPEARANCE_DEFAULT = 0,
        KEYBOARD_APPEARANCE_ALERT
    };

    // Keyboard type.
    enum eKeyboardType
    {
        KEYBOARD_TYPE_DEFAULT = 0,
        KEYBOARD_TYPE_ASCII_CAPABLE,
        KEYBOARD_TYPE_NUMBERS_AND_PUNCTUATION,
        KEYBOARD_TYPE_URL,
        KEYBOARD_TYPE_NUMBER_PAD,
        KEYBOARD_TYPE_PHONE_PAD,
        KEYBOARD_TYPE_NAME_PHONE_PAD,
        KEYBOARD_TYPE_EMAIL_ADDRESS,
        KEYBOARD_TYPE_DECIMAL_PAD,
        KEYBOARD_TYPE_TWITTER,
    };

    // Return key type.
    enum eReturnKeyType
    {
        RETURN_KEY_DEFAULT = 0,
        RETURN_KEY_GO,
        RETURN_KEY_GOOGLE,
        RETURN_KEY_JOIN,
        RETURN_KEY_NEXT,
        RETURN_KEY_ROUTE,
        RETURN_KEY_SEARCH,
        RETURN_KEY_SEND,
        RETURN_KEY_YAHOO,
        RETURN_KEY_DONE,
        RETURN_KEY_EMERGENCY_CALL
    };

    enum eStartEditPolicy
    {
        START_EDIT_WHEN_FOCUSED,
        START_EDIT_BY_USER_REQUEST,
    };

    enum eStopEditPolicy
    {
        STOP_EDIT_WHEN_FOCUS_LOST,
        STOP_EDIT_BY_USER_REQUEST,
    };

    UITextField(const Rect& rect = Rect());

    void OnActive() override;
    void OnInactive() override;

    void OnFocused() override;
    void OnFocusLost() override;
    void OnTouchOutsideFocus() override;

    void SetDelegate(UITextFieldDelegate* delegate);
    UITextFieldDelegate* GetDelegate();

    void Update(float32 timeElapsed) override;

    bool IsEditing() const;
    void StartEdit();
    void StopEdit();

    DAVA_DEPRECATED(const WideString& GetText());
    DAVA_DEPRECATED(virtual void SetText(const WideString& text));

    String GetUtf8Text();
    void SetUtf8Text(const String& text);

    WideString GetAppliedChanges(int32 replacementLocation, int32 replacementLength, const WideString& replacementString);

    void Input(UIEvent* currentInput) override;

    /**
     \brief Sets contol input processing ability.
     */
    void SetInputEnabled(bool isEnabled, bool hierarchic = true) override;
    /**
     \brief Returns the font of control
     \returns Font font of the control
     */
    Font* GetFont() const;
    /**
     \brief Returns the font's size of control
     \returns Font's size of the control
     */
    float32 GetFontSize() const;
    /**
     \brief Returns the text color of control.
     \returns Color color of control's text
     */
    Color GetTextColor() const;
    /**
     \brief Returns text shadow offset relative to base text.
     \returns Vector2 with shadow offset for X and Y axis
     */
    Vector2 GetShadowOffset() const;
    /**
     \brief Returns color of text shadow.
     \returns Color of text shadow.
     */
    Color GetShadowColor() const;

    int32 GetTextAlign() const;

    void SetFocused();

    void ReleaseFocus();

    void SetSelectionColor(const Color& selectionColor);
    const Color& GetSelectionColor() const;

    /**
     \brief Sets the font of the control text.
     \param[in] font font used for text draw of the states.
     */
    void SetFont(Font* font);

    /** Set path to font. */
    void SetFontPath(const FilePath& fontPath);

    /** Return path to font. */
    const FilePath& GetFontPath() const;

    /**
     \brief Sets the color of the text.
     \param[in] fontColor font used for text draw of the states.
     */
    void SetTextColor(const Color& fontColor);
    /**
     \brief Sets the size of the font.
     \param[in] size font size to be set.
     */
    void SetFontSize(float32 size);
    /**
     \brief Sets shadow offset of text control.
     \param[in] offset offset of text shadow relative to base text.
     */
    void SetShadowOffset(const DAVA::Vector2& offset);
    /**
     \brief Sets shadow color of text control.
     \param[in] color color of text shadow.
     */
    void SetShadowColor(const Color& color);

    void SetTextAlign(int32 align);

    /**
     \brief Returns using RTL align flag
     \returns Using RTL align flag
     */
    TextBlock::eUseRtlAlign GetTextUseRtlAlign() const;

    /**
     \brief Sets using mirror align for RTL texts
     \param[in] useRrlAlign flag of support RTL align
     */
    void SetTextUseRtlAlign(TextBlock::eUseRtlAlign useRtlAlign);

    void SetSize(const DAVA::Vector2& newSize) override;
    void SetPosition(const Vector2& position) override;

    void SetMultiline(bool value);
    bool IsMultiline() const;

    /**
     \brief Set control text style hide.
     \param[in] isPassword draw text with hide char.
     */
    void SetIsPassword(bool isPassword);
    /**
     \brief Return is text style is hide.
     */
    bool IsPassword() const;

    /**
      \brief Auto-capitalization type.
     */
    int32 GetAutoCapitalizationType() const;
    void SetAutoCapitalizationType(int32 value);

    /**
      \brief Auto-correction type.
     */
    int32 GetAutoCorrectionType() const;
    void SetAutoCorrectionType(int32 value);

    /**
      \brief Spell checking type.
     */
    int32 GetSpellCheckingType() const;
    void SetSpellCheckingType(int32 value);

    /**
      \brief Keyboard appearance type.
     */
    int32 GetKeyboardAppearanceType() const;
    void SetKeyboardAppearanceType(int32 value);

    /**
      \brief Keyboard type.
     */
    int32 GetKeyboardType() const;
    void SetKeyboardType(int32 value);

    /**
      \brief Return key type.
     */
    int32 GetReturnKeyType() const;
    void SetReturnKeyType(int32 value);

    eStartEditPolicy GetStartEditPolicy() const;
    void SetStartEditPolicy(eStartEditPolicy policy);

    eStopEditPolicy GetStopEditPolicy() const;
    void SetStopEditPolicy(eStopEditPolicy policy);

    /**
      \brief Enable return key automatically.
     */
    bool IsEnableReturnKeyAutomatically() const;
    void SetEnableReturnKeyAutomatically(bool value);

    UITextField* Clone() override;
    void CopyDataFrom(UIControl* srcControl) override;

    // Cursor control.
    uint32 GetCursorPos();
    void SetCursorPos(uint32 pos);

    /**
      \brief Set maximum text length in text edit
      maxLength - >=0 - max count, -1 - unlimited count
     */
    void SetMaxLength(int32 maxLength);
    int32 GetMaxLength() const;

    const String& GetFontPresetName() const;

    void SetFontByPresetName(const String& presetName);

    void Draw(const UIGeometricData& geometricData) override;

    WideString GetVisibleText();

    virtual void OnStartEditing();
    virtual void OnStopEditing();

    virtual void OnKeyboardShown(const Rect& keyboardRect);
    virtual void OnKeyboardHidden();

protected:
    ~UITextField() override;
    void OnVisible() override;
    void OnInvisible() override;

private:
    void SetRenderToTexture(bool value);
    bool IsRenderToTexture() const;

    /**
    \brief Setups initial state to reset settings for cached native control.
    */
    void SetupDefaults();

    int32 GetStartEditPolicyAsInt() const;
    void SetStartEditPolicyFromInt(int32 policy);

    int32 GetStopEditPolicyAsInt() const;
    void SetStopEditPolicyFromInt(int32 policy);

    WideString text;
    String fontPresetName;
    UITextFieldDelegate* delegate = nullptr;
    float32 fontSize = 0.f; // Font size for native text fields
    float32 cursorBlinkingTime = 0.0f;

    // Keyboard customization params.
    eAutoCapitalizationType autoCapitalizationType;
    eAutoCorrectionType autoCorrectionType;
    eSpellCheckingType spellCheckingType;
    eKeyboardAppearanceType keyboardAppearanceType;
    eKeyboardType keyboardType;
    eReturnKeyType returnKeyType;
    eStartEditPolicy startEditPolicy = START_EDIT_BY_USER_REQUEST;
    eStopEditPolicy stopEditPolicy = STOP_EDIT_BY_USER_REQUEST;

    // All Boolean variables are grouped together because of DF-2149.
    bool isPassword;
    bool enableReturnKeyAutomatically;
    bool isMultiline = false;
    bool isEditing = false;

    // Make impl to be controlled by std::shared_ptr as on some platforms (e.g. uwp, android)
    // impl can live longer than its owner: native control can queue callback in UI thread
    // but impl's owner is already dead
    std::shared_ptr<TextFieldPlatformImpl> textFieldImpl;
    int32 maxLength = -1;
};

} // namespace DAVA
