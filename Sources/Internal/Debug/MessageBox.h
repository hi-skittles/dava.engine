#pragma once

namespace DAVA
{
namespace Debug
{
/**
    \ingroup debug
    Display platform dependant modal message box and wait user action.

    `MessageBox` supports up to 3 buttons, any button can be default if platform provides such an ability.
    `MessageBox` returns zero-based index of button clicked by user or -1 under the following conditions:
        - calling thread cannot be blocked if `MessageBox` is invoked from UI thread on Win10 and Android.
        - application goes background if `MessageBox` is invoked from DAVA main thread.
        - system cannot display message box.
    `MessageBox` does its best to try to show message box at any time when application is running but to ensure message box is
    always shown call `MessageBox` function when application has entered `Engine::Run` method.

    Message box visual representation, buttons order, ability to specify default button depends on underlying system.
    Some platform-dependant limitations and differences:
        - mobile platforms usually do not provide ability to specify default button.
        - first button is usually leftmost and last button is rightmost, but some platforms (macOS) show buttons in reverse order.
        - Win10 Mobile platform allows only two buttons in message box (third button is ignored if any).
        - if message box is shown from DAVA main thread and application goes suspended then `MessageBox` function immediately returns
          with result -1 and message box may be dismissed.
        - on some platforms (Win10, Android) if message box is show from UI thread `MessageBox` immediately returns with result -1
          but modal dialog stay on screen.

    ~~~~~~~~~{.cpp}
        // Show message box with three buttons: "One", "Two", "Three", with default button "Two"
        int r = DAVA::Debug::MessageBox("MessageBox", "Choose your destiny", {"One", "Two", "Three"}, 1);
        if (r == 0)
        { // user clicked button "One" }
        else if (r == 1)
        { // user clicked button "Two" }
        else if (r == 2)
        { // user clicked button "Three" }
        else if (r < 0)
        { // Ops, MessageBox cannot show blocking modal dialog }
    ~~~~~~~~~ 
*/

int MessageBox(const String& title, const String& message, const Vector<String>& buttons, int defaultButton = 0);

} // namespace Debug
} // namespace DAVA
