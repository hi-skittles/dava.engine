#include "UITextFieldDelegate.h"
#include "Debug/DVAssert.h"

namespace DAVA
{
void UITextFieldDelegate::TextFieldOnTextChanged(UITextField* textField, const WideString& newText, const WideString& oldText, eReason type)
{
    DVASSERT(newText != oldText);
    TextFieldOnTextChanged(textField, newText, oldText);
}
};