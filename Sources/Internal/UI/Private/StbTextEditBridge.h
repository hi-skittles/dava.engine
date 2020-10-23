#pragma once

#include "Base/BaseTypes.h"
#include "Engine/EngineTypes.h"
#include "Math/Vector.h"
#include "Input/InputElements.h"

namespace DAVA
{
class TextBox;
class UIEvent;
struct StbState;

/** Class that implements bridge for stb_textedit. */
class StbTextEditBridge
{
public:
    /** Delegate to implement stb_textedit callbacks. */
    class StbTextDelegate
    {
    public:
        /** Default destructor. */
        virtual ~StbTextDelegate() = default;

        /** 
        Insert specified `string` with `length` to `position` in data structure.
        Return count of inserted characters.
        */
        virtual uint32 InsertText(uint32 position, const WideString::value_type* string, uint32 length) = 0;

        /** 
        Delete text with specified `length` on `position` from data structure.
        Return count of deleted characters.
        */
        virtual uint32 DeleteText(uint32 position, uint32 length) = 0;

        /** Get instance of TextBox strcuture from field. */
        virtual const TextBox* GetTextBox() const = 0;

        /** Get text length. */
        virtual uint32 GetTextLength() const = 0;

        /** Get character at specified `index` from text. */
        virtual WideString::value_type GetCharAt(uint32 index) const = 0;

        /** Get text from field. */
        virtual WideString GetText() const = 0;

        /** Check that specified `character` is valid for current field. */
        virtual bool IsCharAvaliable(WideString::value_type character) const = 0;

        /** Allow copy or cut selected text to clipnoard. */
        virtual bool IsCopyToClipboardAllowed() const
        {
            return true;
        }

        /** Allow paste specified string from clipbaord to field. */
        virtual bool IsPasteFromClipboardAllowed() const
        {
            return true;
        }
    };

    /** Create StbTextEditBridge with specified valid `delegate`. */
    StbTextEditBridge(StbTextDelegate* delegate);

    /** Copy constructor. */
    StbTextEditBridge(const StbTextEditBridge& c);

    /** Destructor. */
    virtual ~StbTextEditBridge();

    /** Copy class data to correct instance. */
    virtual void CopyStbStateFrom(const StbTextEditBridge& c);

    /**
    Send specified `key` with `modifiers` from UIEvent to STB text edit.
    Return true if content of field has been changed.
    */
    virtual bool SendKey(eInputElements key, eModifierKeys modifiers);

    /**
    Send specified `keyChar` with `modifiers` to STB text edit.
    Return true if content of field has been changed.
    */
    virtual bool SendKeyChar(uint32 keyChar, eModifierKeys modifiers);

    /**
    Send specified raw `codePoint` to STB text edit.
    Return true if content of field has been changed.
    */
    virtual bool SendRaw(uint32 codePoint);

    /**
    Send mouse click with specified `point` in control's local 
    coordinates to STB text edit.
    */
    virtual void Click(const Vector2& point);

    /**
    Send mouse drag event with specified `point` in control's local
    coordinates to STB text edit.
    */
    virtual void Drag(const Vector2& point);

    /** Cut (delete) selected text. */
    virtual bool Cut();

    /** Insert (replace selected) new specified `string` in field. */
    virtual bool Paste(const WideString& string);

    /** Clear STB text edit undo stack. */
    virtual void ClearUndoStack();

    /**
    Get character index of selection start.
    Cursor equal 0 - cursor before first symbol,
    cursor equal text length - cursor after last symbol.
    */
    uint32 GetSelectionStart() const;

    /**
    Move begin selection to specified `position`.
    Cursor equal 0 - cursor before first symbol,
    cursor equal text length - cursor after last symbol.
    */
    void SetSelectionStart(uint32 position) const;

    /**
    Get character index of selection end.
    Cursor equal 0 - cursor before first symbol,
    cursor equal text length - cursor after last symbol.
    */
    uint32 GetSelectionEnd() const;

    /**
    Move end selection to specified `position`.
    Cursor equal 0 - cursor before first symbol,
    cursor equal text length - cursor after last symbol.
    */
    void SetSelectionEnd(uint32 position) const;

    /**
    Get character index of cursor position.
    Cursor equal 0 - cursor before first symbol,
    cursor equal text length - cursor after last symbol.
    */
    uint32 GetCursorPosition() const;

    /**
    Move cursor to specified `position`.
    Cursor equal 0 - cursor before first symbol,
    cursor equal text length - cursor after last symbol.
    */
    void SetCursorPosition(uint32 position) const;

    /** Enable or disable single line mode. */
    void SetSingleLineMode(bool signleLine);

    /** Return single line mode flag. */
    bool IsSingleLineMode() const;

    /** Return inserting mode flag.*/
    bool IsInsertMode() const;

    /** Select word neer cursor position. */
    void SelectWord();

    /** Select whole text. */
    void SelectAll();

    /**
    Cut selected text to clipboard.
    Return true if content has been changed.
    */
    bool CutToClipboard();

    /**
    Copy seleted text to clipboard.
    Return true if text has been copied to clipboard.
    */
    bool CopyToClipboard();

    /**
    Replace selected text with text from clipboard or insert text
    cursor position.
    Return true if content has been changed.
    */
    bool PasteFromClipboard();

    /** Get pointer delegate instance. */
    StbTextDelegate* GetDelegate() const;

private:
    StbState* stb_state = nullptr; //!< Inner STB state structure ptr
    StbTextDelegate* delegate = nullptr; //!< Inner delegate to implement stb callbacks
};

inline StbTextEditBridge::StbTextDelegate* StbTextEditBridge::GetDelegate() const
{
    return delegate;
}
}
