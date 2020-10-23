#pragma once

#include <TArc/Core/BaseApplication.h>

namespace DAVA
{
class Core;
}

class QEApplication : public DAVA::BaseApplication
{
public:
    QEApplication(DAVA::Vector<DAVA::String>&& cmdLine);
    // desctructor will never call on some platforms,
    // so release all resources in Cleanup method.
    ~QEApplication() = default;

protected:
    EngineInitInfo GetInitInfo() const override;
    void CreateModules(DAVA::Core* tarcCore) const override;

    void RegisterReflectionExtensions() override;

private:
    void Init(const DAVA::EngineContext* engineContext) override;
    void Cleanup() override;

    bool AllowMultipleInstances() const override;
    QString GetInstanceKey() const override;

    bool isConsoleMode = false;
    DAVA::Vector<DAVA::String> cmdLine;
};
