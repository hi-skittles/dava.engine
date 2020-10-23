#include "UITextFieldStb.h"

#include "Engine/Engine.h"
#include "UI/UITextField.h"
#include "UI/UITextFieldDelegate.h"
#include "UI/UIStaticText.h"
#include "UI/Text/UITextSystem.h"
#include "UI/Text/UITextComponent.h"
#include "UI/UIControlSystem.h"
#include "UI/UIEvent.h"
#include "Time/SystemTimer.h"
#include "Render/2D/Systems/RenderSystem2D.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"
#include "UI/Render/UIClipContentComponent.h"
#include "Input/InputSystem.h"
#include "Utils/UTF8Utils.h"
#include "Utils/StringUtils.h"
#include "Utils/TextBox.h"

#include <numeric>

namespace DAVA
{
static float32 DEFAULT_CURSOR_WIDTH = 1.f;

namespace TextFieldStbImplDetails
{
static Vector2 TransformInputPoint(const Vector2& inputPoint, const Vector2& controlAbsPosition, const Vector2& controlScale)
{
    return (inputPoint - controlAbsPosition) / controlScale;
}

static UIStyleSheetPropertySet GetComponentPropertiesSet(const Type* componentType)
{
    UIStyleSheetPropertySet componentSet;
    UIStyleSheetPropertyDataBase* db = UIStyleSheetPropertyDataBase::Instance();
    for (int32 i = 0; i < UIStyleSheetPropertyDataBase::STYLE_SHEET_PROPERTY_COUNT; i++)
    {
        const UIStyleSheetPropertyDescriptor& descr = db->GetStyleSheetPropertyByIndex(i);
        componentSet.set(i, descr.group->componentType == componentType);
    }
    return componentSet;
}
}

TextFieldStbImpl::TextFieldStbImpl(Window* w, UITextField* control)
    : control(control)
    , stb(new StbTextEditBridge(this))
    , window(w)
{
    using namespace TextFieldStbImplDetails;

    stb->SetSingleLineMode(true); // Set default because UITextField is single line by default

    control->GetOrCreateComponent<UIClipContentComponent>();

    // Mark all styled properties as modified
    control->SetLocalPropertySet(GetComponentPropertiesSet(Type::Instance<UITextComponent>()));

    PrepareTextComponent();
}

TextFieldStbImpl::~TextFieldStbImpl()
{
    staticText = nullptr;
    SafeDelete(stb);
    control = nullptr;
}

void TextFieldStbImpl::PrepareTextComponent()
{
    staticText = control->GetOrCreateComponent<UITextComponent>();
    staticText->SetRequestedTextRectSize(UIStaticText::NO_REQUIRED_SIZE);
    staticText->SetForceBiDiSupportEnabled(true);
    // Ignore staticText parent color because under different platforms
    // we can't mix colors for text fields and parent backgrounds
    staticText->SetColorInheritType(UIControlBackground::eColorInheritType::COLOR_IGNORE_PARENT);
    GetTextBlock()->SetMeasureEnable(true);
}

void TextFieldStbImpl::Initialize()
{
    window->sizeChanged.Connect(this, &TextFieldStbImpl::OnWindowSizeChanged);
    Engine::Instance()->windowDestroyed.Connect(this, &TextFieldStbImpl::OnWindowDestroyed);
}

void TextFieldStbImpl::OwnerIsDying()
{
    if (window != nullptr)
    {
        window->sizeChanged.Disconnect(this);
        Engine::Instance()->windowDestroyed.Disconnect(this);
    }
}

void TextFieldStbImpl::OnWindowSizeChanged(Window* w, Size2f windowSize, Size2f surfaceSize)
{
    if (isEditing)
    {
        // Set lastCursorPos to some big value that is unlikely to happen
        // to force cursor draw in right place to cover case when window size
        // has changed but virtual size stays the same
        lastCursorPos = uint32(-1);
        UpdateRect(Rect());
    }
}

void TextFieldStbImpl::OnWindowDestroyed(Window* w)
{
    OwnerIsDying();
    window = nullptr;
}

void TextFieldStbImpl::SetDelegate(UITextFieldDelegate* d)
{
}

void TextFieldStbImpl::CopyDataFrom(TextFieldStbImpl* t)
{
    PrepareTextComponent();

    stb->CopyStbStateFrom(*t->stb);
    cursorTime = t->cursorTime;
    showCursor = t->showCursor;
}

void TextFieldStbImpl::OpenKeyboard()
{
    // On focus text field
    if (!isEditing)
    {
        SetCursorPos(GetTextLength());
        stb->SetSelectionStart(0);
        stb->SetSelectionEnd(0);
        isEditing = true;
        control->OnKeyboardShown(Rect());
    }
}

void TextFieldStbImpl::CloseKeyboard()
{
    if (isEditing)
    {
        isEditing = false;
        control->OnKeyboardHidden();
        selectionRects.clear();
    }
}

void TextFieldStbImpl::SetRenderToTexture(bool)
{
}

void TextFieldStbImpl::SetIsPassword(bool)
{
    needRedraw = true;
}

void TextFieldStbImpl::SetFontSize(float32 size)
{
    staticText->SetFontSize(size);
}

float32 TextFieldStbImpl::GetFontSize() const
{
    return staticText->GetFontSize();
}

void TextFieldStbImpl::SetText(const WideString& newText)
{
    WideString prevText = text;
    if (prevText != newText)
    {
        uint32 cursorPos = stb->GetCursorPosition();

        SelectAll();
        ignoreKeyPressedDelegate = true;
        if (newText.empty())
        {
            stb->Cut();
        }
        else
        {
            stb->Paste(newText);
        }
        stb->ClearUndoStack();
        ignoreKeyPressedDelegate = false;

        cursorPos = std::min(cursorPos, uint32(newText.length()));
        SetCursorPos(cursorPos);

        UITextFieldDelegate* delegate = control->GetDelegate();
        if (delegate)
        {
            delegate->TextFieldOnTextChanged(control, text, prevText, UITextFieldDelegate::eReason::CODE);
        }

        staticText->SetText(UTF8Utils::EncodeToUTF8(control->GetVisibleText()));

        needRedraw = true;
    }
}

void TextFieldStbImpl::UpdateRect(const Rect&)
{
    // see comment for TextFieldStbImpl class above

    if (control == GetEngineContext()->uiControlSystem->GetFocusedControl() && isEditing)
    {
        float32 timeElapsed = SystemTimer::GetFrameDelta();
        cursorTime += timeElapsed;
        if (cursorTime >= 0.5f)
        {
            cursorTime = 0;
            showCursor = !showCursor;
        }
        needRedraw = true;
    }
    else if (showCursor)
    {
        cursorTime = 0;
        showCursor = false;
        needRedraw = true;
    }

    if (!needRedraw)
    {
        return;
    }

    staticText->SetText(UTF8Utils::EncodeToUTF8(control->GetVisibleText()));
    needRedraw = false;

    if (lastCursorPos != stb->GetCursorPosition())
    {
        lastCursorPos = stb->GetCursorPosition();

        UpdateCursor(lastCursorPos, stb->IsInsertMode());
        UpdateOffset(cursorRect + staticTextOffset);
        staticText->SetTextOffset(staticTextOffset);
        // Fix cursor position for multiline if end of some line contains many
        // spaces over control size (same behavior in MS Word)
        if (isEditing)
        {
            if (!stb->IsSingleLineMode())
            {
                const Vector2& controlSize = control->GetSize();
                if (cursorRect.x + DEFAULT_CURSOR_WIDTH > controlSize.x)
                {
                    cursorRect.dx = DEFAULT_CURSOR_WIDTH;
                    cursorRect.x = controlSize.x - cursorRect.dx - 1.f;
                }
                else if (cursorRect.x + cursorRect.dx > controlSize.x)
                {
                    cursorRect.dx = controlSize.x - cursorRect.x - 1.f;
                }
            }

            // Show cursor again
            cursorTime = 0.f;
            showCursor = true;
        }
    }

    if (lastSelStart != stb->GetSelectionStart() || lastSelEnd != stb->GetSelectionEnd())
    {
        lastSelStart = stb->GetSelectionStart();
        lastSelEnd = stb->GetSelectionEnd();
        UpdateSelection(lastSelStart, lastSelEnd);
    }
}

void TextFieldStbImpl::SetAutoCapitalizationType(int32)
{
}

void TextFieldStbImpl::SetAutoCorrectionType(int32)
{
}

void TextFieldStbImpl::SetSpellCheckingType(int32)
{
}

void TextFieldStbImpl::SetKeyboardAppearanceType(int32)
{
}

void TextFieldStbImpl::SetKeyboardType(int32)
{
}

void TextFieldStbImpl::SetReturnKeyType(int32)
{
}

void TextFieldStbImpl::SetEnableReturnKeyAutomatically(int32)
{
}

bool TextFieldStbImpl::IsRenderToTexture() const
{
    return false;
}

uint32 TextFieldStbImpl::GetCursorPos() const
{
    return stb->GetCursorPosition();
}

void TextFieldStbImpl::SetCursorPos(int32 position)
{
    stb->SetCursorPosition(position);
}

void TextFieldStbImpl::SetMaxLength(int32 _maxLength)
{
    maxLength = _maxLength;
}

void TextFieldStbImpl::GetText(WideString& output)
{
    output = text;
}

WideString TextFieldStbImpl::GetText() const
{
    return text;
}

bool TextFieldStbImpl::IsCharAvaliable(WideString::value_type ch) const
{
    Font* f = GetRealFont();
    if (f)
    {
        return ch == '\n' || f->IsCharAvaliable(static_cast<char16>(ch));
    }
    return false;
}

bool TextFieldStbImpl::IsCopyToClipboardAllowed() const
{
    return control && !control->IsPassword();
}

void TextFieldStbImpl::SetInputEnabled(bool, bool hierarchic /*= true*/)
{
}

void TextFieldStbImpl::SetVisible(bool v)
{
}

void TextFieldStbImpl::SetFontName(const String& presetName)
{
    DropLastCursorAndSelection();
    staticText->SetFontName(presetName);
}

void TextFieldStbImpl::SetFont(Font* f)
{
    DropLastCursorAndSelection();
    staticText->SetFont(RefPtr<Font>::ConstructWithRetain(f));
}

void TextFieldStbImpl::SetFontPath(const FilePath& path)
{
    DropLastCursorAndSelection();
    staticText->SetFontPath(path);
}

const FilePath& TextFieldStbImpl::GetFontPath() const
{
    return staticText->GetFontPath();
}

Font* TextFieldStbImpl::GetFont() const
{
    return staticText->GetFont();
}

void TextFieldStbImpl::SetTextColor(const Color& c)
{
    staticText->SetColor(c);
}

void TextFieldStbImpl::SetShadowOffset(const Vector2& v)
{
    staticText->SetShadowOffset(v);
}

void TextFieldStbImpl::SetShadowColor(const Color& c)
{
    staticText->SetShadowColor(c);
}

void TextFieldStbImpl::SetTextAlign(int32 align)
{
    if (staticText->GetAlign() != align)
    {
        DropLastCursorAndSelection();
        staticText->SetAlign(align);
    }
}

TextBlock::eUseRtlAlign TextFieldStbImpl::GetTextUseRtlAlign()
{
    return staticText->GetUseRtlAlign();
}

void TextFieldStbImpl::SetTextUseRtlAlign(TextBlock::eUseRtlAlign align)
{
    if (staticText->GetUseRtlAlign() != align)
    {
        DropLastCursorAndSelection();
        staticText->SetUseRtlAlign(align);
    }
}

void TextFieldStbImpl::SetSize(const Vector2 vector2)
{
    DropLastCursorAndSelection();
}

void TextFieldStbImpl::SetRect(const Rect& rect)
{
    DropLastCursorAndSelection();
}

void TextFieldStbImpl::SetMultiline(bool is_multiline)
{
    auto multiline = is_multiline ? UITextComponent::eTextMultiline::MULTILINE_ENABLED : UITextComponent::eTextMultiline::MULTILINE_DISABLED;
    if (staticText->GetMultiline() != multiline)
    {
        DropLastCursorAndSelection();
        staticText->SetMultiline(multiline);
        stb->SetSingleLineMode(!is_multiline);
    }
}

Color TextFieldStbImpl::GetTextColor()
{
    return staticText->GetColor();
}

Vector2 TextFieldStbImpl::GetShadowOffset()
{
    return staticText->GetShadowOffset();
}

Color TextFieldStbImpl::GetShadowColor()
{
    return staticText->GetShadowColor();
}

rhi::int32 TextFieldStbImpl::GetTextAlign()
{
    return GetTextBlock()->GetVisualAlign();
}

void TextFieldStbImpl::SystemDraw(const UIGeometricData& d)
{
    const Vector2& scale = d.scale;
    const Vector2& offset = d.GetUnrotatedRect().GetPosition();

    for (const Rect& r : selectionRects)
    {
        Rect sr = r;
        sr.x += staticTextOffset.x;
        sr.y += staticTextOffset.y;
        sr.x *= scale.x;
        sr.y *= scale.y;
        sr.dx *= scale.x;
        sr.dy *= scale.y;
        RenderSystem2D::Instance()->FillRect(sr + offset, selectionColor);
    }

    if (showCursor)
    {
        Rect sr = cursorRect;
        sr.x += staticTextOffset.x;
        sr.y += staticTextOffset.y;
        sr.x *= scale.x;
        sr.y *= scale.y;
        sr.dx *= scale.x;
        sr.dy *= scale.y;
        RenderSystem2D::Instance()->FillRect(sr + offset, staticText->GetColor());
    }
}

void TextFieldStbImpl::SetSelectionColor(const Color& _selectionColor)
{
    selectionColor = _selectionColor;
}

const Color& TextFieldStbImpl::GetSelectionColor() const
{
    return selectionColor;
}

uint32 TextFieldStbImpl::InsertText(uint32 position, const WideString::value_type* str, uint32 length)
{
    WideString insertText(str, length);
    UITextFieldDelegate* delegate = control->GetDelegate();
    bool apply = true;
    if (!ignoreKeyPressedDelegate && delegate)
    {
        apply = delegate->TextFieldKeyPressed(control, position, 0, insertText);
    }
    if (apply)
    {
        // Additional check if somebody in delegate TextFieldKeyPressed change text manually
        if (position > uint32(text.length()))
        {
            position = uint32(text.length());
        }

        if (control->GetMaxLength() > 0)
        {
            int32 outOfBounds = int32(text.length()) - control->GetMaxLength() + int32(length);
            if (outOfBounds < 0)
                outOfBounds = 0;
            if (outOfBounds > int32(length))
                outOfBounds = int32(length);
            length -= outOfBounds;
        }
        text.insert(position, str, length);
        DropLastCursorAndSelection();
        return length;
    }
    return 0;
}

uint32 TextFieldStbImpl::DeleteText(uint32 position, uint32 length)
{
    UITextFieldDelegate* delegate = control->GetDelegate();
    bool apply = true;
    if (!ignoreKeyPressedDelegate && delegate)
    {
        WideString str;
        apply = delegate->TextFieldKeyPressed(control, position, length, str);
    }
    if (apply)
    {
        text.erase(position, length);
        DropLastCursorAndSelection();
        return length;
    }
    return 0;
}

const TextBox* TextFieldStbImpl::GetTextBox() const
{
    return GetTextBlock()->GetTextBox();
}

uint32 TextFieldStbImpl::GetTextLength() const
{
    return uint32(text.length());
}

WideString::value_type TextFieldStbImpl::GetCharAt(uint32 i) const
{
    return text.at(i);
}

void TextFieldStbImpl::CorrectPos(const TextBox* tb, uint32& pos, bool& cursorRight)
{
#if 1 // Simple correcting with RTL check
    const uint32 charsCount = tb->GetCharactersCount();
    if (pos < charsCount)
    {
        while (pos > 0 && tb->GetCharacter(pos).skip)
        {
            pos--;
        }
        cursorRight = tb->GetCharacter(pos).rtl;
    }
    else
    {
        pos = charsCount - 1;
        cursorRight = !tb->GetCharacter(pos).rtl;
    }
#else // Correct cursor like notepad (more complex)
    int32 charsCount = tb->GetCharactersCount();
    if (pos < charsCount)
    {
        while (pos > 0 && tb->GetCharacter(pos).skip)
        {
            pos--;
        }
        cursorRight = tb->GetCharacter(pos).rtl;
        if ((pos > 0) && (tb->GetCharacter(pos - 1).rtl != tb->GetCharacter(pos).rtl))
        {
            pos--;
            cursorRight = !tb->GetCharacter(pos).rtl;
        }
    }
    else
    {
        pos = charsCount - 1;
        cursorRight = !tb->GetCharacter(pos).rtl;
    }
#endif
}

void TextFieldStbImpl::UpdateSelection(uint32 start, uint32 end)
{
    selectionRects.clear();
    uint32 selStart = std::min(start, end);
    uint32 selEnd = std::max(start, end);
    const TextBox* tb = GetTextBox();
    if (selStart < selEnd && selEnd <= tb->GetCharactersCount())
    {
        for (uint32 i = selStart; i < selEnd; ++i)
        {
            const TextBox::Character& c = tb->GetCharacter(i);
            if (c.skip)
            {
                continue;
            }
            const TextBox::Line& line = tb->GetLine(c.lineIndex);
            if (line.skip)
            {
                continue;
            }

            Rect r;
            r.x = c.xoffset + line.xoffset;
            r.y = line.yoffset;
            r.dx = c.xadvance;
            r.dy = line.yadvance;

            selectionRects.push_back(r);
        }
    }
}

void TextFieldStbImpl::UpdateCursor(uint32 cursorPos, bool insertMode)
{
    const TextBox* tb = GetTextBox();

    Rect r;
    r.dx = DEFAULT_CURSOR_WIDTH;

    // Ensure cursor width is not less than 1 physical pixel for properly
    // drawing when window is very small
    VirtualCoordinatesSystem* vcs = window->GetUIControlSystem()->vcs;
    r.dx = vcs->ConvertVirtualToPhysicalX(r.dx);
    r.dx = std::max(r.dx, 1.f);
    r.dx = vcs->ConvertPhysicalToVirtualX(r.dx);

    int32 charsCount = tb->GetCharactersCount();
    if (charsCount > 0)
    {
        bool atEnd = false;
        CorrectPos(tb, cursorPos, atEnd);

        const TextBox::Character& c = tb->GetCharacter(cursorPos);
        const TextBox::Line& line = tb->GetLine(c.lineIndex);

        r.x += c.xoffset + line.xoffset;
        if (atEnd)
        {
            r.x += c.xadvance;
        }
        r.y += line.yoffset;
        r.dy = line.yadvance;
    }
    else
    {
        r.dy = GetRealFont() ? GetRealFont()->GetFontHeight(GetRealFontSize()) : 0.f;

        int32 ctrlAlign = control->GetTextAlign();
        if (ctrlAlign & ALIGN_RIGHT)
        {
            r.x += control->GetSize().x - r.dx;
        }
        else if (ctrlAlign & ALIGN_HCENTER)
        {
            r.x += (control->GetSize().x - r.dx) * 0.5f;
        }

        if (ctrlAlign & ALIGN_BOTTOM)
        {
            r.y += control->GetSize().y - r.dy;
        }
        else if (ctrlAlign & ALIGN_VCENTER)
        {
            r.y += (control->GetSize().y - r.dy) * 0.5f;
        }
    }

    cursorRect = r;
}

void TextFieldStbImpl::UpdateOffset(const Rect& visibleRect)
{
    const Vector2& controlSize = control->GetSize();
    const Vector2& textSize = GetTextBlock()->GetTextSize();
    const Vector2 offsetMoveDelta = controlSize * 0.25f;

    if (controlSize.dx < textSize.dx && stb->IsSingleLineMode())
    {
        if (visibleRect.x < 0.f)
        {
            staticTextOffset.x += -visibleRect.x + offsetMoveDelta.x;
        }
        else if (visibleRect.x + visibleRect.dx > controlSize.dx)
        {
            staticTextOffset.x += controlSize.dx - visibleRect.x - visibleRect.dx - offsetMoveDelta.x;
        }
        staticTextOffset.x = std::max(std::min(0.f, staticTextOffset.x), controlSize.dx - textSize.dx - visibleRect.dx - 1.f);
    }
    else
    {
        staticTextOffset.x = 0.f;
    }

    if (controlSize.dy < textSize.dy && !stb->IsSingleLineMode())
    {
        if (visibleRect.y < 0.f)
        {
            staticTextOffset.y += -visibleRect.y + offsetMoveDelta.y;
        }
        else if (visibleRect.y + visibleRect.dy > controlSize.dy)
        {
            staticTextOffset.y += controlSize.dy - visibleRect.y - visibleRect.dy - offsetMoveDelta.y;
        }
        staticTextOffset.y = std::max(std::min(0.f, staticTextOffset.y), controlSize.dy - textSize.dy - 1.f);
    }
    else
    {
        staticTextOffset.y = 0.f;
    }
}

void TextFieldStbImpl::Input(UIEvent* currentInput)
{
    using namespace TextFieldStbImplDetails;

    if (control != GetEngineContext()->uiControlSystem->GetFocusedControl())
        return;

    if (currentInput->phase == UIEvent::Phase::ENDED)
    {
        if (control->GetStartEditPolicy() == UITextField::START_EDIT_BY_USER_REQUEST)
        {
            control->StartEdit();
        }
    }

    if (!control->IsEditing())
        return;

    bool textCanChanged = false;
    WideString prevText(text);

    eModifierKeys modifiers = currentInput->modifiers;
    bool isAlt = (modifiers & eModifierKeys::ALT) == eModifierKeys::ALT;

    if (currentInput->phase == UIEvent::Phase::KEY_DOWN ||
        currentInput->phase == UIEvent::Phase::KEY_DOWN_REPEAT)
    {
        if ((currentInput->key == eInputElements::KB_ENTER || currentInput->key == eInputElements::KB_NUMPAD_ENTER) && !isAlt)
        {
            if (control->GetDelegate())
            {
                control->GetDelegate()->TextFieldShouldReturn(control);
            }
            return;
        }
        else if (currentInput->key == eInputElements::KB_ESCAPE)
        {
            if (control->GetDelegate())
            {
                control->GetDelegate()->TextFieldShouldCancel(control);
            }
            return;
        }
        else
        {
            textCanChanged = stb->SendKey(currentInput->key, modifiers);
        }
    }
    if (currentInput->phase == UIEvent::Phase::CHAR ||
        currentInput->phase == UIEvent::Phase::CHAR_REPEAT)
    {
        textCanChanged = stb->SendKeyChar(currentInput->keyChar, modifiers);
    }
    else if (currentInput->phase == UIEvent::Phase::BEGAN)
    {
        Vector2 localPoint = TransformInputPoint(currentInput->point, control->GetAbsolutePosition(), control->GetGeometricData().scale);
        stb->Click(localPoint - staticTextOffset);
        if (currentInput->tapCount == 2)
        {
            stb->SelectWord();
        }
        else if (currentInput->tapCount > 2)
        {
            stb->SelectAll();
        }
    }
    else if (currentInput->phase == UIEvent::Phase::DRAG)
    {
        Vector2 localPoint = TransformInputPoint(currentInput->point, control->GetAbsolutePosition(), control->GetGeometricData().scale);
        stb->Drag(localPoint - staticTextOffset);
    }

    if (textCanChanged && prevText != text)
    {
        UITextFieldDelegate* delegate = control->GetDelegate();
        if (delegate)
        {
            delegate->TextFieldOnTextChanged(control, text, prevText, UITextFieldDelegate::eReason::USER);
        }

        staticText->SetText(UTF8Utils::EncodeToUTF8(control->GetVisibleText()));
        needRedraw = true;
    }

    currentInput->SetInputHandledType(UIEvent::INPUT_HANDLED_SOFT); // Drag is not handled - see please DF-2508.
}

void TextFieldStbImpl::SelectAll()
{
    stb->SelectAll();
}

void TextFieldStbImpl::DropLastCursorAndSelection()
{
    lastCursorPos = INVALID_POS;
    lastSelStart = INVALID_POS;
    lastSelEnd = INVALID_POS;
}

TextBlock* TextFieldStbImpl::GetTextBlock() const
{
    // Apply component changes to internal TextBlock
    if (staticText->IsModified())
    {
        UIControlSystem* ucs = control->GetScene();
        if (ucs)
        {
            ucs->GetTextSystem()->ApplyData(staticText.Get());
        }
        else // Legacy support
        {
            Engine::Instance()->GetContext()->uiControlSystem->GetTextSystem()->ApplyData(staticText.Get());
        }
    }
    return staticText->GetLink()->GetTextBlock();
}

Font* TextFieldStbImpl::GetRealFont() const
{
    return GetFont() ? GetFont() : GetTextBlock()->GetFont();
}

float32 TextFieldStbImpl::GetRealFontSize() const
{
    return !FLOAT_EQUAL(GetFontSize(), 0.f) ? GetFontSize() : GetTextBlock()->GetFontSize();
}
}
