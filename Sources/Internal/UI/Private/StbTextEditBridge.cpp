#include "StbTextEditBridge.h"
#include "Debug/DVAssert.h"
#include "Utils/TextBox.h"
#include "UI/UIEvent.h"
#include "Utils/StringUtils.h"
#include "Render/2D/Font.h"
#include "Clipboard/Clipboard.h"

#if __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

#define STB_TEXTEDIT_CHARTYPE DAVA::WideString::value_type
#define STB_TEXTEDIT_STRING DAVA::StbTextEditBridge
#define STB_TEXTEDIT_NEWLINE L'\n'

#define K_VKEY 0x00010000
#define K_SHIFT 0x00020000
#define K_CTRL 0x00040000
#define K_ALT 0x00080000
#define K_CMD 0x00100000

#include <stb/stb_textedit.h>
#include <cwctype>

inline void stb_layoutrow(StbTexteditRow* row, STB_TEXTEDIT_STRING* str, int start_i)
{
    const DAVA::TextBox* tb = str->GetDelegate()->GetTextBox();
    if (DAVA::uint32(start_i) >= tb->GetCharactersCount())
    {
        return;
    }

    const DAVA::TextBox::Character& ch = tb->GetCharacter(start_i);
    const DAVA::TextBox::Line& line = tb->GetLine(ch.lineIndex);

    row->num_chars = line.length;
    row->x0 = line.xoffset;
    row->x1 = line.xoffset + line.xadvance;
    row->baseline_y_delta = 0.f;
    row->ymin = line.yoffset;
    row->ymax = line.yoffset + line.yadvance;

    // If single line mode enabled then extend height for first/last
    // lines to size of text field
    if (str->IsSingleLineMode())
    {
        if (ch.lineIndex == 0)
        {
            row->ymin = std::numeric_limits<float>::lowest();
        }
        if (ch.lineIndex == tb->GetLinesCount() - 1)
        {
            row->ymax = std::numeric_limits<float>::max();
        }
    }
}

inline void stb_layoutchar(STB_TEXTEDIT_STRING* str, int n, int i, float* x0, float* x1)
{
    const DAVA::int32 li = n + i;
    const DAVA::TextBox* tb = str->GetDelegate()->GetTextBox();
    if (DAVA::uint32(li) >= tb->GetCharactersCount())
    {
        return;
    }

    const DAVA::TextBox::Character& ch = tb->GetCharacter(li);
    const DAVA::TextBox::Line& line = tb->GetLine(ch.lineIndex);
    if (x0 != nullptr)
    {
        *x0 = ch.xoffset + line.xoffset;
    }
    if (x1 != nullptr)
    {
        *x1 = ch.xoffset + ch.xadvance + line.xoffset;
    }
}

inline float stb_getwidth(STB_TEXTEDIT_STRING* str, int n, int i)
{
    const DAVA::TextBox* tb = str->GetDelegate()->GetTextBox();
    DAVA::int32 li = n + i;
    if (tb->GetCharactersCount() > DAVA::uint32(li))
    {
        DAVA::int32 vi = tb->GetCharacter(li).visualIndex;
        return tb->GetCharacter(vi).xadvance;
    }
    return 0.f;
}

inline int stb_insertchars(STB_TEXTEDIT_STRING* str, int pos, STB_TEXTEDIT_CHARTYPE* newtext, int num)
{
    return int(str->GetDelegate()->InsertText(DAVA::uint32(pos), newtext, DAVA::uint32(num)));
}

inline int stb_deletechars(STB_TEXTEDIT_STRING* str, int pos, int num)
{
    return int(str->GetDelegate()->DeleteText(DAVA::uint32(pos), DAVA::uint32(num)));
}

inline int stb_stringlen(STB_TEXTEDIT_STRING* str)
{
    return int(str->GetDelegate()->GetTextLength());
}

inline int stb_keytotext(int key)
{
    if (key & K_VKEY)
        return -1;
    return key;
}

inline STB_TEXTEDIT_CHARTYPE stb_getchar(STB_TEXTEDIT_STRING* str, int i)
{
    return str->GetDelegate()->GetCharAt(DAVA::uint32(i));
}

inline int stb_isspace(STB_TEXTEDIT_CHARTYPE ch)
{
    return iswspace(ch) || iswpunct(ch);
}

#define STB_TEXTEDIT_LAYOUTROW stb_layoutrow
#define STB_DAVA_TEXTEDIT_LAYOUTCHAR stb_layoutchar
#define STB_TEXTEDIT_INSERTCHARS stb_insertchars
#define STB_TEXTEDIT_DELETECHARS stb_deletechars
#define STB_TEXTEDIT_STRINGLEN stb_stringlen
#define STB_TEXTEDIT_GETWIDTH stb_getwidth
#define STB_TEXTEDIT_KEYTOTEXT stb_keytotext
#define STB_TEXTEDIT_GETCHAR stb_getchar
#define STB_TEXTEDIT_IS_SPACE stb_isspace

#if defined(__DAVAENGINE_MACOS__) || defined(__DAVAENGINE_IOS__) // Apple-style hot keys

#define STB_TEXTEDIT_K_SHIFT (K_SHIFT)
#define STB_TEXTEDIT_K_LEFT (K_VKEY | int(DAVA::eInputElements::KB_LEFT))
#define STB_TEXTEDIT_K_RIGHT (K_VKEY | int(DAVA::eInputElements::KB_RIGHT))
#define STB_TEXTEDIT_K_UP (K_VKEY | int(DAVA::eInputElements::KB_UP))
#define STB_TEXTEDIT_K_DOWN (K_VKEY | int(DAVA::eInputElements::KB_DOWN))
#define STB_TEXTEDIT_K_LINESTART (K_VKEY | int(DAVA::eInputElements::KB_HOME))
#define STB_TEXTEDIT_K_LINESTART2 (K_VKEY | K_CMD | int(DAVA::eInputElements::KB_LEFT))
#define STB_TEXTEDIT_K_LINEEND (K_VKEY | int(DAVA::eInputElements::KB_END))
#define STB_TEXTEDIT_K_LINEEND2 (K_VKEY | K_CMD | int(DAVA::eInputElements::KB_RIGHT))
#define STB_TEXTEDIT_K_TEXTSTART (K_VKEY | K_CMD | int(DAVA::eInputElements::KB_UP))
#define STB_TEXTEDIT_K_TEXTEND (K_VKEY | K_CMD | int(DAVA::eInputElements::KB_DOWN))
#define STB_TEXTEDIT_K_WORDLEFT (K_VKEY | K_ALT | int(DAVA::eInputElements::KB_LEFT))
#define STB_TEXTEDIT_K_WORDRIGHT (K_VKEY | K_ALT | int(DAVA::eInputElements::KB_RIGHT))
#define STB_TEXTEDIT_K_INSERT (K_VKEY | int(DAVA::eInputElements::KB_INSERT))
#define STB_TEXTEDIT_K_DELETE (K_VKEY | int(DAVA::eInputElements::KB_DELETE))
#define STB_TEXTEDIT_K_BACKSPACE (K_VKEY | int(DAVA::eInputElements::KB_BACKSPACE))
#define STB_TEXTEDIT_K_UNDO (K_VKEY | K_CMD | int(DAVA::eInputElements::KB_Z))
#define STB_TEXTEDIT_K_REDO (K_VKEY | K_CMD | K_SHIFT | int(DAVA::eInputElements::KB_Z))

#define K_VKEY_SELECT_ALL (K_VKEY | K_CMD | int(DAVA::eInputElements::KB_A))
#define K_VKEY_CUT (K_VKEY | K_CMD | int(DAVA::eInputElements::KB_X))
#define K_VKEY_COPY (K_VKEY | K_CMD | int(DAVA::eInputElements::KB_C))
#define K_VKEY_PASTE (K_VKEY | K_CMD | int(DAVA::eInputElements::KB_V))
#define K_VKEY_DELETE_WORD (K_VKEY | K_ALT | int(DAVA::eInputElements::KB_DELETE))
#define K_VKEY_BACKSPACE_WORD (K_VKEY | K_ALT | int(DAVA::eInputElements::KB_BACKSPACE))

#else // Win-style hot keys

#define STB_TEXTEDIT_K_SHIFT (K_SHIFT)
#define STB_TEXTEDIT_K_LEFT (K_VKEY | int(DAVA::eInputElements::KB_LEFT))
#define STB_TEXTEDIT_K_RIGHT (K_VKEY | int(DAVA::eInputElements::KB_RIGHT))
#define STB_TEXTEDIT_K_UP (K_VKEY | int(DAVA::eInputElements::KB_UP))
#define STB_TEXTEDIT_K_DOWN (K_VKEY | int(DAVA::eInputElements::KB_DOWN))
#define STB_TEXTEDIT_K_LINESTART (K_VKEY | int(DAVA::eInputElements::KB_HOME))
#define STB_TEXTEDIT_K_LINEEND (K_VKEY | int(DAVA::eInputElements::KB_END))
#define STB_TEXTEDIT_K_TEXTSTART (K_VKEY | K_CTRL | int(DAVA::eInputElements::KB_HOME))
#define STB_TEXTEDIT_K_TEXTEND (K_VKEY | K_CTRL | int(DAVA::eInputElements::KB_END))
#define STB_TEXTEDIT_K_WORDLEFT (K_VKEY | K_CTRL | int(DAVA::eInputElements::KB_LEFT))
#define STB_TEXTEDIT_K_WORDRIGHT (K_VKEY | K_CTRL | int(DAVA::eInputElements::KB_RIGHT))
#define STB_TEXTEDIT_K_INSERT (K_VKEY | int(DAVA::eInputElements::KB_INSERT))
#define STB_TEXTEDIT_K_DELETE (K_VKEY | int(DAVA::eInputElements::KB_DELETE))
#define STB_TEXTEDIT_K_BACKSPACE (K_VKEY | int(DAVA::eInputElements::KB_BACKSPACE))
#define STB_TEXTEDIT_K_UNDO (K_VKEY | K_CTRL | int(DAVA::eInputElements::KB_Z))
#define STB_TEXTEDIT_K_REDO (K_VKEY | K_CTRL | int(DAVA::eInputElements::KB_Y))

#define K_VKEY_SELECT_ALL (K_VKEY | K_CTRL | int(DAVA::eInputElements::KB_A))
#define K_VKEY_CUT (K_VKEY | K_CTRL | int(DAVA::eInputElements::KB_X))
#define K_VKEY_COPY (K_VKEY | K_CTRL | int(DAVA::eInputElements::KB_C))
#define K_VKEY_PASTE (K_VKEY | K_CTRL | int(DAVA::eInputElements::KB_V))
#define K_VKEY_DELETE_WORD (K_VKEY | K_CTRL | int(DAVA::eInputElements::KB_DELETE))
#define K_VKEY_BACKSPACE_WORD (K_VKEY | K_CTRL | int(DAVA::eInputElements::KB_BACKSPACE))

#endif

#define STB_TEXTEDIT_IMPLEMENTATION
#include <stb/stb_textedit.h>

#if __clang__
#pragma clang diagnostic pop
#endif

////////////////////////////////////////////////////////////////////////////////

namespace DAVA
{
struct StbState : public STB_TexteditState
{
};

StbTextEditBridge::StbTextEditBridge(StbTextDelegate* delegate)
    : stb_state(new StbState())
    , delegate(delegate)
{
    DVASSERT(delegate, "StbTextEditBridge must be created with delegate!");
    stb_textedit_initialize_state(stb_state, 0);
}

StbTextEditBridge::StbTextEditBridge(const StbTextEditBridge& c)
    : stb_state(new StbState(*c.stb_state))
{
}

StbTextEditBridge::~StbTextEditBridge()
{
    SafeDelete(stb_state);
}

void StbTextEditBridge::CopyStbStateFrom(const StbTextEditBridge& c)
{
    DVASSERT(stb_state);
    DVASSERT(c.stb_state);
    Memcpy(stb_state, c.stb_state, sizeof(StbState));
}

bool StbTextEditBridge::SendKey(eInputElements key, eModifierKeys modifiers)
{
    uint32 code = K_VKEY | static_cast<uint32>(key);
    if ((modifiers & eModifierKeys::CONTROL) != eModifierKeys::NONE)
    {
        code |= K_CTRL;
    }
    if ((modifiers & eModifierKeys::ALT) != eModifierKeys::NONE)
    {
        code |= K_ALT;
    }
    if ((modifiers & eModifierKeys::SHIFT) != eModifierKeys::NONE)
    {
        code |= K_SHIFT;
    }
    if ((modifiers & eModifierKeys::COMMAND) != eModifierKeys::NONE)
    {
        code |= K_CMD;
    }

    switch (code)
    {
    case K_VKEY_SELECT_ALL:
        SelectAll();
        return false;
    case K_VKEY_DELETE_WORD:
        SendRaw(STB_TEXTEDIT_K_WORDRIGHT | STB_TEXTEDIT_K_SHIFT);
        return SendRaw(STB_TEXTEDIT_K_DELETE);
    case K_VKEY_BACKSPACE_WORD:
        SendRaw(STB_TEXTEDIT_K_WORDLEFT | STB_TEXTEDIT_K_SHIFT);
        return SendRaw(STB_TEXTEDIT_K_BACKSPACE);
    case K_VKEY_CUT:
        return delegate->IsCopyToClipboardAllowed() && CutToClipboard(); // Can modify text
    case K_VKEY_COPY:
        return delegate->IsCopyToClipboardAllowed() && CopyToClipboard();
    case K_VKEY_PASTE:
        return delegate->IsPasteFromClipboardAllowed() && PasteFromClipboard(); // Can modify text
    case STB_TEXTEDIT_K_UP:
    case STB_TEXTEDIT_K_DOWN:
        if (IsSingleLineMode())
            return false;
    default:
        return SendRaw(code);
    }
}

bool StbTextEditBridge::SendKeyChar(uint32 keyChar, eModifierKeys modifiers)
{
    if ((modifiers & eModifierKeys::COMMAND) != eModifierKeys::NONE)
    {
        // Skip CMD+char input under MacOS
        return false;
    }

    if (keyChar == '\r' || keyChar == '\n')
    {
        if (IsSingleLineMode())
        {
            // Skip line feed for single line fields
            return false;
        }
        // Transform carriage return to line feed
        keyChar = '\n';
    }
    else if (std::iswcntrl(keyChar))
    {
        // Skip control characters (\b, \t, ^a, ^c, etc.)
        // P.S. backspace already processed in SendKey
        return false;
    }

    if (GetDelegate()->IsCharAvaliable(static_cast<char16>(keyChar)))
    {
        // Send char only if it is allowed
        return SendRaw(keyChar); // Can modify text
    }

    return false;
}

bool DAVA::StbTextEditBridge::SendRaw(uint32 codePoint)
{
    return stb_textedit_key(this, stb_state, codePoint) != 0;
}

bool StbTextEditBridge::Cut()
{
    return stb_textedit_cut(this, stb_state) != 0;
}

bool StbTextEditBridge::Paste(const WideString& str)
{
    // Because stb_textedit_paste didn't work correctly with selected text
    // we first cut selection and paste new text after
    bool hasCutted = stb_textedit_cut(this, stb_state) != 0;
    bool hasPasted = stb_textedit_paste(this, stb_state, str.c_str(), int(str.length())) != 0;
    return hasCutted || hasPasted;
}

void StbTextEditBridge::Click(const Vector2& point)
{
    stb_textedit_click(this, stb_state, point.x, point.y);
}

void StbTextEditBridge::Drag(const Vector2& point)
{
    stb_textedit_drag(this, stb_state, point.x, point.y);
}

uint32 StbTextEditBridge::GetSelectionStart() const
{
    return static_cast<uint32>(stb_state->select_start);
}

void StbTextEditBridge::SetSelectionStart(uint32 position) const
{
    stb_state->select_start = static_cast<int>(position);
}

uint32 StbTextEditBridge::GetSelectionEnd() const
{
    return static_cast<uint32>(stb_state->select_end);
}

void StbTextEditBridge::SetSelectionEnd(uint32 position) const
{
    stb_state->select_end = static_cast<int>(position);
}

uint32 StbTextEditBridge::GetCursorPosition() const
{
    return static_cast<uint32>(stb_state->cursor);
}

void StbTextEditBridge::SetCursorPosition(uint32 position) const
{
    stb_state->cursor = static_cast<int>(position);
}

void StbTextEditBridge::SetSingleLineMode(bool signleLine)
{
    stb_state->single_line = static_cast<unsigned char>(signleLine);
}

bool StbTextEditBridge::IsSingleLineMode() const
{
    return stb_state->single_line != 0;
}

bool StbTextEditBridge::IsInsertMode() const
{
    return stb_state->insert_mode != 0;
}

void StbTextEditBridge::SelectWord()
{
    SendRaw(STB_TEXTEDIT_K_WORDLEFT);
    SendRaw(STB_TEXTEDIT_K_WORDRIGHT | STB_TEXTEDIT_K_SHIFT);
}

void StbTextEditBridge::SelectAll()
{
    SetSelectionStart(0);
    SetSelectionEnd(GetDelegate()->GetTextLength());
    SetCursorPosition(GetDelegate()->GetTextLength());
}

void StbTextEditBridge::ClearUndoStack()
{
    stb_state->undostate.undo_point = 0;
    stb_state->undostate.undo_char_point = 0;
    stb_state->undostate.redo_point = STB_TEXTEDIT_UNDOSTATECOUNT;
    stb_state->undostate.redo_char_point = STB_TEXTEDIT_UNDOCHARCOUNT;
}

bool StbTextEditBridge::CutToClipboard()
{
    uint32 selStart = std::min(GetSelectionStart(), GetSelectionEnd());
    uint32 selEnd = std::max(GetSelectionStart(), GetSelectionEnd());
    if (selStart < selEnd)
    {
        WideString selectedText = GetDelegate()->GetText().substr(selStart, selEnd - selStart);
        if (Clipboard().SetText(selectedText))
        {
            return Cut();
        }
    }
    return false;
}

bool StbTextEditBridge::CopyToClipboard()
{
    uint32 selStart = std::min(GetSelectionStart(), GetSelectionEnd());
    uint32 selEnd = std::max(GetSelectionStart(), GetSelectionEnd());
    if (selStart < selEnd)
    {
        WideString selectedText = GetDelegate()->GetText().substr(selStart, selEnd - selStart);
        return Clipboard().SetText(selectedText);
    }
    return false;
}

bool StbTextEditBridge::PasteFromClipboard()
{
    WideString clipText;
    Clipboard clip;
    if (clip.HasText())
    {
        clipText = clip.GetText();
        clipText.erase(std::remove_if(clipText.begin(), clipText.end(),
                                      [&](WideString::value_type& ch)
                                      {
                                          return !GetDelegate()->IsCharAvaliable(ch);
                                      }),
                       clipText.end());
        if (!clipText.empty())
        {
            return Paste(clipText);
        }
    }
    return false;
}
}
