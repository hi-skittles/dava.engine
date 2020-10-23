#include "Debug/DVAssertDefaultHandlers.h"
#include "Debug/DebuggerDetection.h"
#include "Debug/Backtrace.h"
#include "Debug/MessageBox.h"
#include "Logger/Logger.h"
#include "Utils/StringFormat.h"

namespace DAVA
{
namespace Assert
{
const String AssertMessageTag = "end=assert=msg";

FailBehaviour DefaultLoggerHandler(const AssertInfo& assertInfo)
{
    // clang-format off
    Logger::Error("========================================\n"
                  "DVASSERT failed\n"
                  "Expression: %s\n"
                  "Message: %s\n"
                  "At %s:%d\n"
                  "======================%s====",
                  assertInfo.expression,
                  assertInfo.message,
                  assertInfo.fileName,
                  assertInfo.lineNumber,
                  AssertMessageTag.c_str());

    DAVA::Logger::Error("==== callstack ====\n"
                        "%s\n"
                        "==== callstack end ====",
                        Debug::GetBacktraceString(assertInfo.backtrace).c_str());
    // clang-format on

    // Even though it's more appropriate to return FailBehaviour::Default,
    // return FailBehaviour::Continue to match behaviour of an old assert system
    return FailBehaviour::Continue;
}

FailBehaviour DefaultDialogBoxHandler(const AssertInfo& assertInfo)
{
// Android and iOS both allow content scrolling in assert dialog, so show full backtrace
// On desktops dialogs are not scrollable so limit frames
#if defined(__DAVAENGINE_MACOS__) || defined(__DAVAENGINE_WIN32__)
    const int backtraceDepth = 8;
#else
    const int backtraceDepth = 0;
#endif

    //'message' should be UTF-8, otherwise we have empty AlertDialog on iOS and NSException from
    // NSAlert::setInformativeText on MacOS

    // clang-format off
    String message = Format("DVASSERT failed\n"
                            "Expression: %s\n"
                            "Message: %s\n"
                            "At %s:%d\n"
                            "Callstack:\n"
                            "%s",
                            assertInfo.expression,
                            assertInfo.message,
                            assertInfo.fileName,
                            assertInfo.lineNumber,
                            Debug::GetBacktraceString(assertInfo.backtrace, backtraceDepth).c_str());
    // clang-format on
    int choice = Debug::MessageBox("Assert", message, { "Break", "Continue" }, 1);
    // If by some reason MessageBox cannot be shown or is shown non-modal break execution
    return choice <= 0 ? FailBehaviour::Halt : FailBehaviour::Continue;
}

FailBehaviour DefaultDebuggerBreakHandler(const AssertInfo& assertInfo)
{
    return (IsDebuggerPresent() == true) ? FailBehaviour::Halt : FailBehaviour::Continue;
}

void SetupDefaultHandlers()
{
    RemoveAllHandlers();

#ifdef ENABLE_ASSERT_LOGGING
    AddHandler(DefaultLoggerHandler);
#endif

#ifdef ENABLE_ASSERT_MESSAGE
    AddHandler(DefaultDialogBoxHandler);
#endif
}
}
}
