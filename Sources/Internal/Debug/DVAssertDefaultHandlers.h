#pragma once

#include "Base/BaseTypes.h"
#include "DVAssert.h"

namespace DAVA
{
namespace Assert
{
/**
Part of a message each predefined handler generates
Can be used to distinguish assert messages when processing log for some reason
*/
extern const String AssertMessageTag;

/** Log assert info as an error and return FailBehaviour::Continue */
FailBehaviour DefaultLoggerHandler(const AssertInfo& assertInfo);

/** Show message box with assert info asking user if program should be halted. Return FailBehaviour::Halt if it should */
FailBehaviour DefaultDialogBoxHandler(const AssertInfo& assertInfo);

/** Detect debuger and return FailBehaviour::Halt if debugger connected otherwise return FailBehaviour::Continue. Appliable for console applications */
FailBehaviour DefaultDebuggerBreakHandler(const AssertInfo& assertInfo);

/** Setup assert system using default handlers, based on preprocessor flags: ENABLE_ASSERT_MESSAGE & ENABLE_ASSERT_LOGGING */
void SetupDefaultHandlers(); // TODO: Remove after CoreV2 arrives
}
}
