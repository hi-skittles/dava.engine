#pragma once
#include "Base/BaseTypes.h"

namespace DAVA
{
class UITextField;
struct Rect;
/**
    \brief  The UITextFieldDelegate interface defines the messages sent to a text field delegate as part of the sequence of editing its text. 
            All the methods of the interface is optional.
 */
class UITextFieldDelegate
{
public:
    enum class eReason
    {
        USER = 0,
        CODE = 1,
    };

    virtual ~UITextFieldDelegate() = default;

    /**
        \brief Asks the delegate if the text field should process the pressing of the return button.
        In this function you can check what you want to do with UITextField when return button pressed.
        Works only in single line mode.
     */
    virtual void TextFieldShouldReturn(UITextField* /*textField*/)
    {
    }

    /**
        \brief Asks the delegate if the text field should process the pressing of the ESC button.
        In this function you can check what you want to do with UITextField when ESC button pressed.
        Doesn't work on iOS for now.
     */
    virtual void TextFieldShouldCancel(UITextField* /*textField*/)
    {
    }
    virtual void TextFieldLostFocus(UITextField* /*textField*/)
    {
    }

    /**
        \brief Asks the delegate if the specified text should be changed.
        \param[in] textField The text field containing the text.
        \param[in] replacementLocation starting position of range of characters to be replaced
        \param[in] replacementLength ending position of range of characters to be replaced
        \param[in] replacementString the replacement string.
        \returns true if the specified text range should be replaced; otherwise, false to keep the old text. Default implementation returns true.
     */
    virtual bool TextFieldKeyPressed(UITextField* /*textField*/, int32 /*replacementLocation*/, int32 /*replacementLength*/, WideString& /*replacementString*/)
    {
        return true;
    }

    DAVA_DEPRECATED(virtual void TextFieldOnTextChanged(UITextField* /*textField*/, const WideString& /*newText*/, const WideString& /*oldText*/)
                    {
                    });

    virtual void TextFieldOnTextChanged(UITextField* textField, const WideString& newText, const WideString& oldText, eReason type);

    /*
        \brief Called when device keyboard is displayed/hidden.
        */
    virtual void OnKeyboardShown(const Rect& keyboardRect)
    {
    }

    virtual void OnKeyboardHidden()
    {
    }

    DAVA_DEPRECATED(virtual void OnStartEditing())
    {
    }

    DAVA_DEPRECATED(virtual void OnStopEditing())
    {
    }

    virtual void OnStartEditing(UITextField* textField)
    {
        OnStartEditing();
    }

    virtual void OnStopEditing(UITextField* textField)
    {
        OnStopEditing();
    }
};
} // namespace DAVA
