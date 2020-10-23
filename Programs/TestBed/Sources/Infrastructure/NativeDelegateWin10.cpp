#include "Infrastructure/NativeDelegateWin10.h"

#if defined(__DAVAENGINE_WIN_UAP__)

#include <Logger/Logger.h>
#include <Utils/UTF8Utils.h>
#include <Utils/Utils.h>

const char* ExecutionStateToString(::Windows::ApplicationModel::Activation::ApplicationExecutionState execState)
{
    using ::Windows::ApplicationModel::Activation::ApplicationExecutionState;
    switch (execState)
    {
    case ApplicationExecutionState::NotRunning:
        return "not running";
    case ApplicationExecutionState::Running:
        return "running";
    case ApplicationExecutionState::Suspended:
        return "suspended";
    case ApplicationExecutionState::Terminated:
        return "terminated";
    case ApplicationExecutionState::ClosedByUser:
        return "closed by user";
    default:
        return "unknown";
    }
}

DAVA::String ActivationKindToString(::Windows::ApplicationModel::Activation::ActivationKind kind)
{
    using namespace DAVA;
    using ::Windows::ApplicationModel::Activation::ActivationKind;

    auto toText = [](ActivationKind kind) -> const char* {
        switch (kind)
        {
        case ActivationKind::Launch:
            return "Launch";
        case ActivationKind::Search:
            return "Search";
        case ActivationKind::File:
            return "File";
        case ActivationKind::Protocol:
            return "Protocol";
        case ActivationKind::ToastNotification:
            return "ToastNotification";
        default:
            return "other";
        }
    };
    return Format("%s (%d)", toText(kind), kind);
}

void NativeDelegateWin10::OnLaunched(::Windows::ApplicationModel::Activation::LaunchActivatedEventArgs ^ args)
{
    using namespace DAVA;
    String arguments = RTStringToString(args->Arguments);
    Logger::Debug("TestBed.NativeDelegateWin10::OnLaunched: arguments=%s, activationKind=%s", arguments.c_str(), ActivationKindToString(args->Kind).c_str());
}

void NativeDelegateWin10::OnActivated(::Windows::ApplicationModel::Activation::IActivatedEventArgs ^ args)
{
    using namespace DAVA;
    Logger::Debug("TestBed.NativeDelegateWin10::OnActivated: activationKind=%s, previousExecutionState=%s",
                  ActivationKindToString(args->Kind).c_str(),
                  ExecutionStateToString(args->PreviousExecutionState));
}

#endif // __DAVAENGINE_WIN_UAP__
