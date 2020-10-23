#include "Classes/Qt/Tools/LoggerOutput/ErrorDialogOutput.h"
#include "Classes/Application/RESettings.h"
#include "Classes/Application/REGlobal.h"

#include <TArc/Utils/AssertGuard.h>
#include <TArc/WindowSubSystem/UI.h>

#include <Concurrency/LockGuard.h>
#include <Debug/DVAssertDefaultHandlers.h>
#include <Debug/MessageBox.h>
#include <Engine/PlatformApiQt.h>
#include <Utils/StringFormat.h>

namespace ErrorDialogDetail
{
static const DAVA::uint32 maxErrorsPerDialog = 6;
static const DAVA::String errorDivideLine("--------------------\n");

bool ShouldBeHiddenByUI()
{ //need be filled with context for special cases after Qa and Using
    return false;
}
}

class ErrorDialogOutput::IgnoreHelper
{
public:
    bool ShouldIgnoreMessage(DAVA::Logger::eLogLevel ll, const DAVA::String& textMessage)
    {
        GeneralSettings* settings = REGlobal::GetGlobalContext()->GetData<GeneralSettings>();
        if ((ll < DAVA::Logger::LEVEL_ERROR) || settings->showErrorDialog == false)
        {
            return true;
        }

        if (DAVA::TArc::IsInsideAssertHandler())
        {
            return true;
        }

        return HasIgnoredWords(textMessage);
    }

private:
    bool HasIgnoredWords(const DAVA::String& testedString)
    {
        static const DAVA::String callstackProlog = "==== callstack ====";
        static const DAVA::String callstackEpilog = "==== callstack end ====";

        if (testedString.find(callstackEpilog) != DAVA::String::npos)
        {
            callstackPrinting = false;
            return true;
        }

        if (callstackPrinting == true)
        {
            return true;
        }

        if (testedString.find(callstackProlog) != DAVA::String::npos)
        {
            callstackPrinting = true;
            return true;
        }

        return false;
    }

private:
    bool callstackPrinting = false;
};

ErrorDialogOutput::ErrorDialogOutput(DAVA::TArc::UI* ui)
    : ignoreHelper(new IgnoreHelper())
    , isJobStarted(false)
    , enabled(true)
    , tarcUI(ui)
{
    DVASSERT(tarcUI != nullptr);
    errors.reserve(ErrorDialogDetail::maxErrorsPerDialog);
}

void ErrorDialogOutput::Output(DAVA::Logger::eLogLevel ll, const DAVA::char8* text)
{
    if (!enabled || ignoreHelper->ShouldIgnoreMessage(ll, text))
    {
        return;
    }

    { //lock container to add new text
        DAVA::LockGuard<DAVA::Mutex> lock(errorsLocker);
        if (errors.size() < ErrorDialogDetail::maxErrorsPerDialog)
        {
            errors.insert(text);
        }
    }

    if (isJobStarted == false)
    {
        DVASSERT(waitDialogConnectionToken.IsEmpty());

        isJobStarted = true;
        DelayedExecute(DAVA::MakeFunction(this, &ErrorDialogOutput::ShowErrorDialog));
    }
}

void ErrorDialogOutput::ShowErrorDialog()
{
    DVASSERT(isJobStarted == true);

    if (tarcUI->HasActiveWaitDalogues())
    {
        DVASSERT(waitDialogConnectionToken.IsEmpty());
        waitDialogConnectionToken = tarcUI->lastWaitDialogWasClosed.Connect(this, &ErrorDialogOutput::ShowErrorDialog);
        return;
    }

    { // disconnect from
        if (!waitDialogConnectionToken.IsEmpty())
        {
            tarcUI->lastWaitDialogWasClosed.Disconnect(waitDialogConnectionToken);
            waitDialogConnectionToken.Clear();
        }
        isJobStarted = false;
    }

    DelayedExecute(DAVA::MakeFunction(this, &ErrorDialogOutput::ShowErrorDialogImpl));
}

void ErrorDialogOutput::ShowErrorDialogImpl()
{
    if (ErrorDialogDetail::ShouldBeHiddenByUI() || enabled == false)
    {
        DAVA::LockGuard<DAVA::Mutex> lock(errorsLocker);
        errors.clear();
        return;
    }

    DAVA::String title;
    DAVA::String errorMessage;

    {
        DAVA::LockGuard<DAVA::Mutex> lock(errorsLocker);
        DAVA::uint32 totalErrors = static_cast<DAVA::uint32>(errors.size());

        if (totalErrors == 0)
        {
            return;
        }

        if (totalErrors == 1)
        {
            title = "Error occurred";
            errorMessage = *errors.begin();
        }
        else
        {
            title = DAVA::Format("%u errors occurred", totalErrors);
            for (const auto& message : errors)
            {
                errorMessage += message + ErrorDialogDetail::errorDivideLine;
            }

            if (totalErrors == ErrorDialogDetail::maxErrorsPerDialog)
            {
                errorMessage += "\nSee console log for details.";
            }
        }

        errors.clear();
    }

    DAVA::Debug::MessageBox(title, errorMessage, { "Close" });
}

void ErrorDialogOutput::Disable()
{
    enabled = false;
}
