#pragma once

#include "Base/BaseTypes.h"
#include "Render/2D/TextBlock.h"
#include "UI/Private/StbTextEditBridge.h"

namespace DAVA
{
class UITextField;
class UITextComponent;
class UIGeometricData;
class Font;
class Color;
class UIEvent;
class Vector2;
class TextBox;
class UITextFieldDelegate;
struct Rect;
class Window;

class TextFieldStbImpl : public StbTextEditBridge::StbTextDelegate
{
public:
    static const uint32 INVALID_POS = uint32(-1);

    friend class UITextField;

    TextFieldStbImpl(Window* w, UITextField* control);
    ~TextFieldStbImpl();
    void PrepareTextComponent();
    void Initialize();
    void OwnerIsDying();
    void SetDelegate(UITextFieldDelegate*);
    void CopyDataFrom(TextFieldStbImpl* t);
    void OpenKeyboard();
    void CloseKeyboard();
    void SetRenderToTexture(bool);
    void SetIsPassword(bool);
    void SetFontSize(float32);
    float32 GetFontSize() const;
    void SetText(const WideString& text);
    void UpdateRect(const Rect&);
    void SetAutoCapitalizationType(int32);
    void SetAutoCorrectionType(int32);
    void SetSpellCheckingType(int32);
    void SetKeyboardAppearanceType(int32);
    void SetKeyboardType(int32);
    void SetReturnKeyType(int32);
    void SetEnableReturnKeyAutomatically(int32);
    bool IsRenderToTexture() const;
    uint32 GetCursorPos() const;
    void SetCursorPos(int32);
    void SetMaxLength(int32);
    void GetText(WideString&);
    void SetInputEnabled(bool, bool hierarchic = true);
    void SetVisible(bool v);
    Font* GetFont() const;
    void SetFontName(const String& presetName);
    void SetFont(Font* f);
    void SetFontPath(const FilePath& path);
    const FilePath& GetFontPath() const;
    void SetTextColor(const Color& c);
    void SetShadowOffset(const Vector2& v);
    void SetShadowColor(const Color& c);
    void SetTextAlign(int32 align);
    TextBlock::eUseRtlAlign GetTextUseRtlAlign();
    void SetTextUseRtlAlign(TextBlock::eUseRtlAlign align);
    void SetSize(const Vector2 vector2);
    void SetMultiline(bool is_multiline);
    Color GetTextColor();
    Vector2 GetShadowOffset();
    Color GetShadowColor();
    int32 GetTextAlign();
    void SetRect(const Rect& rect);
    void SystemDraw(const UIGeometricData& d);
    void SetSelectionColor(const Color& selectionColor);
    const Color& GetSelectionColor() const;
    void Input(UIEvent* currentInput);

    void SelectAll();

    // StbTextEditBridge::StbTextDelegate
    uint32 InsertText(uint32 position, const WideString::value_type* str, uint32 length) override;
    uint32 DeleteText(uint32 position, uint32 length) override;
    const TextBox* GetTextBox() const override;
    uint32 GetTextLength() const override;
    WideString::value_type GetCharAt(uint32 i) const override;
    WideString GetText() const override;
    bool IsCharAvaliable(WideString::value_type ch) const override;
    bool IsCopyToClipboardAllowed() const override;

private:
    void DropLastCursorAndSelection();
    void CorrectPos(const TextBox* tb, uint32& pos, bool& cursorRight);
    void UpdateSelection(uint32 start, uint32 end);
    void UpdateCursor(uint32 cursorPos, bool insertMode);
    void UpdateOffset(const Rect& visibleRect);

    void OnWindowSizeChanged(Window* w, Size2f windowSize, Size2f surfaceSize);
    void OnWindowDestroyed(Window* w);

    TextBlock* GetTextBlock() const;
    Font* GetRealFont() const;
    float32 GetRealFontSize() const;

    RefPtr<UITextComponent> staticText; // Component for displaying text
    UITextField* control = nullptr; // Weak link to parent text field
    StbTextEditBridge* stb = nullptr;
    WideString text;
    float32 cursorTime = 0.0f;
    int32 maxLength = 0;
    bool needRedraw = true;
    bool showCursor = true;
    bool isEditing = false;
    bool ignoreKeyPressedDelegate = false;
    Color selectionColor = Color(0.f, 0.f, 0.7f, 0.7f);
    Vector<Rect> selectionRects;
    Rect cursorRect;
    Vector2 staticTextOffset;
    uint32 lastCursorPos = INVALID_POS;
    uint32 lastSelStart = INVALID_POS;
    uint32 lastSelEnd = INVALID_POS;
    Window* window = nullptr;
};

} // end namespace DAVA
