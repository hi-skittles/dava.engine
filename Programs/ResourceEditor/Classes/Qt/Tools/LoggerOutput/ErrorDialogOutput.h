#pragma once

#include <TArc/Utils/QtDelayedExecutor.h>

#include <Concurrency/Mutex.h>
#include <Logger/Logger.h>
#include <Functional/Signal.h>

#include <QObject>

#include <memory>
#include <atomic>

namespace DAVA
{
class UI;
}

class ErrorDialogOutput final : public DAVA::QtDelayedExecutor, public DAVA::LoggerOutput
{
public:
    ErrorDialogOutput(DAVA::UI* ui);

    void Output(DAVA::Logger::eLogLevel ll, const DAVA::char8* text) override;
    void Disable();

private:
    void ShowErrorDialog();
    void ShowErrorDialogImpl();

    class IgnoreHelper;
    std::unique_ptr<IgnoreHelper> ignoreHelper;

    DAVA::UnorderedSet<DAVA::String> errors;
    DAVA::Mutex errorsLocker;

    std::atomic<bool> isJobStarted;
    std::atomic<bool> enabled;

    DAVA::Token waitDialogConnectionToken;
    DAVA::UI* tarcUI = nullptr;
};
